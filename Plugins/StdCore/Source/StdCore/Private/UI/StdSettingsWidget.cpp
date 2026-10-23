#include "UI/StdSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Input/StdInputSubsystem.h"
#include "Settings/StdProjectSettings.h"
#include "Settings/StdSettingsSubsystem.h"
#include "StdLog.h"
#include "UI/StdKeyBindingRowWidget.h"
#include "UI/StdUITheme.h"

#define LOCTEXT_NAMESPACE "StdCore"

namespace StdSettingsTabs
{
	constexpr int32 AudioVideo = 0;
	constexpr int32 Controls = 1;
}

UStdSettingsWidget::UStdSettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FullscreenText = LOCTEXT("WindowModeFullscreen", "Fullscreen");
	WindowedFullscreenText = LOCTEXT("WindowModeWindowedFullscreen", "Borderless window");
	WindowedText = LOCTEXT("WindowModeWindowed", "Window");
	QualityNames = {
		LOCTEXT("QualityLow", "Low"),
		LOCTEXT("QualityMedium", "Medium"),
		LOCTEXT("QualityHigh", "High"),
		LOCTEXT("QualityEpic", "Epic")
	};
	CustomQualityText = LOCTEXT("QualityCustom", "Custom");
	DefaultsRestoredText = LOCTEXT("KeysDefaultsRestored", "Default controls restored.");
}

void UStdSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_TabAudioVideo)
	{
		Button_TabAudioVideo->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTabAudioVideoClicked);
	}
	if (Button_TabControls)
	{
		Button_TabControls->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTabControlsClicked);
	}

	for (USlider* Slider : { Slider_MasterVolume.Get(), Slider_MusicVolume.Get(), Slider_SfxVolume.Get() })
	{
		if (Slider)
		{
			Slider->SetMinValue(0.f);
			Slider->SetMaxValue(1.f);
			Slider->SetStepSize(VolumeStepSize);
		}
	}
	if (Slider_MasterVolume)
	{
		Slider_MasterVolume->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMasterVolumeChanged);
	}
	if (Slider_MusicVolume)
	{
		Slider_MusicVolume->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMusicVolumeChanged);
	}
	if (Slider_SfxVolume)
	{
		Slider_SfxVolume->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleSfxVolumeChanged);
	}

	if (Button_WindowMode)
	{
		Button_WindowMode->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleWindowModeClicked);
	}
	if (Button_Quality)
	{
		Button_Quality->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQualityClicked);
	}
	if (Button_ResetDefaults)
	{
		Button_ResetDefaults->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResetDefaultsClicked);
	}
	if (Button_Back)
	{
		Button_Back->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackClicked);
	}
}

void UStdSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SelectTab(StdSettingsTabs::AudioVideo);
	SetKeyBindingMessage(FText::GetEmpty());
	RefreshAudioVideo();
	RebuildKeyBindings();

	if (UStdInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnKeyMappingsChanged.AddUniqueDynamic(this, &ThisClass::HandleKeyMappingsChanged);
	}
}

void UStdSettingsWidget::NativeDestruct()
{
	if (UStdInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnKeyMappingsChanged.RemoveDynamic(this, &ThisClass::HandleKeyMappingsChanged);
		InputSubsystem->SetKeyCaptureActive(false);
	}
	Super::NativeDestruct();
}

UWidget* UStdSettingsWidget::GetDefaultFocusWidget() const
{
	return Button_TabAudioVideo;
}

void UStdSettingsWidget::NativeOnScreenClosing()
{
	Super::NativeOnScreenClosing();

	if (UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->SaveSettings();
		UE_LOG(LogStdCore, Log, TEXT("Settings saved."));
	}
}

UStdSettingsSubsystem* UStdSettingsWidget::GetSettingsSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UStdSettingsSubsystem>() : nullptr;
}

UStdInputSubsystem* UStdSettingsWidget::GetInputSubsystem() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UStdInputSubsystem>() : nullptr;
}

// Audio and video

