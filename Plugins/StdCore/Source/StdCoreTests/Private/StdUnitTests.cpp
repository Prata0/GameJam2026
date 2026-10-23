// Logic that needs no world: the volume formula, volume clamping, the styles built by the UI theme.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/StdSettingsSubsystem.h"
#include "Settings/StdUserSettings.h"
#include "UI/StdUITheme.h"
#include "UObject/Package.h"

namespace StdUnitTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdVolumesTest, "StdCore.Settings.Volumes", StdUnitTests::Flags)

bool FStdVolumesTest::RunTest(const FString& Parameters)
{
	const FStdClassVolumes Volumes = StdAudio::ComputeClassVolumes(0.5f, 0.8f, 1.f);
	TestEqual(TEXT("SC_Master stays at 1"), Volumes.Master, 1.f);
	TestEqual(TEXT("SC_Music = Music x Master"), Volumes.Music, 0.4f);
	TestEqual(TEXT("SC_SFX = SFX x Master"), Volumes.Sfx, 0.5f);
	TestEqual(TEXT("SC_UI = Master"), Volumes.Ui, 0.5f);

	const FStdClassVolumes Muted = StdAudio::ComputeClassVolumes(0.f, 1.f, 1.f);
	TestEqual(TEXT("Master 0 silences music"), Muted.Music, 0.f);
	TestEqual(TEXT("Master 0 silences effects"), Muted.Sfx, 0.f);
	TestEqual(TEXT("Master 0 silences the UI"), Muted.Ui, 0.f);

	// A transient instance: the class default object holds the player's saved values.
	UStdUserSettings* UserSettings = NewObject<UStdUserSettings>(GetTransientPackage());
	UserSettings->SetMasterVolume(1.5f);
	TestEqual(TEXT("Volumes above 1 are clamped"), UserSettings->GetMasterVolume(), 1.f);
	UserSettings->SetMusicVolume(-0.2f);
	TestEqual(TEXT("Volumes below 0 are clamped"), UserSettings->GetMusicVolume(), 0.f);
	UserSettings->SetSfxVolume(0.35f);
	TestEqual(TEXT("Volumes in range are kept"), UserSettings->GetSfxVolume(), 0.35f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStdThemeStylesTest, "StdCore.Theme.Styles", StdUnitTests::Flags)

bool FStdThemeStylesTest::RunTest(const FString& Parameters)
{
	const UStdUITheme* Theme = GetDefault<UStdUITheme>();

	const FButtonStyle Button = Theme->MakeButtonStyle();
	TestTrue(TEXT("Buttons use rounded boxes"), Button.Normal.DrawAs == ESlateBrushDrawType::RoundedBox);
	TestEqual(TEXT("Button normal color"), Button.Normal.TintColor.GetSpecifiedColor(), Theme->ButtonColor);
	TestEqual(TEXT("Button hovered color is the accent"), Button.Hovered.TintColor.GetSpecifiedColor(), Theme->AccentColor);
	TestEqual(TEXT("Button label over the accent"), Button.HoveredForeground.GetSpecifiedColor(), Theme->TextOnAccentColor);

	const FButtonStyle Selected = Theme->MakeTabStyle(true);
	const FButtonStyle Unselected = Theme->MakeTabStyle(false);
	TestEqual(TEXT("The selected tab is filled"), Selected.Normal.TintColor.GetSpecifiedColor(), Theme->ButtonColor);
	TestEqual(TEXT("An unselected tab has no fill"), Unselected.Normal.TintColor.GetSpecifiedColor().A, 0.f);
	TestEqual(TEXT("Tabs show the accent on hover and focus"), Unselected.Hovered.TintColor.GetSpecifiedColor(), Theme->AccentColor);

	const FSliderStyle Slider = Theme->MakeSliderStyle();
	TestEqual(TEXT("Slider thumb in the accent color"), Slider.NormalThumbImage.TintColor.GetSpecifiedColor(), Theme->AccentColor);
	TestTrue(TEXT("Slider thumb has a size"), Slider.NormalThumbImage.ImageSize.X > 0.f);

	TestNotNull(TEXT("The default fonts point to the engine's Roboto"), Theme->BodyFont.FontObject.Get());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
