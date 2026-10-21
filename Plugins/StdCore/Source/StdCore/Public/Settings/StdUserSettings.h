#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StdUserSettings.generated.h"

/**
 * Player preferences owned by the plugin, saved in GameUserSettings.ini under [/Script/StdCore.StdUserSettings].
 * A plain config object instead of a UGameUserSettings subclass, so the host project does not need
 * GameUserSettingsClassName in its DefaultEngine.ini. Window mode and quality stay in UGameUserSettings.
 *
 * Only the class default object is used: UStdUserSettings::Get().
 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class STDCORE_API UStdUserSettings : public UObject
{
	GENERATED_BODY()

public:
	static UStdUserSettings* Get() { return GetMutableDefault<UStdUserSettings>(); }

	static constexpr float DefaultMasterVolume = 1.f;
	static constexpr float DefaultMusicVolume = 0.8f;
	static constexpr float DefaultSfxVolume = 1.f;

	/** All volumes are 0..1; values read from a hand-edited ini are clamped. */
	float GetMasterVolume() const { return FMath::Clamp(MasterVolume, 0.f, 1.f); }
	float GetMusicVolume() const { return FMath::Clamp(MusicVolume, 0.f, 1.f); }
	float GetSfxVolume() const { return FMath::Clamp(SfxVolume, 0.f, 1.f); }

	void SetMasterVolume(float Value) { MasterVolume = FMath::Clamp(Value, 0.f, 1.f); }
	void SetMusicVolume(float Value) { MusicVolume = FMath::Clamp(Value, 0.f, 1.f); }
	void SetSfxVolume(float Value) { SfxVolume = FMath::Clamp(Value, 0.f, 1.f); }

	void ResetToDefaults();

	/** Writes the values to GameUserSettings.ini. */
	void Save();

private:
	UPROPERTY(Config)
	float MasterVolume = DefaultMasterVolume;

	UPROPERTY(Config)
	float MusicVolume = DefaultMusicVolume;

	UPROPERTY(Config)
	float SfxVolume = DefaultSfxVolume;
};
