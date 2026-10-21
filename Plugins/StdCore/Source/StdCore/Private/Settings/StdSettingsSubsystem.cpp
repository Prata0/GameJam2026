#include "Settings/StdSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Scalability.h"
#include "Settings/StdProjectSettings.h"
#include "Settings/StdUserSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "StdLog.h"

void UStdSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::HandleWorldInitialized);

	// In the editor the scalability belongs to the editor viewport: only a game process restores the saved video settings.
	if (!GIsEditor)
	{
		LoadStartupVideoSettings();
	}

	ApplyVolumesInternal(/*bPushMix*/ true);
}

void UStdSettingsSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedHandle);
	FCoreDelegates::OnFEngineLoopInitComplete.Remove(EngineInitCompleteHandle);
	Super::Deinitialize();
}

void UStdSettingsSubsystem::HandleWorldInitialized(const FActorsInitializedParams& Params)
{
	if (Params.World && Params.World->IsGameWorld() && Params.World->GetGameInstance() == GetGameInstance())
	{
		// A map load can drop the active sound mixes: push the mix again with the stored volumes.
		ApplyVolumesInternal(/*bPushMix*/ true);
	}
}

// Audio

float UStdSettingsSubsystem::GetVolume(EStdVolumeChannel Channel) const
{
	const UStdUserSettings* UserSettings = UStdUserSettings::Get();
	switch (Channel)
	{
	case EStdVolumeChannel::Master: return UserSettings->GetMasterVolume();
	case EStdVolumeChannel::Music: return UserSettings->GetMusicVolume();
	case EStdVolumeChannel::Sfx: return UserSettings->GetSfxVolume();
	}
	return 1.f;
}

void UStdSettingsSubsystem::SetVolume(EStdVolumeChannel Channel, float Volume)
{
	UStdUserSettings* UserSettings = UStdUserSettings::Get();
	switch (Channel)
	{
	case EStdVolumeChannel::Master: UserSettings->SetMasterVolume(Volume); break;
	case EStdVolumeChannel::Music: UserSettings->SetMusicVolume(Volume); break;
	case EStdVolumeChannel::Sfx: UserSettings->SetSfxVolume(Volume); break;
	}

	// The mix is already active: changing its overrides is enough, pushing it again would only add a reference.
	ApplyVolumesInternal(/*bPushMix*/ false);
}

void UStdSettingsSubsystem::ApplyVolumes()
{
	ApplyVolumesInternal(/*bPushMix*/ true);
}

void UStdSettingsSubsystem::ApplyVolumesInternal(bool bPushMix)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	USoundMix* Mix = StdSettings::LoadAsset(Settings->VolumesSoundMix, TEXT("VolumesSoundMix"), StdCoreDefaults::VolumesSoundMix);
	if (!Mix)
	{
		return;
	}

	const UStdUserSettings* UserSettings = UStdUserSettings::Get();
	const float Master = UserSettings->GetMasterVolume();

	// One explicit formula per branch, so the master is applied exactly once whatever the hierarchy does:
	// the root stays at 1 and each branch carries Master x its own volume. The branches apply to their children,
	// so a Sound Class the game adds under SC_Music, SC_SFX or SC_UI follows its branch without new adjusters.
	auto ApplyClass = [World, Mix](USoundClass* SoundClass, float Volume, bool bApplyToChildren)
	{
		if (SoundClass)
		{
			UGameplayStatics::SetSoundMixClassOverride(World, Mix, SoundClass, Volume, 1.f, 0.f, bApplyToChildren);
		}
	};

	ApplyClass(StdSettings::LoadAsset(Settings->MasterSoundClass, TEXT("MasterSoundClass"), StdCoreDefaults::MasterSoundClass),
		1.f, /*bApplyToChildren*/ false);
	ApplyClass(StdSettings::LoadAsset(Settings->MusicSoundClass, TEXT("MusicSoundClass"), StdCoreDefaults::MusicSoundClass),
		UserSettings->GetMusicVolume() * Master, /*bApplyToChildren*/ true);
	ApplyClass(StdSettings::LoadAsset(Settings->SfxSoundClass, TEXT("SfxSoundClass"), StdCoreDefaults::SfxSoundClass),
		UserSettings->GetSfxVolume() * Master, /*bApplyToChildren*/ true);
	ApplyClass(StdSettings::LoadAsset(Settings->UiSoundClass, TEXT("UiSoundClass"), StdCoreDefaults::UiSoundClass),
		Master, /*bApplyToChildren*/ true);

	if (bPushMix)
	{
		UGameplayStatics::PushSoundMixModifier(World, Mix);
	}
}

