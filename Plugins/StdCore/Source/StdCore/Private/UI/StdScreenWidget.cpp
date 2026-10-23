#include "UI/StdScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/InputKeySelector.h"
#include "Components/Slider.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/StdInputSubsystem.h"
#include "InputCoreTypes.h"
#include "StdLog.h"
#include "UI/StdUITheme.h"
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

void UStdScreenWidget::NativePreConstruct()
{
	// Before the Blueprint's Pre Construct, so a Blueprint can still override single widgets.
	if (bApplyTheme)
	{
		if (const UStdUITheme* Theme = StdUITheme::Get())
		{
			StdUITheme::ApplyToWidget(*Theme, *this);
		}
	}
	Super::NativePreConstruct();
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

namespace StdScreenFocus
{
	bool IsInteractive(const UWidget* Widget)
	{
		return Widget->IsA<UButton>() || Widget->IsA<USlider>() || Widget->IsA<UInputKeySelector>();
	}

	/** Buttons, sliders and key selectors of the tree, including those inside nested user widgets (key binding rows). */
	void CollectInteractive(const UWidgetTree* Tree, TArray<UWidget*>& OutWidgets)
	{
		if (!Tree)
		{
			return;
		}
		Tree->ForEachWidget([&OutWidgets](UWidget* Widget)
		{
			if (IsInteractive(Widget))
			{
				OutWidgets.Add(Widget);
			}
			else if (const UUserWidget* Nested = Cast<UUserWidget>(Widget))
			{
				CollectInteractive(Nested->WidgetTree, OutWidgets);
			}
		});
	}
}

void UStdScreenWidget::UpdateFocusHighlight()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || IsDesignTime())
	{
		return;
	}

	TArray<UWidget*> Widgets;
	StdScreenFocus::CollectInteractive(WidgetTree, Widgets);

	UWidget* Focused = nullptr;
	for (UWidget* Widget : Widgets)
	{
		// Key selectors and sliders may hold the focus on an inner Slate widget.
		if (Widget->HasUserFocus(PlayerController) || Widget->HasUserFocusedDescendants(PlayerController))
		{
			Focused = Widget;
			break;
		}
	}

	// The mouse entering a widget moves the focus there, so only one widget is ever highlighted.
	TArray<TWeakObjectPtr<UWidget>> NowHovered;
	for (UWidget* Widget : Widgets)
	{
		if (Widget->IsHovered() && Widget->GetIsEnabled())
		{
			NowHovered.Add(Widget);
			if (!HoveredWidgets.Contains(Widget) && Widget != Focused && !IsKeyCaptureActive())
			{
				Widget->SetFocus();
				Focused = Widget;
			}
		}
	}
	HoveredWidgets = MoveTemp(NowHovered);

	if (Focused != HighlightedWidget.Get())
	{
		SetHighlighted(Focused);
	}
}

void UStdScreenWidget::SetHighlighted(UWidget* Widget)
{
	ResetFocusHighlight();
	if (!Widget)
	{
		return;
	}

	// FButtonStyle has no focused state: the hovered look stands in for it.
	if (UButton* Button = Cast<UButton>(Widget))
	{
		HighlightedButtonStyle = Button->GetStyle();
		FButtonStyle Style = HighlightedButtonStyle;
		Style.SetNormal(Style.Hovered);
		Style.SetNormalForeground(Style.HoveredForeground);
		Button->SetStyle(Style);
	}
	else if (UInputKeySelector* KeySelector = Cast<UInputKeySelector>(Widget))
	{
		HighlightedButtonStyle = KeySelector->GetButtonStyle();
		FButtonStyle Style = HighlightedButtonStyle;
		Style.SetNormal(Style.Hovered);
		Style.SetNormalForeground(Style.HoveredForeground);
		StdUITheme::SetKeySelectorStyle(KeySelector, Style);
	}
	else if (USlider* Slider = Cast<USlider>(Widget))
	{
		HighlightedSliderStyle = Slider->GetWidgetStyle();
		FSliderStyle Style = HighlightedSliderStyle;
		Style.SetNormalBarImage(Style.HoveredBarImage);
		Style.SetNormalThumbImage(Style.HoveredThumbImage);
		Slider->SetWidgetStyle(Style);
	}
	HighlightedWidget = Widget;
}

void UStdScreenWidget::ResetFocusHighlight()
{
	UWidget* Widget = HighlightedWidget.Get();
	HighlightedWidget.Reset();
	if (!Widget)
	{
		return;
	}

	if (UButton* Button = Cast<UButton>(Widget))
	{
		Button->SetStyle(HighlightedButtonStyle);
	}
	else if (UInputKeySelector* KeySelector = Cast<UInputKeySelector>(Widget))
	{
		StdUITheme::SetKeySelectorStyle(KeySelector, HighlightedButtonStyle);
	}
	else if (USlider* Slider = Cast<USlider>(Widget))
	{
		Slider->SetWidgetStyle(HighlightedSliderStyle);
	}
}

void UStdScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bHighlightFocus && !bClosing)
	{
		UpdateFocusHighlight();
	}

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
