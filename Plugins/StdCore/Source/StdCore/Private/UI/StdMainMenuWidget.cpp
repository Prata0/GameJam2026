#include "UI/StdMainMenuWidget.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Flow/StdFlowSubsystem.h"
#include "Settings/StdProjectSettings.h"
#include "UI/StdCreditsWidget.h"
#include "UI/StdSettingsWidget.h"

void UStdMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Play)
	{
		Button_Play->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePlayClicked);
	}
	if (Button_Settings)
	{
		Button_Settings->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSettingsClicked);
	}
	if (Button_Credits)
	{
		Button_Credits->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCreditsClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
}

UWidget* UStdMainMenuWidget::GetDefaultFocusWidget() const
{
	return Button_Play;
}

void UStdMainMenuWidget::HandlePlayClicked()
{
	if (UStdFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStdFlowSubsystem>() : nullptr)
	{
		Flow->StartGameplay();
	}
}

void UStdMainMenuWidget::HandleSettingsClicked()
{
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	OpenChildScreen(StdSettings::LoadClass(Settings->SettingsWidgetClass, TEXT("SettingsWidgetClass"), StdCoreDefaults::SettingsWidget));
}

void UStdMainMenuWidget::HandleCreditsClicked()
{
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	OpenChildScreen(StdSettings::LoadClass(Settings->CreditsWidgetClass, TEXT("CreditsWidgetClass"), StdCoreDefaults::CreditsWidget));
}

void UStdMainMenuWidget::HandleQuitClicked()
{
	if (UStdFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStdFlowSubsystem>() : nullptr)
	{
		Flow->QuitGame();
	}
}
