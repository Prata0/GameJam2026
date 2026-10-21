#include "Input/StdInputSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputDeveloperSettings.h"
#include "EnhancedInputSubsystems.h"
#include "Input/StdInputBinding.h"
#include "InputMappingContext.h"
#include "Settings/StdProjectSettings.h"
#include "StdLog.h"

#define LOCTEXT_NAMESPACE "StdCore"

void UStdInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UEnhancedInputLocalPlayerSubsystem>();
	Super::Initialize(Collection);
}

void UStdInputSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RetryHandle);
	RetryHandle.Reset();
	Super::Deinitialize();
}

void UStdInputSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	if (!NewPlayerController)
	{
		return;
	}

	ApplyMappingContexts();

	// The Enhanced Input subsystem creates the user settings and the player input in the same notification, and the
	// order between subsystems is not guaranteed: apply once more on the next tick. ApplyMappingContexts is idempotent.
	if (!RetryHandle.IsValid())
	{
		RetryHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickRetryApply));
	}
}

bool UStdInputSubsystem::TickRetryApply(float DeltaTime)
{
	RetryHandle.Reset();
	ApplyMappingContexts();
	return false;
}

UEnhancedInputLocalPlayerSubsystem* UStdInputSubsystem::GetEnhancedInput() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
}

UEnhancedInputUserSettings* UStdInputSubsystem::GetUserSettings() const
{
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = GetEnhancedInput();
	UEnhancedInputUserSettings* UserSettings = EnhancedInput ? EnhancedInput->GetUserSettings() : nullptr;
	if (!UserSettings && !GetDefault<UEnhancedInputDeveloperSettings>()->bEnableUserSettings)
	{
		StdLog::WarnOnce(TEXT("EnhancedInputUserSettingsDisabled"),
			TEXT("Enhanced Input user settings are disabled (Project Settings > Engine > Enhanced Input > Enable User Settings): key remapping is not available."));
	}
	return UserSettings;
}

UEnhancedPlayerMappableKeyProfile* UStdInputSubsystem::GetActiveProfile() const
{
	const UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	return UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
}

void UStdInputSubsystem::ApplyMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* EnhancedInput = GetEnhancedInput();
	if (!EnhancedInput)
	{
		return;
	}

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	bool bRegisteredNewContext = false;

	for (int32 Index = 0; Index < Settings->MappingContexts.Num(); ++Index)
	{
		const FStdMappingContextSetting& Entry = Settings->MappingContexts[Index];
		const UInputMappingContext* MappingContext = StdSettings::LoadAsset(Entry.MappingContext,
			FString::Printf(TEXT("MappingContexts[%d]"), Index), StdCoreDefaults::MappingContext);
		if (!MappingContext)
		{
			continue;
		}

		// Registered before being added, so the first rebuild already uses the player's saved keys.
		if (UserSettings && !UserSettings->IsMappingContextRegistered(MappingContext))
		{
			bRegisteredNewContext |= UserSettings->RegisterInputMappingContext(MappingContext);
		}

		if (!EnhancedInput->HasMappingContext(MappingContext))
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = false;
			EnhancedInput->AddMappingContext(MappingContext, Entry.Priority, Options);
		}
	}

	if (bRegisteredNewContext)
	{
		EnhancedInput->RequestRebuildControlMappings();
	}
}

bool UStdInputSubsystem::IsKeyboardMouseMapping(const FPlayerKeyMapping& Mapping)
{
	const FKey& Key = Mapping.GetDefaultKey();
	return Key.IsValid() && !Key.IsGamepadKey() && !Key.IsTouch() && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D();
}

