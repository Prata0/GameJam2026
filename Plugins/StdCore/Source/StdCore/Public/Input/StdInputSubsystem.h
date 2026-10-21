#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "StdInputSubsystem.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

/** One remappable keyboard/mouse entry, as shown in the Controls tab. */
USTRUCT(BlueprintType)
struct STDCORE_API FStdKeyBindingRow
{
	GENERATED_BODY()

	/** Player Mappable Key name set on the Input Action or on the mapping (e.g. MoveForward). */
	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	FName MappingName;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	EPlayerMappableKeySlot Slot = EPlayerMappableKeySlot::First;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	FText Category;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	FKey CurrentKey;

	UPROPERTY(BlueprintReadOnly, Category = "Std Core|Input")
	FKey DefaultKey;
};

UENUM(BlueprintType)
enum class EStdRemapResult : uint8
{
	/** The entry now uses the new key. */
	Remapped,
	/** The key was used by another entry: the two entries exchanged keys. */
	Swapped,
	/** The entry already used that key. */
	Unchanged,
	/** Reserved, gamepad, axis or unknown key, or no such entry: nothing changed. */
	Rejected
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStdOnKeyMappingsChanged);

/**
 * Input of one local player: mapping contexts from UStdProjectSettings, keyboard/mouse remapping through the
 * Enhanced Input user settings, and the key currently bound to an input tag (for "Press E to interact" hints).
 *
 * Mapping contexts are added and registered for remapping whenever the player controller changes, so saved
 * mappings are active before the first input. Remapped keys are saved by Enhanced Input in its own save slot.
 */
UCLASS()
class STDCORE_API UStdInputSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	/** Adds the configured mapping contexts and registers them for remapping. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	void ApplyMappingContexts();

	/** Remappable keyboard/mouse entries, grouped by category in the order they first appear. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	TArray<FStdKeyBindingRow> GetRemappableKeys() const;

	/**
	 * Assigns NewKey to an entry. A key already used by another entry is swapped with it; reserved keys, gamepad
	 * keys and axes are refused. OutMessage explains a swap or a refusal (empty otherwise). Saved immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	EStdRemapResult RemapKey(FName MappingName, EPlayerMappableKeySlot Slot, FKey NewKey, FText& OutMessage);

	/** Restores every remappable key to the value in the mapping contexts. Saved immediately. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	void ResetKeysToDefaults();

	/** Whether the player may assign Key; OutReason says why not. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Input")
	bool CanAssignKey(FKey Key, FText& OutReason) const;

	/**
	 * The key currently bound to the action of Tag, following remapping. Keyboard/mouse keys are preferred unless
	 * bPreferGamepad. Invalid when the tag has no action or the action no key.
	 */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	FKey GetKeyForTag(FGameplayTag Tag, bool bPreferGamepad = false) const;

	/** GetKeyForTag as display text ("E", "Space Bar"); "Unbound" when there is no key. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	FText GetKeyDisplayTextForTag(FGameplayTag Tag, bool bPreferGamepad = false) const;

	/** Set by key selectors while they wait for a key, so pause and back handling leave that key alone. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	void SetKeyCaptureActive(bool bActive) { bKeyCaptureActive = bActive; }

	UFUNCTION(BlueprintPure, Category = "Std Core|Input")
	bool IsKeyCaptureActive() const { return bKeyCaptureActive; }

	/** Broadcast after a remap or a reset. */
	UPROPERTY(BlueprintAssignable, Category = "Std Core|Input")
	FStdOnKeyMappingsChanged OnKeyMappingsChanged;

private:
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInput() const;
	UEnhancedInputUserSettings* GetUserSettings() const;
	UEnhancedPlayerMappableKeyProfile* GetActiveProfile() const;
	const FPlayerKeyMapping* FindMapping(FName MappingName, EPlayerMappableKeySlot Slot) const;
	void CommitKeyChanges();
	bool TickRetryApply(float DeltaTime);

	static bool IsKeyboardMouseMapping(const FPlayerKeyMapping& Mapping);

	bool bKeyCaptureActive = false;
	FTSTicker::FDelegateHandle RetryHandle;
};
