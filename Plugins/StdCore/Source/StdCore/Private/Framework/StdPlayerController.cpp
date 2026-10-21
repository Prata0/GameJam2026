#include "Framework/StdPlayerController.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Flow/StdFlowSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/StdPauseKeyProcessor.h"
#include "Input/StdInputBinding.h"
#include "Input/StdInputSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/StdProjectSettings.h"
#include "StdGameplayTags.h"
#include "StdLog.h"
#include "UI/StdPauseMenuWidget.h"

AStdPlayerController::AStdPlayerController()
{
	bShowMouseCursor = false;
}

void AStdPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UStdInputSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UStdInputSubsystem>(GetLocalPlayer()))
		{
			InputSubsystem->ApplyMappingContexts();
		}

		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void AStdPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetPauseKeyProcessorActive(false);
	Super::EndPlay(EndPlayReason);
}

void AStdPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		StdLog::WarnOnce(TEXT("NoEnhancedInputComponent"),
			TEXT("AStdPlayerController needs an EnhancedInputComponent: set Project Settings > Engine > Input > Default Input Component Class to EnhancedInputComponent. Pause and input tags are disabled."));
		return;
	}

	StdInput::BindActionByTag(EnhancedInputComponent, StdGameplayTags::InputTag_Pause, ETriggerEvent::Started, this, &ThisClass::HandlePauseAction);

	TArray<TPair<const UInputAction*, FGameplayTag>> TaggedActions;
	StdInput::GetAllTaggedActions(TaggedActions);
	for (const TPair<const UInputAction*, FGameplayTag>& TaggedAction : TaggedActions)
	{
		for (const ETriggerEvent TriggerEvent : { ETriggerEvent::Started, ETriggerEvent::Triggered, ETriggerEvent::Completed, ETriggerEvent::Canceled })
		{
			EnhancedInputComponent->BindAction(TaggedAction.Key, TriggerEvent, this, &ThisClass::HandleTaggedAction, TaggedAction.Value);
		}
	}
}

void AStdPlayerController::HandleTaggedAction(const FInputActionInstance& Instance, FGameplayTag InputTag)
{
	const FInputActionValue Value = Instance.GetValue();
	const ETriggerEvent TriggerEvent = Instance.GetTriggerEvent();
	OnInputTag.Broadcast(InputTag, Value, TriggerEvent);
	ReceiveInputTag(InputTag, Value, TriggerEvent);
}

void AStdPlayerController::HandlePauseAction()
{
	TogglePause();
}

void AStdPlayerController::TogglePause()
{
	if (bPausedByMenu)
	{
		ResumeGame();
	}
	else
	{
		PauseGame();
	}
}

bool AStdPlayerController::PauseGame()
{
	if (bPausedByMenu || !bCanPause || !IsLocalController())
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UStdFlowSubsystem* Flow = GameInstance ? GameInstance->GetSubsystem<UStdFlowSubsystem>() : nullptr;
	if (Flow && Flow->IsTransitioning())
	{
		UE_LOG(LogStdCore, Verbose, TEXT("Pause refused: a map transition is running."));
		return false;
	}

	if (!UGameplayStatics::SetGamePaused(this, true))
	{
		UE_LOG(LogStdCore, Warning, TEXT("Pause refused by the game mode."));
		return false;
	}
	bPausedByMenu = true;

	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	const TSubclassOf<UStdPauseMenuWidget> PauseMenuClass = StdSettings::LoadClass(Settings->PauseMenuWidgetClass,
		TEXT("PauseMenuWidgetClass"), StdCoreDefaults::PauseMenuWidget);
	PauseMenu = PauseMenuClass ? CreateWidget<UStdPauseMenuWidget>(this, PauseMenuClass) : nullptr;

	if (PauseMenu)
	{
		PauseMenu->ShowAsRoot(PauseMenuZOrder);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseMenu->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		PauseMenu->FocusDefaultWidget();
	}
	else
	{
		// Without a menu the pause still works: keep game input so the console and the pause keys stay reachable.
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	SetShowMouseCursor(true);
	FlushPressedKeys();
	CollectPauseResumeKeys();
	SetPauseKeyProcessorActive(true);

	UE_LOG(LogStdCore, Log, TEXT("Game paused%s."), PauseMenu ? TEXT("") : TEXT(" (no pause menu widget)"));
	OnPauseChanged.Broadcast(true);
	return true;
}

bool AStdPlayerController::ResumeGame()
{
	if (!bPausedByMenu)
	{
		return false;
	}

	SetPauseKeyProcessorActive(false);

	if (PauseMenu)
	{
		// Closes the whole stack (Settings included, which saves) before the game takes the input back.
		PauseMenu->CloseScreen();
		PauseMenu = nullptr;
	}

	UGameplayStatics::SetGamePaused(this, false);
	bPausedByMenu = false;

	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	FlushPressedKeys();

	UE_LOG(LogStdCore, Log, TEXT("Game resumed."));
	OnPauseChanged.Broadcast(false);
	return true;
}

void AStdPlayerController::HandlePauseBackKey()
{
	UStdScreenWidget* TopScreen = PauseMenu ? PauseMenu->GetTopScreen() : nullptr;
	if (TopScreen && TopScreen != PauseMenu)
	{
		TopScreen->HandleBackAction();
	}
	else
	{
		ResumeGame();
	}
}

void AStdPlayerController::HandlePauseResumeKey()
{
	ResumeGame();
}

bool AStdPlayerController::IsKeyCaptureActive() const
{
	const UStdInputSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UStdInputSubsystem>(GetLocalPlayer());
	return InputSubsystem && InputSubsystem->IsKeyCaptureActive();
}

void AStdPlayerController::CollectPauseResumeKeys()
{
	PauseResumeKeys.Reset();

	if (const UInputAction* PauseAction = StdInput::FindInputActionForTag(StdGameplayTags::InputTag_Pause))
	{
		if (const UEnhancedInputLocalPlayerSubsystem* EnhancedInput = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			PauseResumeKeys.Append(EnhancedInput->QueryKeysMappedToAction(PauseAction));
		}
	}

	PauseResumeKeys.Append(UStdProjectSettings::Get()->ReservedKeys);
	// The gamepad Start button works even when IA_Pause is missing.
	PauseResumeKeys.AddUnique(EKeys::Gamepad_Special_Right);
	PauseResumeKeys.RemoveAll([](const FKey& Key) { return !Key.IsValid() || UStdScreenWidget::IsBackKey(Key); });
}

void AStdPlayerController::SetPauseKeyProcessorActive(bool bActive)
{
	if (!FSlateApplication::IsInitialized())
	{
		PauseKeyProcessor.Reset();
		return;
	}

	if (bActive && !PauseKeyProcessor.IsValid())
	{
		PauseKeyProcessor = MakeShared<FStdPauseKeyProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(PauseKeyProcessor);
	}
	else if (!bActive && PauseKeyProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(PauseKeyProcessor);
		PauseKeyProcessor.Reset();
	}
}
