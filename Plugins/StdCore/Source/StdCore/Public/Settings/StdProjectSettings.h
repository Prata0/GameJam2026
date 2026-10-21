#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Templates/SubclassOf.h"
#include "StdProjectSettings.generated.h"

class UInputMappingContext;
class USoundClass;
class USoundMix;
class UStdCreditsWidget;
class UStdInputConfig;
class UStdKeyBindingRowWidget;
class UStdMainMenuWidget;
class UStdPauseMenuWidget;
class UStdSettingsWidget;
class UWorld;

/** Asset paths the plugin points to by default. They live in the plugin content (mount point /StdCore/). */
namespace StdCoreDefaults
{
	inline const TCHAR* MainMenuMap = TEXT("/StdCore/Maps/L_MainMenu.L_MainMenu");
	inline const TCHAR* GameplayMap = TEXT("/StdCore/Maps/L_TestGameplay.L_TestGameplay");

	inline const TCHAR* VolumesSoundMix = TEXT("/StdCore/Audio/SCM_Volumes.SCM_Volumes");
	inline const TCHAR* MasterSoundClass = TEXT("/StdCore/Audio/SC_Master.SC_Master");
	inline const TCHAR* MusicSoundClass = TEXT("/StdCore/Audio/SC_Music.SC_Music");
	inline const TCHAR* SfxSoundClass = TEXT("/StdCore/Audio/SC_SFX.SC_SFX");
	inline const TCHAR* UiSoundClass = TEXT("/StdCore/Audio/SC_UI.SC_UI");

	inline const TCHAR* InputConfig = TEXT("/StdCore/Input/DA_InputConfig_Core.DA_InputConfig_Core");
	inline const TCHAR* MappingContext = TEXT("/StdCore/Input/IMC_Default.IMC_Default");

	inline const TCHAR* MainMenuWidget = TEXT("/StdCore/UI/WBP_MainMenu.WBP_MainMenu_C");
	inline const TCHAR* PauseMenuWidget = TEXT("/StdCore/UI/WBP_PauseMenu.WBP_PauseMenu_C");
	inline const TCHAR* SettingsWidget = TEXT("/StdCore/UI/WBP_Settings.WBP_Settings_C");
	inline const TCHAR* KeyBindingRowWidget = TEXT("/StdCore/UI/WBP_KeyBindingRow.WBP_KeyBindingRow_C");
	inline const TCHAR* CreditsWidget = TEXT("/StdCore/UI/WBP_Credits.WBP_Credits_C");
}

/** An Input Mapping Context added to every local player, with its priority. */
USTRUCT(BlueprintType)
struct STDCORE_API FStdMappingContextSetting
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> MappingContext;

	/** Higher priority contexts win when two contexts map the same key. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	int32 Priority = 0;
};

/**
 * Per-project settings of the Std Core plugin: Project Settings > Game > Std Core.
 * Saved in the host project's DefaultGame.ini. Every default points to an asset inside the plugin,
 * so the plugin works as soon as it is copied, and a project can replace any of them without code changes.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Std Core"))
class STDCORE_API UStdProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UStdProjectSettings();

	static const UStdProjectSettings* Get() { return GetDefault<UStdProjectSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// Maps

	/** Map opened by "Main menu" and by UStdFlowSubsystem::ReturnToMainMenu. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> MainMenuMap;

	/** Map opened by the main menu's "Play" button. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TSoftObjectPtr<UWorld> GameplayMap;

	// Transitions

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutDuration = 0.4f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInDuration = 0.4f;

	/** Time spent fully faded after the new map is loaded, so the first frames (streaming, hitches) stay hidden. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInDelay = 0.15f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FLinearColor FadeColor = FLinearColor::Black;

	/** The first map of the game appears with a fade in. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	bool bFadeInOnStartup = true;

	/** If the new map has not arrived after this time, the fade is removed, input is unblocked and an error is logged. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "1.0", Units = "s"))
	float TravelTimeoutSeconds = 30.f;

	// Audio

	/** Sound Mix with one adjuster per Sound Class below. The volume sliders override its adjusters. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundMix> VolumesSoundMix;

	/** Root of the hierarchy. Kept at 1: never assign sounds to it directly. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TSoftObjectPtr<USoundClass> UiSoundClass;

	// Input

	/** Input Configs searched, in order, when code binds or queries an input tag. Add the game's configs next to the plugin's. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<TSoftObjectPtr<UStdInputConfig>> InputConfigs;

	/** Mapping contexts added to every local player and registered for key remapping. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FStdMappingContextSetting> MappingContexts;

	/** Keys the player can never assign while remapping. Gamepad keys and axes are always refused as well. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TArray<FKey> ReservedKeys;

	// UI

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UStdMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UStdPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UStdSettingsWidget> SettingsWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UStdKeyBindingRowWidget> KeyBindingRowWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UStdCreditsWidget> CreditsWidgetClass;
};

/**
 * Loading of the assets named in UStdProjectSettings. A setting that is empty, points to a package that
 * does not exist or does not load logs one warning (setting name, current and expected path) and returns null.
 * Never asserts: callers continue without the asset.
 */
namespace StdSettings
{
	STDCORE_API UObject* LoadSettingAsset(const FSoftObjectPath& Path, const FString& SettingName, const TCHAR* ExpectedPath);

	template <typename T>
	T* LoadAsset(const TSoftObjectPtr<T>& Ptr, const FString& SettingName, const TCHAR* ExpectedPath)
	{
		UObject* Object = LoadSettingAsset(Ptr.ToSoftObjectPath(), SettingName, ExpectedPath);
		return Cast<T>(Object);
	}

	template <typename T>
	TSubclassOf<T> LoadClass(const TSoftClassPtr<T>& Ptr, const FString& SettingName, const TCHAR* ExpectedPath)
	{
		UClass* Class = Cast<UClass>(LoadSettingAsset(Ptr.ToSoftObjectPath(), SettingName, ExpectedPath));
		return (Class && Class->IsChildOf(T::StaticClass())) ? Class : nullptr;
	}
}
