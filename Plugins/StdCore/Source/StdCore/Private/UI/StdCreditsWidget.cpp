#include "UI/StdCreditsWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UStdCreditsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Back)
	{
		Button_Back->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
}

void UStdCreditsWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Text_Credits && !CreditsText.IsEmpty())
	{
		Text_Credits->SetText(CreditsText);
	}
}

void UStdCreditsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	DelayRemaining = StartDelay;
	ScrollOffset = 0.f;
	if (ScrollBox_Credits)
	{
		ScrollBox_Credits->SetScrollOffset(0.f);
	}
}

void UStdCreditsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ScrollBox_Credits || ScrollSpeed <= 0.f)
	{
		return;
	}

	if (DelayRemaining > 0.f)
	{
		DelayRemaining -= InDeltaTime;
		return;
	}

	const float End = ScrollBox_Credits->GetScrollOffsetOfEnd();
	ScrollOffset += ScrollSpeed * InDeltaTime;
	if (ScrollOffset >= End)
	{
		if (bLoop && End > 0.f)
		{
			ScrollOffset = 0.f;
			DelayRemaining = StartDelay;
		}
		else
		{
			ScrollOffset = End;
		}
	}
	ScrollBox_Credits->SetScrollOffset(ScrollOffset);
}

UWidget* UStdCreditsWidget::GetDefaultFocusWidget() const
{
	return Button_Back;
}

void UStdCreditsWidget::HandleBackClicked()
{
	CloseScreen();
}
