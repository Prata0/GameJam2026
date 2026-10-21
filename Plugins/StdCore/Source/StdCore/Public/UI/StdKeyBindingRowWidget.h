#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Commands/InputChord.h"
#include "Input/StdInputSubsystem.h"
#include "StdKeyBindingRowWidget.generated.h"

class UInputKeySelector;
class UStdSettingsWidget;
class UTextBlock;

/**
 * One line of the Controls tab: the action's name and a key selector.
 * The selector is a focusable button: with a gamepad, navigate to it and press the bottom face button, then press
 * a keyboard or mouse key. Escape or gamepad B cancel the capture. Gamepad keys are not accepted.
 */
UCLASS(Abstract)
class STDCORE_API UStdKeyBindingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FStdKeyBindingRow& InRow, UStdSettingsWidget* InOwner);

	/** Shows Key without changing the mapping. */
	void SetDisplayedKey(const FKey& Key);

	FName GetMappingName() const { return Row.MappingName; }
	EPlayerMappableKeySlot GetSlot() const { return Row.Slot; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ActionName;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI", meta = (BindWidget))
	TObjectPtr<UInputKeySelector> KeySelector;

	/** The entry shown by this row. */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|UI")
	FStdKeyBindingRow Row;

private:
	UStdInputSubsystem* GetInputSubsystem() const;

	UFUNCTION()
	void HandleKeySelected(FInputChord SelectedKey);

	UFUNCTION()
	void HandleIsSelectingKeyChanged();

	TWeakObjectPtr<UStdSettingsWidget> Owner;
};
