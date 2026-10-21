#pragma once

#include "CoreMinimal.h"
#include "UI/StdScreenWidget.h"
#include "StdPauseMenuWidget.generated.h"

class UButton;

/**
 * Pause menu: Resume, Settings, Main menu, Quit. Created by AStdPlayerController::PauseGame.
 * Back (Escape, gamepad B) on this screen resumes the game; Start resumes from any screen opened from it.
 */
UCLASS(Abstract)
class STDCORE_API UStdPauseMenuWidget : public UStdScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual UWidget* GetDefaultFocusWidget() const override;
	virtual bool NativeHandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Settings;

	/** Travels to UStdProjectSettings::MainMenuMap. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_MainMenu;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

private:
	void ResumeOwningPlayer();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