void UStdSettingsWidget::RefreshAudioVideo()
{
	const UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem();
	if (!SettingsSubsystem)
	{
		return;
	}

	if (Slider_MasterVolume)
	{
		Slider_MasterVolume->SetValue(SettingsSubsystem->GetVolume(EStdVolumeChannel::Master));
	}
	if (Slider_MusicVolume)
	{
		Slider_MusicVolume->SetValue(SettingsSubsystem->GetVolume(EStdVolumeChannel::Music));
	}
	if (Slider_SfxVolume)
	{
		Slider_SfxVolume->SetValue(SettingsSubsystem->GetVolume(EStdVolumeChannel::Sfx));
	}
	RefreshVolumeTexts();
	if (Text_WindowMode)
	{
		Text_WindowMode->SetText(GetWindowModeText(SettingsSubsystem->GetWindowMode()));
	}
	if (Text_Quality)
	{
		Text_Quality->SetText(GetQualityText(SettingsSubsystem->GetQualityLevel()));
	}
}

void UStdSettingsWidget::RefreshVolumeTexts()
{
	const UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem();
	if (!SettingsSubsystem)
	{
		return;
	}

	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	const TPair<UTextBlock*, EStdVolumeChannel> Texts[] = {
		{ Text_MasterVolumeValue, EStdVolumeChannel::Master },
		{ Text_MusicVolumeValue, EStdVolumeChannel::Music },
		{ Text_SfxVolumeValue, EStdVolumeChannel::Sfx }
	};
	for (const TPair<UTextBlock*, EStdVolumeChannel>& Entry : Texts)
	{
		if (Entry.Key)
		{
			Entry.Key->SetText(FText::AsPercent(SettingsSubsystem->GetVolume(Entry.Value), &Options));
		}
	}
}

void UStdSettingsWidget::SelectTab(int32 Index)
{
	if (Switcher_Tabs)
	{
		Switcher_Tabs->SetActiveWidgetIndex(Index);
	}

	if (bApplyTheme)
	{
		// The highlight keeps a copy of the style it replaced: give it back before restyling the tabs.
		ResetFocusHighlight();
		StdUITheme::SetTabSelected(Button_TabAudioVideo, Index == StdSettingsTabs::AudioVideo);
		StdUITheme::SetTabSelected(Button_TabControls, Index == StdSettingsTabs::Controls);
	}
}

FText UStdSettingsWidget::GetWindowModeText(EWindowMode::Type WindowMode) const
{
	switch (WindowMode)
	{
	case EWindowMode::Fullscreen: return FullscreenText;
	case EWindowMode::WindowedFullscreen: return WindowedFullscreenText;
	default: return WindowedText;
	}
}

FText UStdSettingsWidget::GetQualityText(int32 Level) const
{
	return QualityNames.IsValidIndex(Level) ? QualityNames[Level] : CustomQualityText;
}

void UStdSettingsWidget::HandleTabAudioVideoClicked()
{
	SelectTab(StdSettingsTabs::AudioVideo);
}

void UStdSettingsWidget::HandleTabControlsClicked()
{
	SelectTab(StdSettingsTabs::Controls);
}

void UStdSettingsWidget::HandleMasterVolumeChanged(float Value)
{
	if (UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->SetVolume(EStdVolumeChannel::Master, Value);
		RefreshVolumeTexts();
	}
}

void UStdSettingsWidget::HandleMusicVolumeChanged(float Value)
{
	if (UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->SetVolume(EStdVolumeChannel::Music, Value);
		RefreshVolumeTexts();
	}
}

void UStdSettingsWidget::HandleSfxVolumeChanged(float Value)
{
	if (UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem())
	{
		SettingsSubsystem->SetVolume(EStdVolumeChannel::Sfx, Value);
		RefreshVolumeTexts();
	}
}

void UStdSettingsWidget::HandleWindowModeClicked()
{
	UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem();
	if (!SettingsSubsystem)
	{
		return;
	}

	static const EWindowMode::Type Cycle[] = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
	const EWindowMode::Type Current = SettingsSubsystem->GetWindowMode();
	int32 CurrentIndex = INDEX_NONE;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cycle); ++Index)
	{
		if (Cycle[Index] == Current)
		{
			CurrentIndex = Index;
		}
	}

	SettingsSubsystem->SetWindowMode(Cycle[(CurrentIndex + 1) % UE_ARRAY_COUNT(Cycle)]);
	RefreshAudioVideo();
}

void UStdSettingsWidget::HandleQualityClicked()
{
	UStdSettingsSubsystem* SettingsSubsystem = GetSettingsSubsystem();
	if (!SettingsSubsystem)
	{
		return;
	}

	// A custom level (-1) moves to Low, like any other wrap-around.
	const int32 Current = SettingsSubsystem->GetQualityLevel();
	SettingsSubsystem->SetQualityLevel(Current >= 0 && Current < 3 ? Current + 1 : 0);
	RefreshAudioVideo();
}

