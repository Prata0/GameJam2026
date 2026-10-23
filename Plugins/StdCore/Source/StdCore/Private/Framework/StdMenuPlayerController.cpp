#include "Framework/StdMenuPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Input/StdInputSubsystem.h"
#include "Settings/StdProjectSettings.h"
#include "UI/StdMainMenuWidget.h"

AStdMenuPlayerController::AStdMenuPlayerController()
{
	bShowMouseCursor = true;
}

void AStdMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (UStdInputSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UStdInputSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->ApplyMappingContexts();
	}

	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	const TSubclassOf<UStdMainMenuWidget> MainMenuClass = StdSettings::LoadClass(Settings->MainMenuWidgetClass,
		TEXT("MainMenuWidgetClass"), StdCoreDefaults::MainMenuWidget);
	MainMenu = MainMenuClass ? CreateWidget<UStdMainMenuWidget>(this, MainMenuClass) : nullptr;

	if (MainMenu)
	{
		MainMenu->ShowAsRoot(MainMenuZOrder);
		// The focus goes to the menu's default button below: the user widget itself is not focusable.
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		MainMenu->FocusDefaultWidget();
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	SetShowMouseCursor(true);
}