TArray<FStdKeyBindingRow> UStdInputSubsystem::GetRemappableKeys() const
{
	TArray<FStdKeyBindingRow> Rows;
	const UEnhancedPlayerMappableKeyProfile* Profile = GetActiveProfile();
	if (!Profile)
	{
		return Rows;
	}

	for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
	{
		for (const FPlayerKeyMapping& Mapping : RowPair.Value.Mappings)
		{
			if (!IsKeyboardMouseMapping(Mapping))
			{
				continue;
			}

			FStdKeyBindingRow& Row = Rows.AddDefaulted_GetRef();
			Row.MappingName = Mapping.GetMappingName();
			Row.Slot = Mapping.GetSlot();
			Row.DisplayName = Mapping.GetDisplayName().IsEmpty() ? FText::FromName(Row.MappingName) : Mapping.GetDisplayName();
			Row.Category = Mapping.GetDisplayCategory();
			Row.CurrentKey = Mapping.GetCurrentKey();
			Row.DefaultKey = Mapping.GetDefaultKey();
		}
	}

	// Group by category, keeping the order in which categories first appear (the order of the mapping contexts).
	TArray<FString> CategoryOrder;
	for (const FStdKeyBindingRow& Row : Rows)
	{
		CategoryOrder.AddUnique(Row.Category.ToString());
	}
	Rows.StableSort([&CategoryOrder](const FStdKeyBindingRow& A, const FStdKeyBindingRow& B)
	{
		return CategoryOrder.IndexOfByKey(A.Category.ToString()) < CategoryOrder.IndexOfByKey(B.Category.ToString());
	});

	return Rows;
}

const FPlayerKeyMapping* UStdInputSubsystem::FindMapping(FName MappingName, EPlayerMappableKeySlot Slot) const
{
	const UEnhancedPlayerMappableKeyProfile* Profile = GetActiveProfile();
	const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(MappingName) : nullptr;
	if (!Row)
	{
		return nullptr;
	}

	for (const FPlayerKeyMapping& Mapping : Row->Mappings)
	{
		if (Mapping.GetSlot() == Slot)
		{
			return &Mapping;
		}
	}
	return nullptr;
}

bool UStdInputSubsystem::CanAssignKey(FKey Key, FText& OutReason) const
{
	if (!Key.IsValid() || Key == EKeys::AnyKey)
	{
		OutReason = LOCTEXT("KeyInvalid", "No key was selected.");
		return false;
	}
	if (Key.IsGamepadKey())
	{
		OutReason = LOCTEXT("KeyGamepad", "Gamepad buttons cannot be remapped.");
		return false;
	}
	if (Key.IsTouch() || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())
	{
		OutReason = FText::Format(LOCTEXT("KeyAxis", "{0} cannot be assigned to an action."), Key.GetDisplayName());
		return false;
	}
	if (UStdProjectSettings::Get()->ReservedKeys.Contains(Key))
	{
		OutReason = FText::Format(LOCTEXT("KeyReserved", "{0} is reserved and cannot be assigned."), Key.GetDisplayName());
		return false;
	}

	OutReason = FText::GetEmpty();
	return true;
}

