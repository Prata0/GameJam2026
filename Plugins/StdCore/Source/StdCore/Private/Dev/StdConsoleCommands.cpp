// Console commands for testing the plugin without menus or assets. Not compiled in Shipping builds.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Flow/StdFlowSubsystem.h"
#include "Framework/StdPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Input/StdInputSubsystem.h"
#include "Misc/PackageName.h"
#include "Settings/StdSettingsSubsystem.h"
#include "StdGameplayTags.h"
#include "StdLog.h"

namespace StdConsoleCommands
{
	UGameInstance* GetGameInstance(UWorld* World)
	{
		return World ? World->GetGameInstance() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs Travel(
		TEXT("Std.Travel"),
		TEXT("Std.Travel <map package, e.g. /Engine/Maps/Entry>: travel with the Std Core fade."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* GameInstance = GetGameInstance(World);
			UStdFlowSubsystem* Flow = GameInstance ? GameInstance->GetSubsystem<UStdFlowSubsystem>() : nullptr;
			if (!Flow || Args.IsEmpty())
			{
				UE_LOG(LogStdCore, Warning, TEXT("Usage: Std.Travel /Path/To/Map (in a running game)."));
				return;
			}

			FString Path = Args[0];
			if (!Path.Contains(TEXT(".")))
			{
				Path = Path + TEXT(".") + FPackageName::GetShortName(Path);
			}
			Flow->TravelToLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(Path)));
		}));

	FAutoConsoleCommandWithWorld MainMenu(
		TEXT("Std.MainMenu"),
		TEXT("Travel to the main menu map of the Std Core settings."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UGameInstance* GameInstance = GetGameInstance(World);
			if (UStdFlowSubsystem* Flow = GameInstance ? GameInstance->GetSubsystem<UStdFlowSubsystem>() : nullptr)
			{
				Flow->ReturnToMainMenu();
			}
		}));

	FAutoConsoleCommandWithWorld TogglePause(
		TEXT("Std.TogglePause"),
		TEXT("Pause or resume through the first AStdPlayerController."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			AStdPlayerController* PlayerController = World ? Cast<AStdPlayerController>(World->GetFirstPlayerController()) : nullptr;
			if (PlayerController)
			{
				PlayerController->TogglePause();
			}
			else
			{
				UE_LOG(LogStdCore, Warning, TEXT("Std.TogglePause: the first player controller is not an AStdPlayerController."));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs Volume(
		TEXT("Std.Volume"),
		TEXT("Std.Volume <master|music|sfx> <0..1>: set and save a volume."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* GameInstance = GetGameInstance(World);
			UStdSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UStdSettingsSubsystem>() : nullptr;
			if (!Settings || Args.Num() < 2)
			{
				UE_LOG(LogStdCore, Warning, TEXT("Usage: Std.Volume master|music|sfx 0..1 (in a running game)."));
				return;
			}

			EStdVolumeChannel Channel = EStdVolumeChannel::Master;
			if (Args[0].Equals(TEXT("music"), ESearchCase::IgnoreCase))
			{
				Channel = EStdVolumeChannel::Music;
			}
			else if (Args[0].Equals(TEXT("sfx"), ESearchCase::IgnoreCase))
			{
				Channel = EStdVolumeChannel::Sfx;
			}

			Settings->SetVolume(Channel, FCString::Atof(*Args[1]));
			Settings->SaveSettings();
			UE_LOG(LogStdCore, Log, TEXT("Volume %s = %.2f (saved)."), *Args[0], Settings->GetVolume(Channel));
		}));

	FAutoConsoleCommandWithWorld DumpInput(
		TEXT("Std.DumpInput"),
		TEXT("Log the remappable keys and the key of each Std Core input tag."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
			const UStdInputSubsystem* Input = PlayerController ? ULocalPlayer::GetSubsystem<UStdInputSubsystem>(PlayerController->GetLocalPlayer()) : nullptr;
			if (!Input)
			{
				UE_LOG(LogStdCore, Warning, TEXT("Std.DumpInput: no local player."));
				return;
			}

			const TArray<FStdKeyBindingRow> Rows = Input->GetRemappableKeys();
			UE_LOG(LogStdCore, Log, TEXT("%d remappable keys:"), Rows.Num());
			for (const FStdKeyBindingRow& Row : Rows)
			{
				UE_LOG(LogStdCore, Log, TEXT("  [%s] %s (%s): %s (default %s)"), *Row.Category.ToString(), *Row.DisplayName.ToString(),
					*Row.MappingName.ToString(), *Row.CurrentKey.ToString(), *Row.DefaultKey.ToString());
			}

			for (const FGameplayTag& Tag : { StdGameplayTags::InputTag_Move.GetTag(), StdGameplayTags::InputTag_Look.GetTag(),
				StdGameplayTags::InputTag_Jump.GetTag(), StdGameplayTags::InputTag_Interact.GetTag(), StdGameplayTags::InputTag_Pause.GetTag() })
			{
				UE_LOG(LogStdCore, Log, TEXT("  %s -> %s"), *Tag.ToString(), *Input->GetKeyDisplayTextForTag(Tag).ToString());
			}
		}));
}

#endif // !UE_BUILD_SHIPPING
