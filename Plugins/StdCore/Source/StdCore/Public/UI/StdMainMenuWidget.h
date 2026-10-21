#pragma once

#include "CoreMinimal.h"
#include "UI/StdScreenWidget.h"
#include "StdMainMenuWidget.generated.h"

class UButton;

/** Main menu: Play, Settings, Credits, Quit. Shown by AStdMenuPlayerController. */
UCLASS(Abstract)
class STDCORE_API UStdMainMenuWidget : public UStdScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* GetDefaultFocusWidget() const override;

	/** Travels to UStdProjectSettings::GameplayMap. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Play;

	/** Opens UStdProjectSettings::SettingsWidgetClass on top of the menu. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Settings;

	/** Opens UStdProjectSettings::CreditsWidgetClass on top of the menu. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Credits;

	/** Fades out and quits. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
