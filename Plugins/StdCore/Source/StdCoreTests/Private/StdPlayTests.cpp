// Play In Editor tests: map transitions, pause and key remapping, through the same calls the menus use.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Flow/StdFlowSubsystem.h"
#include "Framework/StdPlayerController.h"
#include "Input/StdInputSubsystem.h"
#include "Settings/StdProjectSettings.h"
#include "StdGameplayTags.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/StdMainMenuWidget.h"

namespace StdPlayTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	UWorld* GetPlayWorld()
	{
		return GEditor ? GEditor->PlayWorld.Get() : nullptr;
	}

	UStdFlowSubsystem* GetFlow()
	{
		const UWorld* World = GetPlayWorld();
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UStdFlowSubsystem>() : nullptr;
	}

	APlayerController* GetPlayerController()
	{
		const UWorld* World = GetPlayWorld();
		return World ? World->GetFirstPlayerController() : nullptr;
	}

	UStdInputSubsystem* GetInput()
	{
		const APlayerController* PlayerController = GetPlayerController();
		return PlayerController ? ULocalPlayer::GetSubsystem<UStdInputSubsystem>(PlayerController->GetLocalPlayer()) : nullptr;
	}

	bool IsOnMap(const TCHAR* MapName)
	{
		const UWorld* World = GetPlayWorld();
		const UStdFlowSubsystem* Flow = GetFlow();
		return World && World->GetMapName().EndsWith(MapName) && Flow && !Flow->IsTransitioning() && GetPlayerController();
	}

	/** Queues a step that runs once. */
	void Step(TFunction<void()> Action)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Action]()
		{
			Action();
			return true;
		}));
	}

	/** Queues a wait for Condition; after TimeoutSeconds the test fails and the next steps run anyway. */
	void WaitUntil(FAutomationTestBase* Test, const FString& What, double TimeoutSeconds, TFunction<bool()> Condition)
	{
		TSharedRef<double> StartTime = MakeShared<double>(0.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Test, What, TimeoutSeconds, Condition, StartTime]()
		{
			if (*StartTime == 0.0)
			{
				*StartTime = FPlatformTime::Seconds();
			}
			if (Condition())
			{
				return true;
			}
			if (FPlatformTime::Seconds() - *StartTime > TimeoutSeconds)
			{
				Test->AddError(FString::Printf(TEXT("Timed out after %.0f s waiting for %s."), TimeoutSeconds, *What));
				return true;
			}
			return false;
		}));
	}

	void StartPlayOnMap(FAutomationTestBase* Test, const TSoftObjectPtr<UWorld>& Map, const TCHAR* MapName)
	{
		FAutomationEditorCommonUtils::LoadMap(Map.ToSoftObjectPath().GetLongPackageName());
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		WaitUntil(Test, FString::Printf(TEXT("%s to start and fade in"), MapName), 20.0, [MapName]() { return IsOnMap(MapName); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdFlowPlayTest, "StdCore.Flow.Play", StdPlayTests::Flags)

bool FStdFlowPlayTest::RunTest(const FString& Parameters)
{
	using namespace StdPlayTests;
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();

	// Logged on purpose by the refused travel below.
	AddExpectedError(TEXT("does not exist; nothing happens"), EAutomationExpectedErrorFlags::Contains, 1);

	StartPlayOnMap(this, Settings->MainMenuMap, TEXT("L_MainMenu"));

	Step([this]()
	{
		TArray<UUserWidget*> Menus;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetPlayWorld(), Menus, UStdMainMenuWidget::StaticClass(), false);
		TestEqual(TEXT("The main menu is shown"), Menus.Num(), 1);

		UStdFlowSubsystem* Flow = GetFlow();
		if (TestNotNull(TEXT("Flow subsystem"), Flow))
		{
			TestFalse(TEXT("A map that does not exist is refused"),
				Flow->TravelToLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/StdCore/Maps/L_DoesNotExist.L_DoesNotExist")))));
			TestFalse(TEXT("The refused travel starts no transition"), Flow->IsTransitioning());
			TestTrue(TEXT("Start Gameplay starts a travel"), Flow->StartGameplay());
			TestFalse(TEXT("A second request during the transition is ignored"), Flow->StartGameplay());
		}
	});
	WaitUntil(this, TEXT("L_TestGameplay to load and fade in"), 20.0, []() { return IsOnMap(TEXT("L_TestGameplay")); });

	Step([this]()
	{
		AStdPlayerController* PlayerController = Cast<AStdPlayerController>(GetPlayerController());
		if (!TestNotNull(TEXT("The gameplay map uses AStdPlayerController"), PlayerController))
		{
			return;
		}
		TestTrue(TEXT("Pause Game succeeds"), PlayerController->PauseGame());
		TestTrue(TEXT("The world is paused"), GetPlayWorld()->IsPaused());
		TestNotNull(TEXT("The pause menu is shown"), PlayerController->GetPauseMenu());
		TestFalse(TEXT("Pausing twice is refused"), PlayerController->PauseGame());
		TestTrue(TEXT("Resume Game succeeds"), PlayerController->ResumeGame());
		TestFalse(TEXT("The world runs again"), GetPlayWorld()->IsPaused());
		TestNull(TEXT("The pause menu is closed"), PlayerController->GetPauseMenu());

		UStdFlowSubsystem* Flow = GetFlow();
		TestTrue(TEXT("Return To Main Menu starts a travel"), Flow && Flow->ReturnToMainMenu());
	});
	WaitUntil(this, TEXT("L_MainMenu to load again"), 20.0, []() { return IsOnMap(TEXT("L_MainMenu")); });

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdInputRemapTest, "StdCore.Input.Remap", StdPlayTests::Flags)

bool FStdInputRemapTest::RunTest(const FString& Parameters)
{
	using namespace StdPlayTests;
	static const FName Jump(TEXT("Jump"));
	const EPlayerMappableKeySlot Slot = EPlayerMappableKeySlot::First;

	StartPlayOnMap(this, UStdProjectSettings::Get()->GameplayMap, TEXT("L_TestGameplay"));

	Step([this, Slot]()
	{
		UStdInputSubsystem* Input = GetInput();
		if (!TestNotNull(TEXT("Std input subsystem"), Input))
		{
			return;
		}

		// Start from the defaults whatever the player saved before.
		Input->ResetKeysToDefaults();

		FText Reason;
		TestFalse(TEXT("Escape is reserved"), Input->CanAssignKey(EKeys::Escape, Reason));
		TestFalse(TEXT("Gamepad buttons are refused"), Input->CanAssignKey(EKeys::Gamepad_FaceButton_Bottom, Reason));
		TestFalse(TEXT("Mouse axes are refused"), Input->CanAssignKey(EKeys::MouseX, Reason));
		TestTrue(TEXT("A free keyboard key is accepted"), Input->CanAssignKey(EKeys::F, Reason));

		FText Message;
		TestTrue(TEXT("Jump to E swaps with Interact"), Input->RemapKey(Jump, Slot, EKeys::E, Message) == EStdRemapResult::Swapped);
		TestFalse(TEXT("A swap explains itself"), Message.IsEmpty());
		TestTrue(TEXT("Assigning the same key again changes nothing"), Input->RemapKey(Jump, Slot, EKeys::E, Message) == EStdRemapResult::Unchanged);
		TestTrue(TEXT("A reserved key is rejected"), Input->RemapKey(Jump, Slot, EKeys::Escape, Message) == EStdRemapResult::Rejected);
	});

	// The player's mappings are rebuilt on the next frames.
	WaitUntil(this, TEXT("Jump on E and Interact on Space Bar"), 5.0, []()
	{
		const UStdInputSubsystem* Input = GetInput();
		return Input && Input->GetKeyForTag(StdGameplayTags::InputTag_Jump) == EKeys::E
			&& Input->GetKeyForTag(StdGameplayTags::InputTag_Interact) == EKeys::SpaceBar;
	});

	Step([]()
	{
		if (UStdInputSubsystem* Input = GetInput())
		{
			Input->ResetKeysToDefaults();
		}
	});
	WaitUntil(this, TEXT("the default keys after a reset"), 5.0, []()
	{
		const UStdInputSubsystem* Input = GetInput();
		return Input && Input->GetKeyForTag(StdGameplayTags::InputTag_Jump) == EKeys::SpaceBar
			&& Input->GetKeyForTag(StdGameplayTags::InputTag_Interact) == EKeys::E;
	});

	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
