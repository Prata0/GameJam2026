#include "UI/StdKeyBindingRowWidget.h"

#include "Components/InputKeySelector.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "UI/StdScreenWidget.h"
#include "UI/StdSettingsWidget.h"

void UStdKeyBindingRowWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (KeySelector)
	{
		KeySelector->SetAllowGamepadKeys(false);
		KeySelector->SetAllowModifierKeys(false);
		KeySelector->SetEscapeKeys({ EKeys::Escape, EKeys::Gamepad_FaceButton_Right });
		KeySelector->OnKeySelected.AddUniqueDynamic(this, &ThisClass::HandleKeySelected);
		KeySelector->OnIsSelectingKeyChanged.AddUniqueDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
	}
}

void UStdKeyBindingRowWidget::NativeDestruct()
{
	// A row removed while capturing must not leave pause and back handling disabled.
	if (KeySelector && KeySelector->GetIsSelectingKey())
	{
		if (UStdInputSubsystem* InputSubsystem = GetInputSubsystem())
		{
			InputSubsystem->SetKeyCaptureActive(false);
		}
	}
	Super::NativeDestruct();
}

void UStdKeyBindingRowWidget::Setup(const FStdKeyBindingRow& InRow, UStdSettingsWidget* InOwner)
{
	Row = InRow;
	Owner = InOwner;

	if (Text_ActionName)
	{
		Text_ActionName->SetText(Row.DisplayName);
	}
	SetDisplayedKey(Row.CurrentKey);
}

void UStdKeyBindingRowWidget::SetDisplayedKey(const FKey& Key)
{
	Row.CurrentKey = Key;
	if (KeySelector)
	{
		KeySelector->SetSelectedKey(FInputChord(Key));
	}
}

UStdInputSubsystem* UStdKeyBindingRowWidget::GetInputSubsystem() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UStdInputSubsystem>() : nullptr;
}

void UStdKeyBindingRowWidget::HandleKeySelected(FInputChord SelectedKey)
{
	if (SelectedKey.Key == Row.CurrentKey || UStdScreenWidget::IsBackKey(SelectedKey.Key))
	{
		return;
	}

	if (UStdSettingsWidget* SettingsWidget = Owner.Get())
	{
		SettingsWidget->HandleRowKeySelected(this, SelectedKey.Key);
	}
}

void UStdKeyBindingRowWidget::HandleIsSelectingKeyChanged()
{
	if (UStdInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->SetKeyCaptureActive(KeySelector && KeySelector->GetIsSelectingKey());
	}
}
