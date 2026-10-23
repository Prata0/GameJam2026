#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "StdScreenWidget.generated.h"

class UStdScreenWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStdOnScreenClosed, UStdScreenWidget*, Screen);

/**
 * Base class of every Std Core menu screen.
 *
 * - Focus: when shown, the screen focuses its default widget (the first button), and takes focus back if a click
 *   on empty space leaves it on the game viewport, so keyboard and gamepad navigation keep working.
 * - Navigation: OpenChildScreen opens a screen on top (Settings, Credits) and hides this one; CloseScreen goes back.
 * - Back: Escape and the gamepad's right face button run HandleBackAction (by default: close if there is a parent).
 *
 * Buttons react to Enter and to the gamepad's bottom face button through Slate's Accept navigation action.
 */
UCLASS(Abstract)
class STDCORE_API UStdScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Adds this screen to the viewport as the bottom of a stack and focuses its default widget. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|UI")
	void ShowAsRoot(int32 ZOrder = 10);

	/** Opens ChildClass on top of this screen, hides this one and moves focus to the child. Null if the class is null. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|UI")
	UStdScreenWidget* OpenChildScreen(TSubclassOf<UStdScreenWidget> ChildClass);

	/** Closes this screen and every screen opened from it. The parent becomes visible and gets the focus back. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|UI")
	void CloseScreen();

	/** What Escape and gamepad B do on this screen. Returns true when the input was used. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|UI")
	bool HandleBackAction();

	UFUNCTION(BlueprintCallable, Category = "Std Core|UI")
	void FocusDefaultWidget();

	/** The last screen opened from this one (this screen when it has no child). */
	UFUNCTION(BlueprintPure, Category = "Std Core|UI")
	UStdScreenWidget* GetTopScreen();

	UFUNCTION(BlueprintPure, Category = "Std Core|UI")
	UStdScreenWidget* GetParentScreen() const { return ParentScreen.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Std Core|UI")
	FStdOnScreenClosed OnScreenClosed;

	/** Escape and the gamepad's right face button. */
	static bool IsBackKey(const FKey& Key);

	/** Gives the highlighted widget its own style back. Call it before changing a button style at runtime. */
	void ResetFocusHighlight();

protected:
	/** The widget focused when the screen opens. Subclasses return their first button. */
	virtual UWidget* GetDefaultFocusWidget() const { return nullptr; }

	/** Default: close the screen when it was opened from another one. */
	virtual bool NativeHandleBackAction();

	/** Called once before the screen is removed, also when its parent closes it. Settings save here. */
	virtual void NativeOnScreenClosing() {}

	UFUNCTION(BlueprintImplementableEvent, Category = "Std Core|UI", meta = (DisplayName = "On Screen Closing"))
	void ReceiveScreenClosing();

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Take the focus back when it falls on the game viewport (a click on empty space). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Std Core|UI")
	bool bRestoreFocusWhenLost = true;

	/** Style this screen with the theme of Project Settings > Std Core > Theme (also in the designer). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|UI")
	bool bApplyTheme = true;

	/**
	 * The focused button, slider or key selector shows its hovered look, and the mouse moves the focus to what it
	 * hovers: one highlighted widget at a time, for mouse, keyboard and gamepad alike.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Std Core|UI")
	bool bHighlightFocus = true;

	bool IsKeyCaptureActive() const;

private:
	void UpdateFocusHighlight();
	void SetHighlighted(UWidget* Widget);

	/** The widget shown with its hovered look, and its style before that. */
	TWeakObjectPtr<UWidget> HighlightedWidget;
	FButtonStyle HighlightedButtonStyle;
	FSliderStyle HighlightedSliderStyle;

	/** Interactive widgets under the mouse during the last tick. */
	TArray<TWeakObjectPtr<UWidget>> HoveredWidgets;

	TWeakObjectPtr<UStdScreenWidget> ParentScreen;

	UPROPERTY(Transient)
	TObjectPtr<UStdScreenWidget> ChildScreen;

	int32 ScreenZOrder = 10;
	ESlateVisibility VisibilityBeforeChild = ESlateVisibility::SelfHitTestInvisible;
	bool bClosing = false;
};
