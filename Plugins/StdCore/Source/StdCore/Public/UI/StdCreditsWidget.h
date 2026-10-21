#pragma once

#include "CoreMinimal.h"
#include "UI/StdScreenWidget.h"
#include "StdCreditsWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;

/** Credits: a text that scrolls by itself, and Back. */
UCLASS(Abstract)
class STDCORE_API UStdCreditsWidget : public UStdScreenWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual UWidget* GetDefaultFocusWidget() const override;

	/** Scrolled automatically; contains Text_Credits. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_Credits;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Credits;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	/** When not empty, replaces the text of Text_Credits. Either edit this or the text block itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|Credits", meta = (MultiLine = true))
	FText CreditsText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|Credits", meta = (ClampMin = "0.0", Units = "s"))
	float StartDelay = 1.5f;

	/** Pixels per second. 0 disables the automatic scrolling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|Credits", meta = (ClampMin = "0.0"))
	float ScrollSpeed = 40.f;

	/** Start again from the top after reaching the end. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|Credits")
	bool bLoop = true;

private:
	UFUNCTION()
	void HandleBackClicked();

	float DelayRemaining = 0.f;
	float ScrollOffset = 0.f;
};
