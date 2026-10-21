#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StdSettingsSubsystem.generated.h"

struct FActorsInitializedParams;

UENUM(BlueprintType)
enum class EStdVolumeChannel : uint8
{
	Master,
	Music,
	Sfx
};

/**
 * Volumes, window mode and graphics quality.
 *
 * Volumes are stored in UStdUserSettings and applied through the Sound Mix in UStdProjectSettings: every change
 * is audible at once, SaveSettings writes it to disk. The mix is applied again on every new world, because a map
 * load can drop it. Window mode and quality use the engine's UGameUserSettings.
 */
UCLASS()
class STDCORE_API UStdSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Audio

	UFUNCTION(BlueprintPure, Category = "Std Core|Settings")
	float GetVolume(EStdVolumeChannel Channel) const;

	/** Clamped to 0..1 and audible immediately. Not saved until SaveSettings. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Settings")
	void SetVolume(EStdVolumeChannel Channel, float Volume);

	/** Applies the stored volumes to the Sound Mix. Safe to call at any time; does nothing without a world or a mix. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Settings")
	void ApplyVolumes();

	// Video

	UFUNCTION(BlueprintPure, Category = "Std Core|Settings")
	EWindowMode::Type GetWindowMode() const;

	/** Applied immediately. Fullscreen modes use the desktop resolution; windowed mode shrinks a desktop-sized window. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Settings")
	void SetWindowMode(EWindowMode::Type WindowMode);

	/** 0 Low, 1 Medium, 2 High, 3 Epic; -1 when the scalability groups do not match a single level. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Settings")
	int32 GetQualityLevel() const;

	/** Applied immediately. Clamped to 0..3. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Settings")
	void SetQualityLevel(int32 Level);

	/** Writes volumes (GameUserSettings.ini, StdUserSettings section) and video settings to disk. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Settings")
	void SaveSettings();

	/** Quality used when nothing valid is saved: Medium. Fixed, so every log can state what the game started on. */
	static constexpr int32 DefaultQualityLevel = 1;

private:
	void HandleWorldInitialized(const FActorsInitializedParams& Params);
	void ApplyVolumesInternal(bool bPushMix);
	void LoadStartupVideoSettings();
	void ApplyStartupVideoSettings();

	FDelegateHandle WorldInitializedHandle;
	FDelegateHandle EngineInitCompleteHandle;
};
