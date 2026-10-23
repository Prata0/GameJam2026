// Checks that the plugin's assets exist and match the specification in README.md ("Assets to create").
// The first test to look at after copying the plugin into another project.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Input/StdInputConfig.h"
#include "Settings/StdProjectSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "StdGameplayTags.h"
#include "UI/StdCreditsWidget.h"
#include "UI/StdKeyBindingRowWidget.h"
#include "UI/StdMainMenuWidget.h"
#include "UI/StdPauseMenuWidget.h"
#include "UI/StdSettingsWidget.h"
#include "UI/StdUITheme.h"

namespace StdAssetTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	template <typename T>
	T* LoadSetting(FAutomationTestBase& Test, const TSoftObjectPtr<T>& Setting, const TCHAR* Name)
	{
		T* Asset = Setting.LoadSynchronous();
		Test.TestNotNull(FString::Printf(TEXT("Setting %s (%s) loads"), Name, *Setting.ToString()), Asset);
		return Asset;
	}

	template <typename T>
	void LoadClassSetting(FAutomationTestBase& Test, const TSoftClassPtr<T>& Setting, const TCHAR* Name)
	{
		const UClass* Class = Setting.LoadSynchronous();
		Test.TestTrue(FString::Printf(TEXT("Setting %s (%s) loads as a %s"), Name, *Setting.ToString(), *T::StaticClass()->GetName()),
			Class && Class->IsChildOf(T::StaticClass()));
	}

	/** The plugin's own asset at its default path, whatever the project settings point to. */
	template <typename T>
	T* LoadDefault(FAutomationTestBase& Test, const TCHAR* Path)
	{
		T* Asset = LoadObject<T>(nullptr, Path);
		Test.TestNotNull(FString::Printf(TEXT("%s loads"), Path), Asset);
		return Asset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdSettingsDefaultsTest, "StdCore.Assets.SettingsDefaults", StdAssetTests::Flags)

