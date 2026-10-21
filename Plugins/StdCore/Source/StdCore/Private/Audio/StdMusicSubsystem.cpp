#include "Audio/StdMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/StdProjectSettings.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "StdLog.h"

bool UStdMusicSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UStdMusicSubsystem::Deinitialize()
{
	if (CurrentComponent)
	{
		CurrentComponent->Stop();
		CurrentComponent = nullptr;
	}
	for (UAudioComponent* Component : FadingComponents)
	{
		if (Component)
		{
			Component->OnAudioFinishedNative.RemoveAll(this);
			Component->Stop();
		}
	}
	FadingComponents.Reset();
	Super::Deinitialize();
}

void UStdMusicSubsystem::PlayMusic(USoundBase* Music, float FadeInDuration)
{
	if (!Music)
	{
		UE_LOG(LogStdCore, Warning, TEXT("PlayMusic called without a sound; ignored."));
		return;
	}

	if (CurrentComponent && CurrentComponent->GetSound() == Music && CurrentComponent->IsPlaying())
	{
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogStdCore, Warning, TEXT("PlayMusic(%s): no world yet; ignored."), *GetNameSafe(Music));
		return;
	}

	FadeInDuration = FMath::Max(FadeInDuration, 0.f);

	UAudioComponent* NewComponent = UGameplayStatics::CreateSound2D(World, Music, 1.f, 1.f, 0.f, nullptr,
		/*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
	if (!NewComponent)
	{
		// Audio disabled (-nosound, dedicated server, world without audio): nothing to play.
		return;
	}

	NewComponent->bIsUISound = true;
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	if (USoundClass* MusicClass = StdSettings::LoadAsset(Settings->MusicSoundClass, TEXT("MusicSoundClass"), StdCoreDefaults::MusicSoundClass))
	{
		NewComponent->SoundClassOverride = MusicClass;
	}
	NewComponent->FadeIn(FadeInDuration);

	if (CurrentComponent)
	{
		FadeOutAndRelease(CurrentComponent, FadeInDuration);
	}
	CurrentComponent = NewComponent;
}

void UStdMusicSubsystem::StopMusic(float FadeOutDuration)
{
	if (CurrentComponent)
	{
		FadeOutAndRelease(CurrentComponent, FMath::Max(FadeOutDuration, 0.f));
		CurrentComponent = nullptr;
	}
}

USoundBase* UStdMusicSubsystem::GetCurrentMusic() const
{
	return CurrentComponent ? CurrentComponent->GetSound() : nullptr;
}

void UStdMusicSubsystem::FadeOutAndRelease(UAudioComponent* Component, float FadeOutDuration)
{
	if (!Component)
	{
		return;
	}

	if (!Component->IsPlaying() || FadeOutDuration <= 0.f)
	{
		Component->Stop();
		return;
	}

	FadingComponents.Add(Component);
	Component->OnAudioFinishedNative.AddUObject(this, &ThisClass::HandleFadedOut);
	Component->FadeOut(FadeOutDuration, 0.f);
}

void UStdMusicSubsystem::HandleFadedOut(UAudioComponent* Component)
{
	if (Component)
	{
		Component->OnAudioFinishedNative.RemoveAll(this);
	}
	FadingComponents.Remove(Component);
}