EStdRemapResult UStdInputSubsystem::RemapKey(FName MappingName, EPlayerMappableKeySlot Slot, FKey NewKey, FText& OutMessage)
{
	OutMessage = FText::GetEmpty();

	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		OutMessage = LOCTEXT("RemapUnavailable", "Key remapping is not available.");
		return EStdRemapResult::Rejected;
	}

	const FPlayerKeyMapping* Mapping = FindMapping(MappingName, Slot);
	if (!Mapping || !IsKeyboardMouseMapping(*Mapping))
	{
		OutMessage = LOCTEXT("RemapUnknownAction", "This action cannot be remapped.");
		UE_LOG(LogStdCore, Warning, TEXT("RemapKey: no remappable keyboard/mouse mapping named '%s'."), *MappingName.ToString());
		return EStdRemapResult::Rejected;
	}

	if (!CanAssignKey(NewKey, OutMessage))
	{
		return EStdRemapResult::Rejected;
	}

	// Copied now: mapping pointers do not survive MapPlayerKey.
	const FKey OldKey = Mapping->GetCurrentKey();
	const FName HardwareDeviceId = Mapping->GetHardwareDeviceId().HardwareDeviceIdentifier;
	if (OldKey == NewKey)
	{
		return EStdRemapResult::Unchanged;
	}

	TOptional<FStdKeyBindingRow> OtherRow;
	for (const FStdKeyBindingRow& Row : GetRemappableKeys())
	{
		if (Row.CurrentKey == NewKey && !(Row.MappingName == MappingName && Row.Slot == Slot))
		{
			OtherRow = Row;
			break;
		}
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = Slot;
	Args.NewKey = NewKey;
	Args.HardwareDeviceId = HardwareDeviceId;
	FGameplayTagContainer FailureReason;
	UserSettings->MapPlayerKey(Args, FailureReason);
	if (!FailureReason.IsEmpty())
	{
		UE_LOG(LogStdCore, Warning, TEXT("RemapKey(%s -> %s) failed: %s"), *MappingName.ToString(), *NewKey.ToString(), *FailureReason.ToString());
		OutMessage = LOCTEXT("RemapFailed", "The key could not be assigned.");
		return EStdRemapResult::Rejected;
	}

	EStdRemapResult Result = EStdRemapResult::Remapped;
	if (OtherRow.IsSet())
	{
		const FPlayerKeyMapping* OtherMapping = FindMapping(OtherRow->MappingName, OtherRow->Slot);
		FMapPlayerKeyArgs SwapArgs;
		SwapArgs.MappingName = OtherRow->MappingName;
		SwapArgs.Slot = OtherRow->Slot;
		SwapArgs.NewKey = OldKey;
		SwapArgs.HardwareDeviceId = OtherMapping ? OtherMapping->GetHardwareDeviceId().HardwareDeviceIdentifier : NAME_None;
		FGameplayTagContainer SwapFailureReason;
		UserSettings->MapPlayerKey(SwapArgs, SwapFailureReason);
		if (!SwapFailureReason.IsEmpty())
		{
			UE_LOG(LogStdCore, Warning, TEXT("RemapKey: swapping %s onto '%s' failed: %s"),
				*OldKey.ToString(), *OtherRow->MappingName.ToString(), *SwapFailureReason.ToString());
		}

		Result = EStdRemapResult::Swapped;
		OutMessage = FText::Format(LOCTEXT("RemapSwapped", "{0} was used by \"{1}\": the two actions swapped keys ({1} is now {2})."),
			NewKey.GetDisplayName(), OtherRow->DisplayName, OldKey.GetDisplayName());
	}

	UE_LOG(LogStdCore, Log, TEXT("Key for '%s' changed from %s to %s%s."), *MappingName.ToString(), *OldKey.ToString(), *NewKey.ToString(),
		OtherRow.IsSet() ? *FString::Printf(TEXT(" (swapped with '%s')"), *OtherRow->MappingName.ToString()) : TEXT(""));

	CommitKeyChanges();
	return Result;
}

void UStdInputSubsystem::ResetKeysToDefaults()
{
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	FGameplayTagContainer FailureReason;
	UserSettings->ResetKeyProfileIdToDefault(UserSettings->GetActiveKeyProfileId(), FailureReason);
	if (!FailureReason.IsEmpty())
	{
		UE_LOG(LogStdCore, Warning, TEXT("Resetting keys to defaults reported: %s"), *FailureReason.ToString());
	}

	UE_LOG(LogStdCore, Log, TEXT("Keys reset to defaults."));
	CommitKeyChanges();
}

void UStdInputSubsystem::CommitKeyChanges()
{
	if (UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		UserSettings->ApplySettings();
		UserSettings->SaveSettings();
	}
	if (UEnhancedInputLocalPlayerSubsystem* EnhancedInput = GetEnhancedInput())
	{
		EnhancedInput->RequestRebuildControlMappings();
	}
	OnKeyMappingsChanged.Broadcast();
}

FKey UStdInputSubsystem::GetKeyForTag(FGameplayTag Tag, bool bPreferGamepad) const
{
	const UInputAction* Action = StdInput::FindInputActionForTag(Tag);
	if (!Action)
	{
		StdInput::WarnMissingTag(Tag, nullptr);
		return EKeys::Invalid;
	}

	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = GetEnhancedInput();
	if (!EnhancedInput)
	{
		return EKeys::Invalid;
	}

	// The live mappings of the player, which already contain the remapped keys.
	FKey Fallback;
	for (const FKey& Key : EnhancedInput->QueryKeysMappedToAction(Action))
	{
		if (!Key.IsValid() || Key.IsTouch())
		{
			continue;
		}
		if (Key.IsGamepadKey() == bPreferGamepad)
		{
			return Key;
		}
		if (!Fallback.IsValid())
		{
			Fallback = Key;
		}
	}
	return Fallback;
}

FText UStdInputSubsystem::GetKeyDisplayTextForTag(FGameplayTag Tag, bool bPreferGamepad) const
{
	const FKey Key = GetKeyForTag(Tag, bPreferGamepad);
	return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false) : LOCTEXT("KeyUnbound", "Unbound");
}

#undef LOCTEXT_NAMESPACE
