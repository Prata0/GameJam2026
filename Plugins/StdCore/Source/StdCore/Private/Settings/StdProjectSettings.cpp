#include "Settings/StdProjectSettings.h"

#include "Engine/World.h"
#include "Input/StdInputConfig.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "StdLog.h"
#include "UI/StdCreditsWidget.h"
#include "UI/StdKeyBindingRowWidget.h"
#include "UI/StdMainMenuWidget.h"
#include "UI/StdPauseMenuWidget.h"
#include "UI/StdSettingsWidget.h"

UStdProjectSettings::UStdProjectSettings()
{
	MainMenuMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(StdCoreDefaults::MainMenuMap));
	GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(StdCoreDefaults::GameplayMap));

	VolumesSoundMix = TSoftObjectPtr<USoundMix>(FSoftObjectPath(StdCoreDefaults::VolumesSoundMix));
	MasterSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(StdCoreDefaults::MasterSoundClass));
	MusicSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(StdCoreDefaults::MusicSoundClass));
	SfxSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(StdCoreDefaults::SfxSoundClass));
	UiSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(StdCoreDefaults::UiSoundClass));

	InputConfigs.Add(TSoftObjectPtr<UStdInputConfig>(FSoftObjectPath(StdCoreDefaults::InputConfig)));

	FStdMappingContextSetting DefaultContext;
	DefaultContext.MappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(StdCoreDefaults::MappingContext));
	DefaultContext.Priority = 0;
	MappingContexts.Add(DefaultContext);

	// Escape always pauses, so the player can never lock themselves out of the menu.
	ReservedKeys.Add(EKeys::Escape);

	MainMenuWidgetClass = TSoftClassPtr<UStdMainMenuWidget>(FSoftObjectPath(StdCoreDefaults::MainMenuWidget));
	PauseMenuWidgetClass = TSoftClassPtr<UStdPauseMenuWidget>(FSoftObjectPath(StdCoreDefaults::PauseMenuWidget));
	SettingsWidgetClass = TSoftClassPtr<UStdSettingsWidget>(FSoftObjectPath(StdCoreDefaults::SettingsWidget));
	KeyBindingRowWidgetClass = TSoftClassPtr<UStdKeyBindingRowWidget>(FSoftObjectPath(StdCoreDefaults::KeyBindingRowWidget));
	CreditsWidgetClass = TSoftClassPtr<UStdCreditsWidget>(FSoftObjectPath(StdCoreDefaults::CreditsWidget));
}

UObject* StdSettings::LoadSettingAsset(const FSoftObjectPath& Path, const FString& SettingName, const TCHAR* ExpectedPath)
{
	const FString WarnKey = SettingName + TEXT("|") + Path.ToString();

	if (Path.IsNull())
	{
		StdLog::WarnOnce(WarnKey, FString::Printf(
			TEXT("Std Core setting '%s' is empty (expected '%s'). Continuing without it."), *SettingName, ExpectedPath));
		return nullptr;
	}

	// Checked first so a missing asset costs one warning of ours instead of an engine load error every call.
	if (!FPackageName::DoesPackageExist(Path.GetLongPackageName()))
	{
		const bool bIsPluginDefault = Path.ToString().Equals(ExpectedPath);
		StdLog::WarnOnce(WarnKey, bIsPluginDefault
			? FString::Printf(TEXT("Std Core setting '%s': the asset '%s' does not exist yet. Continuing without it."),
				*SettingName, ExpectedPath)
			: FString::Printf(TEXT("Std Core setting '%s' points to '%s', which does not exist (plugin default: '%s'). Continuing without it."),
				*SettingName, *Path.ToString(), ExpectedPath));
		return nullptr;
	}

	UObject* Object = Path.TryLoad();
	if (!Object)
	{
		StdLog::WarnOnce(WarnKey, FString::Printf(
			TEXT("Std Core setting '%s' could not load '%s' (expected '%s'). Continuing without it."),
			*SettingName, *Path.ToString(), ExpectedPath));
	}
	return Object;
}