bool FStdSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	using namespace StdAssetTests;
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();

	LoadSetting(*this, Settings->MainMenuMap, TEXT("MainMenuMap"));
	LoadSetting(*this, Settings->GameplayMap, TEXT("GameplayMap"));

	LoadSetting(*this, Settings->VolumesSoundMix, TEXT("VolumesSoundMix"));
	LoadSetting(*this, Settings->MasterSoundClass, TEXT("MasterSoundClass"));
	LoadSetting(*this, Settings->MusicSoundClass, TEXT("MusicSoundClass"));
	LoadSetting(*this, Settings->SfxSoundClass, TEXT("SfxSoundClass"));
	LoadSetting(*this, Settings->UiSoundClass, TEXT("UiSoundClass"));

	TestTrue(TEXT("At least one Input Config is set"), Settings->InputConfigs.Num() > 0);
	for (const TSoftObjectPtr<UStdInputConfig>& Config : Settings->InputConfigs)
	{
		LoadSetting(*this, Config, TEXT("InputConfigs"));
	}
	TestTrue(TEXT("At least one Mapping Context is set"), Settings->MappingContexts.Num() > 0);
	for (const FStdMappingContextSetting& Entry : Settings->MappingContexts)
	{
		LoadSetting(*this, Entry.MappingContext, TEXT("MappingContexts"));
	}

	LoadClassSetting(*this, Settings->MainMenuWidgetClass, TEXT("MainMenuWidgetClass"));
	LoadClassSetting(*this, Settings->PauseMenuWidgetClass, TEXT("PauseMenuWidgetClass"));
	LoadClassSetting(*this, Settings->SettingsWidgetClass, TEXT("SettingsWidgetClass"));
	LoadClassSetting(*this, Settings->KeyBindingRowWidgetClass, TEXT("KeyBindingRowWidgetClass"));
	LoadClassSetting(*this, Settings->CreditsWidgetClass, TEXT("CreditsWidgetClass"));

	// An empty theme is a valid choice (designer styles); a set one must load.
	if (!Settings->Theme.IsNull())
	{
		LoadSetting(*this, Settings->Theme, TEXT("Theme"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdAudioAssetsTest, "StdCore.Assets.Audio", StdAssetTests::Flags)

bool FStdAudioAssetsTest::RunTest(const FString& Parameters)
{
	using namespace StdAssetTests;

	USoundClass* Master = LoadDefault<USoundClass>(*this, StdCoreDefaults::MasterSoundClass);
	USoundClass* Music = LoadDefault<USoundClass>(*this, StdCoreDefaults::MusicSoundClass);
	USoundClass* Sfx = LoadDefault<USoundClass>(*this, StdCoreDefaults::SfxSoundClass);
	USoundClass* Ui = LoadDefault<USoundClass>(*this, StdCoreDefaults::UiSoundClass);
	USoundMix* Mix = LoadDefault<USoundMix>(*this, StdCoreDefaults::VolumesSoundMix);
	if (!Master || !Music || !Sfx || !Ui || !Mix)
	{
		return false;
	}

	TestNull(TEXT("SC_Master has no parent"), Master->ParentClass.Get());
	TestTrue(TEXT("SC_Music's parent is SC_Master"), Music->ParentClass.Get() == Master);
	TestTrue(TEXT("SC_SFX's parent is SC_Master"), Sfx->ParentClass.Get() == Master);
	TestTrue(TEXT("SC_UI's parent is SC_Master"), Ui->ParentClass.Get() == Master);

	TestTrue(TEXT("SC_Music plays while paused"), Music->Properties.bIsUISound);
	TestFalse(TEXT("SC_SFX stops while paused"), Sfx->Properties.bIsUISound);
	TestTrue(TEXT("SC_UI plays while paused"), Ui->Properties.bIsUISound);

	TestEqual(TEXT("SCM_Volumes has four adjusters"), Mix->SoundClassEffects.Num(), 4);
	for (USoundClass* SoundClass : { Master, Music, Sfx, Ui })
	{
		const FSoundClassAdjuster* Adjuster = Mix->SoundClassEffects.FindByPredicate([SoundClass](const FSoundClassAdjuster& Entry)
		{
			return Entry.SoundClassObject == SoundClass;
		});
		if (TestNotNull(FString::Printf(TEXT("SCM_Volumes adjusts %s"), *SoundClass->GetName()), Adjuster))
		{
			// The root is kept at 1 for itself only; each branch carries its volume down to the game's child classes.
			TestTrue(FString::Printf(TEXT("%s: Apply to Children is %s"), *SoundClass->GetName(), SoundClass != Master ? TEXT("on") : TEXT("off")),
				Adjuster->bApplyToChildren == (SoundClass != Master));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdInputAssetsTest, "StdCore.Assets.Input", StdAssetTests::Flags)

bool FStdInputAssetsTest::RunTest(const FString& Parameters)
{
	using namespace StdAssetTests;

	const UStdInputConfig* Config = LoadDefault<UStdInputConfig>(*this, StdCoreDefaults::InputConfig);
	const UInputMappingContext* Context = LoadDefault<UInputMappingContext>(*this, StdCoreDefaults::MappingContext);
	if (!Config || !Context)
	{
		return false;
	}

	struct FExpectedAction
	{
		FGameplayTag Tag;
		EInputActionValueType ValueType;
		bool bTriggerWhenPaused;
	};
	const FExpectedAction ExpectedActions[] = {
		{ StdGameplayTags::InputTag_Move, EInputActionValueType::Axis2D, false },
		{ StdGameplayTags::InputTag_Look, EInputActionValueType::Axis2D, false },
		{ StdGameplayTags::InputTag_Jump, EInputActionValueType::Boolean, false },
		{ StdGameplayTags::InputTag_Interact, EInputActionValueType::Boolean, false },
		{ StdGameplayTags::InputTag_Pause, EInputActionValueType::Boolean, true },
	};

	TestEqual(TEXT("DA_InputConfig_Core has five actions"), Config->InputActions.Num(), static_cast<int32>(UE_ARRAY_COUNT(ExpectedActions)));
	for (const FExpectedAction& Expected : ExpectedActions)
	{
		const UInputAction* Action = Config->FindInputActionForTag(Expected.Tag);
		if (TestNotNull(FString::Printf(TEXT("An action is paired with %s"), *Expected.Tag.ToString()), Action))
		{
			TestTrue(FString::Printf(TEXT("%s: value type"), *Action->GetName()), Action->ValueType == Expected.ValueType);
			TestTrue(FString::Printf(TEXT("%s: Trigger when Paused is %s"), *Action->GetName(), Expected.bTriggerWhenPaused ? TEXT("on") : TEXT("off")),
				Action->bTriggerWhenPaused == Expected.bTriggerWhenPaused);
		}
	}

	// The remappable entries, in IMC order: they are the rows of the Controls tab.
	TArray<FString> Remappable;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (Mapping.IsPlayerMappable())
		{
			Remappable.Add(FString::Printf(TEXT("%s/%s"), *Mapping.GetMappingName().ToString(), *Mapping.GetDisplayCategory().ToString()));
		}
	}
	TestEqual(TEXT("IMC_Default's remappable entries"), FString::Join(Remappable, TEXT(", ")),
		FString(TEXT("MoveForward/Movement, MoveBackward/Movement, MoveLeft/Movement, MoveRight/Movement, Jump/Actions, Interact/Actions")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
