#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "StdPlayerController.generated.h"

class FStdPauseKeyProcessor;
class UStdPauseMenuWidget;
struct FInputActionInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStdOnInputTag, FGameplayTag, InputTag, FInputActionValue, Value, ETriggerEvent, TriggerEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStdOnPauseChanged, bool, bPaused);

/**
 * Gameplay player controller: mapping contexts from UStdProjectSettings, pause through InputTag.Pause, and input
 * tags as Blueprint events. Movement stays with the game's pawn.
 *
 * Blueprint gameplay listens to OnInputTag (or overrides ReceiveInputTag) and switches on the tag: every action of
 * the configured Input Configs is reported with its tag, its value and its phase (Started, Triggered, Completed,
 * Canceled).
 *
 * While the pause menu is open, input goes to the UI only, so Enhanced Input sees no keys. An input pre-processor
 * handles the pause keys meanwhile: Escape and gamepad B go back one screen, or resume from the pause menu itself;
 * the other keys of IA_Pause (Start) resume from any screen. It stays inactive while a key selector captures a key.
 */
UCLASS()
class STDCORE_API AStdPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AStdPlayerController();

	/** Pauses and shows the pause menu. Refused during a map transition, when bCanPause is false or already paused. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Pause")
	bool PauseGame();

	/** Closes the pause menu (saving open settings) and resumes. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Pause")
	bool ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Std Core|Pause")
	void TogglePause();

	UFUNCTION(BlueprintPure, Category = "Std Core|Pause")
	bool IsPausedByMenu() const { return bPausedByMenu; }

	UFUNCTION(BlueprintPure, Category = "Std Core|Pause")
	UStdPauseMenuWidget* GetPauseMenu() const { return PauseMenu; }

	/** Set to false where the game must not pause (cutscenes, a game-over screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Std Core|Pause")
	bool bCanPause = true;

	/** Every tagged action of the configured Input Configs: tag, value and phase. */
	UPROPERTY(BlueprintAssignable, Category = "Std Core|Input")
	FStdOnInputTag OnInputTag;

	UPROPERTY(BlueprintAssignable, Category = "Std Core|Pause")
	FStdOnPauseChanged OnPauseChanged;

	/** Pause key processor: Escape or gamepad B while paused. */
	void HandlePauseBackKey();

	/** Pause key processor: another key of IA_Pause while paused. */
	void HandlePauseResumeKey();

	/** Keys that resume from any pause screen: IA_Pause's keys and the reserved keys, minus the back keys. */
	bool IsPauseResumeKey(const FKey& Key) const { return PauseResumeKeys.Contains(Key); }

	bool IsKeyCaptureActive() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Std Core|Input", meta = (DisplayName = "Receive Input Tag"))
	void ReceiveInputTag(FGameplayTag InputTag, FInputActionValue Value, ETriggerEvent TriggerEvent);

	/** Z order of the pause menu in the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|Pause")
	int32 PauseMenuZOrder = 50;

private:
	void HandlePauseAction();
	void HandleTaggedAction(const FInputActionInstance& Instance, FGameplayTag InputTag);
	void SetPauseKeyProcessorActive(bool bActive);
	void CollectPauseResumeKeys();

	UPROPERTY(Transient)
	TObjectPtr<UStdPauseMenuWidget> PauseMenu;

	bool bPausedByMenu = false;
	TArray<FKey> PauseResumeKeys;
	TSharedPtr<FStdPauseKeyProcessor> PauseKeyProcessor;
};
