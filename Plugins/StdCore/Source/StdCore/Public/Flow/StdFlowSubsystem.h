#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StdFlowSubsystem.generated.h"

class FStdInputBlocker;
class SWidget;
class UGameViewportClient;
class UWorld;
struct FActorsInitializedParams;

UENUM(BlueprintType)
enum class EStdFlowState : uint8
{
	Idle,
	FadingOut,
	Loading,
	WaitingToFadeIn,
	FadingIn
};

/**
 * Moving between maps with a fade: fade out, load, fade in.
 *
 * The fade is a Slate widget added to the game viewport client, not a UMG widget: the viewport client outlives
 * the world, so the overlay stays on screen while the map loads (UMG widgets are removed with their world).
 * While a transition runs, player input is blocked and further travel requests are ignored.
 * Fades run on real time, so they work with the game paused and ignore time dilation.
 */
UCLASS()
class STDCORE_API UStdFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Fades out, opens Level and fades in. Returns false, without starting anything, when a transition is already
	 * running, when Level is empty or when its package does not exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Flow")
	bool TravelToLevel(TSoftObjectPtr<UWorld> Level);

	/** Travels to UStdProjectSettings::MainMenuMap. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Flow")
	bool ReturnToMainMenu();

	/** Travels to UStdProjectSettings::GameplayMap. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Flow")
	bool StartGameplay();

	/** Fades out, then quits the game (ends the session in Play In Editor). Ignored during a transition. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Flow")
	void QuitGame();

	/** True from the start of a fade out to the end of the fade in. Pause and travel are refused meanwhile. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Flow")
	bool IsTransitioning() const { return State != EStdFlowState::Idle; }

	UFUNCTION(BlueprintPure, Category = "Std Core|Flow")
	EStdFlowState GetState() const { return State; }

private:
	bool Tick(float DeltaTime);

	void BeginFadeOut();
	void HandleFadeOutFinished();
	void HandleWorldInitialized(const FActorsInitializedParams& Params);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void FinishTransition();
	void AbortTransition(const FString& Reason);
	void SetState(EStdFlowState NewState);

	void EnsureOverlay();
	void RemoveOverlay();
	void SetInputBlocked(bool bBlocked);

	FLinearColor GetOverlayColor() const;
	bool IsOverlayVisible() const { return Alpha > 0.f; }

	EStdFlowState State = EStdFlowState::Idle;

	/** Opacity of the overlay, 0..1. */
	float Alpha = 0.f;

	/** Seconds left in WaitingToFadeIn. */
	float FadeInDelayRemaining = 0.f;

	/** FPlatformTime when the current state began, for the travel timeout. */
	double StateStartTime = 0.0;

	bool bQuitAfterFadeOut = false;

	TSoftObjectPtr<UWorld> PendingLevel;

	TSharedPtr<SWidget> Overlay;
	TWeakObjectPtr<UGameViewportClient> OverlayViewport;
	TSharedPtr<FStdInputBlocker> InputBlocker;

	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle WorldInitializedHandle;
	FDelegateHandle TravelFailureHandle;
};