// Controls

UWidget* UStdSettingsWidget::CreateCategoryHeader_Implementation(const FText& Category)
{
	UTextBlock* Header = WidgetTree ? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : nullptr;
	if (Header)
	{
		Header->SetText(Category);
		const UStdUITheme* Theme = bApplyTheme ? StdUITheme::Get() : nullptr;
		if (Theme)
		{
			Header->SetFont(Theme->HeadingFont);
			Header->SetColorAndOpacity(FSlateColor(Theme->AccentColor));
		}
	}
	return Header;
}

void UStdSettingsWidget::RebuildKeyBindings()
{
	Rows.Reset();
	if (!Box_KeyBindings)
	{
		return;
	}
	Box_KeyBindings->ClearChildren();

	UStdInputSubsystem* InputSubsystem = GetInputSubsystem();
	if (!InputSubsystem)
	{
		return;
	}

	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	const TSubclassOf<UStdKeyBindingRowWidget> RowClass = StdSettings::LoadClass(Settings->KeyBindingRowWidgetClass,
		TEXT("KeyBindingRowWidgetClass"), StdCoreDefaults::KeyBindingRowWidget);
	if (!RowClass)
	{
		return;
	}

	FString CurrentCategory;
	bool bFirstRow = true;
	for (const FStdKeyBindingRow& RowData : InputSubsystem->GetRemappableKeys())
	{
		const FString Category = RowData.Category.ToString();
		if (bFirstRow || Category != CurrentCategory)
		{
			bFirstRow = false;
			CurrentCategory = Category;
			if (!RowData.Category.IsEmpty())
			{
				if (UWidget* Header = CreateCategoryHeader(RowData.Category))
				{
					Box_KeyBindings->AddChild(Header);
				}
			}
		}

		UStdKeyBindingRowWidget* Row = CreateWidget<UStdKeyBindingRowWidget>(this, RowClass);
		if (Row)
		{
			Row->Setup(RowData, this);
			Box_KeyBindings->AddChild(Row);
			Rows.Add(Row);
		}
	}
}

void UStdSettingsWidget::RefreshKeyBindingRows()
{
	const UStdInputSubsystem* InputSubsystem = GetInputSubsystem();
	if (!InputSubsystem)
	{
		return;
	}

	const TArray<FStdKeyBindingRow> RowData = InputSubsystem->GetRemappableKeys();
	for (UStdKeyBindingRowWidget* Row : Rows)
	{
		const FStdKeyBindingRow* Data = RowData.FindByPredicate([Row](const FStdKeyBindingRow& Candidate)
		{
			return Candidate.MappingName == Row->GetMappingName() && Candidate.Slot == Row->GetSlot();
		});
		if (Data)
		{
			Row->SetDisplayedKey(Data->CurrentKey);
		}
	}
}

void UStdSettingsWidget::HandleRowKeySelected(UStdKeyBindingRowWidget* Row, const FKey& NewKey)
{
	UStdInputSubsystem* InputSubsystem = GetInputSubsystem();
	if (!Row || !InputSubsystem)
	{
		return;
	}

	FText Message;
	const EStdRemapResult Result = InputSubsystem->RemapKey(Row->GetMappingName(), Row->GetSlot(), NewKey, Message);
	if (Result != EStdRemapResult::Unchanged)
	{
		SetKeyBindingMessage(Message);
	}

	// Also after a refusal: the selector shows the key that was pressed until it is told otherwise.
	RefreshKeyBindingRows();
}

void UStdSettingsWidget::HandleResetDefaultsClicked()
{
	if (UStdInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->ResetKeysToDefaults();
		RefreshKeyBindingRows();
		SetKeyBindingMessage(DefaultsRestoredText);
	}
}

void UStdSettingsWidget::HandleKeyMappingsChanged()
{
	RefreshKeyBindingRows();
}

void UStdSettingsWidget::SetKeyBindingMessage(const FText& Message)
{
	if (Text_KeyBindingMessage)
	{
		Text_KeyBindingMessage->SetText(Message);
	}
}

void UStdSettingsWidget::HandleBackClicked()
{
	CloseScreen();
}

#undef LOCTEXT_NAMESPACE
