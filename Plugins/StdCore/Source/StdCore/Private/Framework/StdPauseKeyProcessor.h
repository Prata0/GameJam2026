#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/StdPlayerController.h"
#include "UI/StdScreenWidget.h"

/**
 * Active only while AStdPlayerController's pause menu is open, when the UI owns the keyboard and Enhanced Input
 * receives nothing. Runs before the focused widget, so the pause keys work whatever has the focus.
 */
class FStdPauseKeyProcessor : public IInputProcessor
{
public:
	explicit FStdPauseKeyProcessor(AStdPlayerController* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		AStdPlayerController* PlayerController = Owner.Get();
		if (!PlayerController || !PlayerController->IsPausedByMenu() || InKeyEvent.IsRepeat() || PlayerController->IsKeyCaptureActive())
		{
			return false;
		}

		const FKey Key = InKeyEvent.GetKey();
		if (UStdScreenWidget::IsBackKey(Key))
		{
			PlayerController->HandlePauseBackKey();
			return true;
		}
		if (PlayerController->IsPauseResumeKey(Key))
		{
			PlayerController->HandlePauseResumeKey();
			return true;
		}
		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("StdCorePauseKeys"); }

private:
	TWeakObjectPtr<AStdPlayerController> Owner;
};
