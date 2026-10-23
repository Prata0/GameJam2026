#include "UI/StdUITheme.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/InputKeySelector.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Settings/StdProjectSettings.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace StdUIThemeDetail
{
	FLinearColor Darken(const FLinearColor& Color, float Factor)
	{
		return FLinearColor(Color.R * Factor, Color.G * Factor, Color.B * Factor, Color.A);
	}

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FSlateSound MakeSound(USoundBase* Sound)
	{
		FSlateSound SlateSound;
		SlateSound.SetResourceObject(Sound);
		return SlateSound;
	}
}

UStdUITheme::UStdUITheme()
{
	// sRGB values, converted to linear by FLinearColor(FColor).
	AccentColor = FLinearColor(FColor(45, 127, 249));
	PanelColor = FLinearColor(FColor(18, 20, 26, 235));
	BackdropColor = FLinearColor(FColor(0, 0, 0, 140));
	ButtonColor = FLinearColor(FColor(44, 48, 58));
	TextColor = FLinearColor(FColor(236, 238, 242));
	MutedTextColor = FLinearColor(FColor(150, 156, 168));
	TextOnAccentColor = FLinearColor(FColor(255, 255, 255));

	// The engine's Roboto font asset, also used by UMG's default text: always cooked with the engine content it needs.
	static ConstructorHelpers::FObjectFinder<UFont> RobotoFont(*UWidget::GetDefaultFontName());
	if (RobotoFont.Succeeded())
	{
		TitleFont = FSlateFontInfo(RobotoFont.Object, 40, TEXT("Bold"));
		HeadingFont = FSlateFontInfo(RobotoFont.Object, 22, TEXT("Bold"));
		BodyFont = FSlateFontInfo(RobotoFont.Object, 18, TEXT("Regular"));
		ButtonFont = FSlateFontInfo(RobotoFont.Object, 18, TEXT("Bold"));
	}
}

FSlateBrush UStdUITheme::MakeRoundedBrush(const FLinearColor& Color, float Radius) const
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(Color);
	Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	return Brush;
}

FButtonStyle UStdUITheme::MakeButtonStyle() const
{
	using namespace StdUIThemeDetail;

	FButtonStyle Style;
	Style.SetNormal(MakeRoundedBrush(ButtonColor, CornerRadius));
	Style.SetHovered(MakeRoundedBrush(AccentColor, CornerRadius));
	Style.SetPressed(MakeRoundedBrush(Darken(AccentColor, 0.7f), CornerRadius));
	Style.SetDisabled(MakeRoundedBrush(WithAlpha(ButtonColor, ButtonColor.A * 0.5f), CornerRadius));
	Style.SetNormalForeground(FSlateColor(TextColor));
	Style.SetHoveredForeground(FSlateColor(TextOnAccentColor));
	Style.SetPressedForeground(FSlateColor(TextOnAccentColor));
	Style.SetDisabledForeground(FSlateColor(MutedTextColor));
	Style.SetNormalPadding(ButtonPadding);
	Style.SetPressedPadding(ButtonPadding);
	if (HoverSound)
	{
		Style.SetHoveredSound(MakeSound(HoverSound));
	}
	if (PressSound)
	{
		Style.SetPressedSound(MakeSound(PressSound));
	}
	return Style;
}

FButtonStyle UStdUITheme::MakeTabStyle(bool bSelected) const
{
	using namespace StdUIThemeDetail;

	// Selected: filled like a button. Not selected: no fill, muted label. Focus and hover use the accent, so the
	// three states stay distinct.
	FButtonStyle Style = MakeButtonStyle();
	if (bSelected)
	{
		Style.SetNormal(MakeRoundedBrush(ButtonColor, CornerRadius));
		Style.SetNormalForeground(FSlateColor(TextColor));
	}
	else
	{
		Style.SetNormal(MakeRoundedBrush(WithAlpha(ButtonColor, 0.f), CornerRadius));
		Style.SetNormalForeground(FSlateColor(MutedTextColor));
	}
	return Style;
}

FSliderStyle UStdUITheme::MakeSliderStyle() const
{
	constexpr float ThumbSize = 18.f;

	FSliderStyle Style;
	Style.SetNormalBarImage(MakeRoundedBrush(MutedTextColor, 2.f));
	Style.SetHoveredBarImage(MakeRoundedBrush(AccentColor, 2.f));
	Style.SetDisabledBarImage(MakeRoundedBrush(StdUIThemeDetail::WithAlpha(MutedTextColor, 0.3f), 2.f));

	FSlateBrush Thumb = MakeRoundedBrush(AccentColor, ThumbSize * 0.5f);
	Thumb.ImageSize = FVector2D(ThumbSize, ThumbSize);
	Style.SetNormalThumbImage(Thumb);
	Thumb.TintColor = FSlateColor(TextColor);
	Style.SetHoveredThumbImage(Thumb);
	Thumb.TintColor = FSlateColor(MutedTextColor);
	Style.SetDisabledThumbImage(Thumb);

	Style.SetBarThickness(4.f);
	return Style;
}

