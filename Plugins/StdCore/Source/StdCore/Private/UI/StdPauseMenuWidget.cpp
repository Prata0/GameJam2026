#include "UI/StdPauseMenuWidget.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Flow/StdFlowSubsystem.h"
#include "Framework/StdPlayerController.h"
#include "Settings/StdProjectSettings.h"
#include "StdLog.h"
#include "UI/StdSettingsWidget.h"

void UStdPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Resume)
	{
		Button_Resume->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResumeClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSettingsClicked);
	}
	if (Button_MainMenu)
	{
		Button_MainMenu->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
}

UWidget* UStdPauseMenuWidget::GetDefaultFocusWidget() const
{
	return Button_Resume;
}

bool UStdPauseMenuWidget::NativeHandleBackAction()
{
	if (Super::NativeHandleBackAction())
	{
		return true;
	}
	ResumeOwningPlayer();
	return true;
}

void UStdPauseMenuWidget::ResumeOwningPlayer()
{
	if (AStdPlayerController* PlayerController = GetOwningPlayer<AStdPlayerController>())
	{
		PlayerController->ResumeGame();
	}
	else
	{
		UE_LOG(LogStdCore, Warning, TEXT("%s: the owning player controller is not an AStdPlayerController; cannot resume."), *GetName());
	}
}

void UStdPauseMenuWidget::HandleResumeClicked()
{
	ResumeOwningPlayer();
}

void UStdPauseMenuWidget::HandleSettingsClicked()
{
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	OpenChildScreen(StdSettings::LoadClass(Settings->SettingsWidgetClass, TEXT("SettingsWidgetClass"), StdCoreDefaults::SettingsWidget));
}

void UStdPauseMenuWidget::HandleMainMenuClicked()
{
	if (UStdFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStdFlowSubsystem>() : nullptr)
	{
		Flow->ReturnToMainMenu();
	}
}

void UStdPauseMenuWidget::HandleQuitClicked()
{
	if (UStdFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStdFlowSubsystem>() : nullptr)
	{
		Flow->QuitGame();
	}
}
