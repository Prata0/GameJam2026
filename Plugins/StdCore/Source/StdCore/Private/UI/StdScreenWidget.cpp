#include "UI/StdScreenWidget.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/StdInputSubsystem.h"
#include "InputCoreTypes.h"
#include "StdLog.h"
#include "Widgets/SViewport.h"

bool UStdScreenWidget::IsBackKey(const FKey& Key)
{
	return Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
}

void UStdScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UWidget* FocusWidget = GetDefaultFocusWidget())
	{
		SetDesiredFocusWidget(FocusWidget);
	}
}

void UStdScreenWidget::ShowAsRoot(int32 ZOrder)
{
	ScreenZOrder = ZOrder;
	AddToViewport(ZOrder);
	FocusDefaultWidget();
}

UStdScreenWidget* UStdScreenWidget::OpenChildScreen(TSubclassOf<UStdScreenWidget> ChildClass)
{
	if (!ChildClass)
	{
		UE_LOG(LogStdCore, Warning, TEXT("%s: OpenChildScreen called without a widget class; ignored."), *GetName());
		return nullptr;
	}

	if (ChildScreen)
	{
		ChildScreen->CloseScreen();
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UStdScreenWidget* Child = OwningPlayer ? CreateWidget<UStdScreenWidget>(OwningPlayer, ChildClass) : CreateWidget<UStdScreenWidget>(this, ChildClass);
	if (!Child)
	{
		return nullptr;
	}

	Child->ParentScreen = this;
	Child->ScreenZOrder = ScreenZOrder + 1;
	ChildScreen = Child;

	VisibilityBeforeChild = GetVisibility();
	SetVisibility(ESlateVisibility::Collapsed);

	Child->AddToViewport(Child->ScreenZOrder);
	Child->FocusDefaultWidget();
	return Child;
}

void UStdScreenWidget::CloseScreen()
{
	if (bClosing)
	{
		return;
	}
	bClosing = true;

	if (ChildScreen)
	{
		ChildScreen->CloseScreen();
		ChildScreen = nullptr;
	}

	NativeOnScreenClosing();
	ReceiveScreenClosing();
	RemoveFromParent();

	if (UStdScreenWidget* Parent = ParentScreen.Get())
	{
		Parent->ChildScreen = nullptr;
		if (!Parent->bClosing)
		{
			Parent->SetVisibility(Parent->VisibilityBeforeChild);
			Parent->FocusDefaultWidget();
		}
	}

	OnScreenClosed.Broadcast(this);
}

bool UStdScreenWidget::HandleBackAction()
{
	return NativeHandleBackAction();
}

bool UStdScreenWidget::NativeHandleBackAction()
{
	if (ParentScreen.IsValid())
	{
		CloseScreen();
		return true;
	}
	return false;
}

void UStdScreenWidget::FocusDefaultWidget()
{
	if (UWidget* FocusWidget = GetDefaultFocusWidget())
	{
		FocusWidget->SetFocus();
	}
	else
	{
		SetFocus();
	}
}

UStdScreenWidget* UStdScreenWidget::GetTopScreen()
{
	UStdScreenWidget* Top = this;
	while (Top->ChildScreen)
	{
		Top = Top->ChildScreen;
	}
	return Top;
}

bool UStdScreenWidget::IsKeyCaptureActive() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UStdInputSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UStdInputSubsystem>() : nullptr;
	return InputSubsystem && InputSubsystem->IsKeyCaptureActive();
}

void UStdScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bRestoreFocusWhenLost || ChildScreen || bClosing || !IsVisible())
	{
		return;
	}

	// Only when the focus sits on the game viewport itself: never take it from editor panels or other windows.
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	const TSharedPtr<SViewport> ViewportWidget = ViewportClient ? ViewportClient->GetGameViewportWidget() : nullptr;
	const TSharedPtr<FSlateUser> SlateUser = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	if (ViewportWidget.IsValid() && SlateUser.IsValid() && SlateUser->GetFocusedWidget().Get() == ViewportWidget.Get())
	{
		FocusDefaultWidget();
	}
}

FReply UStdScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsBackKey(InKeyEvent.GetKey()) && !InKeyEvent.IsRepeat() && !IsKeyCaptureActive())
	{
		if (HandleBackAction())
		{
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