FTextBlockStyle UStdUITheme::MakeTextStyle(const FSlateFontInfo& Font, const FLinearColor& Color) const
{
	FTextBlockStyle Style;
	Style.SetFont(Font);
	Style.SetColorAndOpacity(FSlateColor(Color));
	return Style;
}

const UStdUITheme* StdUITheme::Get()
{
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	if (Settings->Theme.IsNull())
	{
		return nullptr;
	}
	return StdSettings::LoadAsset(Settings->Theme, TEXT("Theme"), StdCoreDefaults::UITheme);
}

void StdUITheme::ApplyToWidget(const UStdUITheme& Theme, UUserWidget& Widget)
{
	if (!Widget.WidgetTree)
	{
		return;
	}

	const FButtonStyle ButtonStyle = Theme.MakeButtonStyle();
	const FButtonStyle TabStyle = Theme.MakeTabStyle(false);
	const FSliderStyle SliderStyle = Theme.MakeSliderStyle();
	const FTextBlockStyle KeyTextStyle = Theme.MakeTextStyle(Theme.ButtonFont, Theme.TextColor);

	Widget.WidgetTree->ForEachWidget([&](UWidget* Child)
	{
		const FString Name = Child->GetName();

		if (UButton* Button = Cast<UButton>(Child))
		{
			Button->SetStyle(Name.StartsWith(TEXT("Button_Tab")) ? TabStyle : ButtonStyle);
		}
		else if (UTextBlock* Text = Cast<UTextBlock>(Child))
		{
			if (Cast<UButton>(Text->GetParent()))
			{
				// A button label: its color follows the button state (normal, hovered, pressed).
				Text->SetFont(Theme.ButtonFont);
				Text->SetColorAndOpacity(FSlateColor::UseForeground());
			}
			else if (Name == TEXT("Text_Title"))
			{
				Text->SetFont(Theme.TitleFont);
				Text->SetColorAndOpacity(FSlateColor(Theme.TextColor));
			}
			else if (Name.StartsWith(TEXT("Text_Heading")))
			{
				Text->SetFont(Theme.HeadingFont);
				Text->SetColorAndOpacity(FSlateColor(Theme.TextColor));
			}
			else
			{
				Text->SetFont(Theme.BodyFont);
				Text->SetColorAndOpacity(FSlateColor(Name == TEXT("Text_Version") ? Theme.MutedTextColor : Theme.TextColor));
			}
		}
		else if (USlider* Slider = Cast<USlider>(Child))
		{
			Slider->SetWidgetStyle(SliderStyle);
			Slider->SetSliderBarColor(FLinearColor::White);
			Slider->SetSliderHandleColor(FLinearColor::White);
		}
		else if (UInputKeySelector* KeySelector = Cast<UInputKeySelector>(Child))
		{
			SetKeySelectorStyle(KeySelector, ButtonStyle);
			// Same pointer hand-off as the button style.
			KeySelector->SetTextStyle(KeyTextStyle);
			KeySelector->SetTextStyle(KeySelector->GetTextStyle());
		}
		else if (UBorder* Border = Cast<UBorder>(Child))
		{
			if (Name == TEXT("Border_Background"))
			{
				Border->SetBrush(Theme.MakeRoundedBrush(Theme.BackdropColor, 0.f));
				Border->SetBrushColor(FLinearColor::White);
			}
			else if (Name == TEXT("Border_Panel"))
			{
				Border->SetBrush(Theme.MakeRoundedBrush(Theme.PanelColor, Theme.CornerRadius * 2.f));
				Border->SetBrushColor(FLinearColor::White);
			}
		}
	});
}

void StdUITheme::SetTabSelected(UButton* TabButton, bool bSelected)
{
	if (TabButton)
	{
		if (const UStdUITheme* Theme = Get())
		{
			TabButton->SetStyle(Theme->MakeTabStyle(bSelected));
		}
	}
}

void StdUITheme::SetKeySelectorStyle(UInputKeySelector* KeySelector, const FButtonStyle& ButtonStyle)
{
	if (KeySelector)
	{
		KeySelector->SetButtonStyle(ButtonStyle);
		KeySelector->SetButtonStyle(KeySelector->GetButtonStyle());
	}
}

void UStdUIThemeLibrary::ApplyStdUITheme(UUserWidget* Widget)
{
	if (Widget)
	{
		if (const UStdUITheme* Theme = StdUITheme::Get())
		{
			StdUITheme::ApplyToWidget(*Theme, *Widget);
		}
	}
}