// Video

EWindowMode::Type UStdSettingsSubsystem::GetWindowMode() const
{
	const UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return GameSettings ? GameSettings->GetFullscreenMode() : EWindowMode::Windowed;
}

void UStdSettingsSubsystem::SetWindowMode(EWindowMode::Type WindowMode)
{
	UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GameSettings)
	{
		return;
	}

	const FIntPoint Desktop = GameSettings->GetDesktopResolution();
	if (WindowMode == EWindowMode::Windowed)
	{
		// A window as large as the desktop looks like nothing happened: shrink it to two thirds.
		const FIntPoint Current = GameSettings->GetScreenResolution();
		if (Current.X <= 0 || Current.Y <= 0 || Current.X >= Desktop.X || Current.Y >= Desktop.Y)
		{
			GameSettings->SetScreenResolution(FIntPoint(Desktop.X * 2 / 3, Desktop.Y * 2 / 3));
		}
	}
	else if (Desktop.X > 0 && Desktop.Y > 0)
	{
		GameSettings->SetScreenResolution(Desktop);
	}

	GameSettings->SetFullscreenMode(WindowMode);
	GameSettings->ApplyResolutionSettings(/*bCheckForCommandLineOverrides*/ false);
	GameSettings->ConfirmVideoMode();
	UE_LOG(LogStdCore, Log, TEXT("Window mode set to %s."), LexToString(WindowMode));
}

int32 UStdSettingsSubsystem::GetQualityLevel() const
{
	const UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return GameSettings ? GameSettings->GetOverallScalabilityLevel() : DefaultQualityLevel;
}

void UStdSettingsSubsystem::SetQualityLevel(int32 Level)
{
	UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GameSettings)
	{
		return;
	}

	GameSettings->SetOverallScalabilityLevel(FMath::Clamp(Level, 0, 3));
	GameSettings->ApplyNonResolutionSettings();
	UE_LOG(LogStdCore, Log, TEXT("Quality level set to %d."), GameSettings->GetOverallScalabilityLevel());
}

void UStdSettingsSubsystem::SaveSettings()
{
	UStdUserSettings::Get()->Save();
	if (UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GameSettings->SaveSettings();
	}
}

void UStdSettingsSubsystem::LoadStartupVideoSettings()
{
	UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GameSettings)
	{
		return;
	}

	// UGameUserSettings::LoadSettings does not read scalability from the ini: it copies the live CVars and assumes
	// Scalability::LoadState already ran. Load the saved state into the CVars first, then read it back.
	Scalability::LoadState(*GGameUserSettingsIni);
	GameSettings->LoadSettings();

	// The quality selector offers four presets: a mixed ("Custom", -1) state has no entry, so snap it to the default.
	// Set only: saving now would write the live CVars, which still hold the mixed state.
	if (GameSettings->GetOverallScalabilityLevel() < 0)
	{
		UE_LOG(LogStdCore, Warning, TEXT("Saved quality matches no single level; using the default level %d."), DefaultQualityLevel);
		GameSettings->SetOverallScalabilityLevel(DefaultQualityLevel);
	}

	// ApplyNonResolutionSettings skips scalability until the engine reports itself initialized, which happens after
	// the game instance starts: apply once the engine loop has finished initializing.
	EngineInitCompleteHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &ThisClass::ApplyStartupVideoSettings);
}

void UStdSettingsSubsystem::ApplyStartupVideoSettings()
{
	FCoreDelegates::OnFEngineLoopInitComplete.Remove(EngineInitCompleteHandle);

	if (UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GameSettings->ApplyNonResolutionSettings();
		GameSettings->SaveSettings();
		UE_LOG(LogStdCore, Log, TEXT("Startup video settings applied: quality %d, window mode %s."),
			GameSettings->GetOverallScalabilityLevel(), LexToString(GameSettings->GetFullscreenMode()));
	}
}
