#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StdMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/**
 * Music that survives map changes and pause.
 *
 * The audio component is created with bPersistAcrossLevelTransition, so a track keeps playing through travel,
 * and as a UI sound, so it keeps playing while the game is paused. It plays in the Music Sound Class from
 * UStdProjectSettings even when the asset itself has no class assigned.
 */
UCLASS()
class STDCORE_API UStdMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;

	/**
	 * Plays Music, fading it in. A different track already playing crossfades out over the same time.
	 * Asking for the track that is already playing does nothing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Music")
	void PlayMusic(USoundBase* Music, float FadeInDuration = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Std Core|Music")
	void StopMusic(float FadeOutDuration = 1.f);

	/** The track currently playing or fading in; null when there is none. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Music")
	USoundBase* GetCurrentMusic() const;

private:
	void FadeOutAndRelease(UAudioComponent* Component, float FadeOutDuration);
	void HandleFadedOut(UAudioComponent* Component);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentComponent;

	/** Tracks fading out, kept referenced until they finish. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> FadingComponents;
};
