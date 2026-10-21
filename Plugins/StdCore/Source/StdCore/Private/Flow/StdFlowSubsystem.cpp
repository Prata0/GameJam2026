#include "Flow/StdFlowSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Flow/StdInputBlocker.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Settings/StdProjectSettings.h"
#include "StdLog.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"

namespace StdFlow
{
	/** Above every UMG layer: menus are added with small Z orders. */
	constexpr int32 OverlayZOrder = 10000;

	/** A blocking load arrives as one huge tick; clamping it keeps the fade visible instead of skipping it. */
	constexpr float MaxFadeStep = 1.f / 30.f;
}

bool UStdFlowSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UStdFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick));
	WorldInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ThisClass::HandleWorldInitialized);
	if (GEngine)
	{
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	if (UStdProjectSettings::Get()->bFadeInOnStartup)
	{
		// Opaque from the start: the overlay is attached as soon as the first world exists, before its first frame.
		Alpha = 1.f;
		SetState(EStdFlowState::Loading);
		SetInputBlocked(true);
	}
}

void UStdFlowSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedHandle);
	if (GEngine)
	{
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	SetInputBlocked(false);
	RemoveOverlay();
	Super::Deinitialize();
}

bool UStdFlowSubsystem::TravelToLevel(TSoftObjectPtr<UWorld> Level)
{
	if (IsTransitioning())
	{
		UE_LOG(LogStdCore, Log, TEXT("TravelToLevel(%s) ignored: a transition is already running."), *Level.ToString());
		return false;
	}

	if (Level.IsNull())
	{
		UE_LOG(LogStdCore, Error, TEXT("TravelToLevel called with an empty map; nothing happens."));
		return false;
	}

	const FString PackageName = Level.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogStdCore, Error, TEXT("TravelToLevel(%s): the map package '%s' does not exist; nothing happens."),
			*Level.ToString(), *PackageName);
		return false;
	}

	UE_LOG(LogStdCore, Log, TEXT("Travelling to %s."), *PackageName);
	PendingLevel = Level;
	bQuitAfterFadeOut = false;
	BeginFadeOut();
	return true;
}

bool UStdFlowSubsystem::ReturnToMainMenu()
{
	return TravelToLevel(UStdProjectSettings::Get()->MainMenuMap);
}

bool UStdFlowSubsystem::StartGameplay()
{
	return TravelToLevel(UStdProjectSettings::Get()->GameplayMap);
}

void UStdFlowSubsystem::QuitGame()
{
	if (IsTransitioning())
	{
		UE_LOG(LogStdCore, Log, TEXT("QuitGame ignored: a transition is already running."));
		return;
	}

	UE_LOG(LogStdCore, Log, TEXT("Quitting."));
	PendingLevel.Reset();
	bQuitAfterFadeOut = true;
	BeginFadeOut();
}

void UStdFlowSubsystem::BeginFadeOut()
{
	EnsureOverlay();
	SetInputBlocked(true);
	SetState(EStdFlowState::FadingOut);
}

bool UStdFlowSubsystem::Tick(float DeltaTime)
{
	const UStdProjectSettings* Settings = UStdProjectSettings::Get();
	const float Step = FMath::Min(DeltaTime, StdFlow::MaxFadeStep);

	switch (State)
	{
	case EStdFlowState::FadingOut:
		Alpha = Settings->FadeOutDuration > 0.f ? FMath::Min(Alpha + Step / Settings->FadeOutDuration, 1.f) : 1.f;
		if (Alpha >= 1.f)
		{
			HandleFadeOutFinished();
		}
		break;

	case EStdFlowState::Loading:
		if (FPlatformTime::Seconds() - StateStartTime > Settings->TravelTimeoutSeconds)
		{
			AbortTransition(FString::Printf(TEXT("the new map did not arrive within %.0f s"), Settings->TravelTimeoutSeconds));
		}
		break;

	case EStdFlowState::WaitingToFadeIn:
		FadeInDelayRemaining -= Step;
		if (FadeInDelayRemaining <= 0.f)
		{
			SetState(EStdFlowState::FadingIn);
		}
		break;

	case EStdFlowState::FadingIn:
		Alpha = Settings->FadeInDuration > 0.f ? FMath::Max(Alpha - Step / Settings->FadeInDuration, 0.f) : 0.f;
		if (Alpha <= 0.f)
		{
			FinishTransition();
		}
		break;

	case EStdFlowState::Idle:
		break;
	}

	return true;
}

void UStdFlowSubsystem::HandleFadeOutFinished()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;

	if (bQuitAfterFadeOut)
	{
		bQuitAfterFadeOut = false;
		SetState(EStdFlowState::Idle);
		UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
		return;
	}

	if (!World)
	{
		AbortTransition(TEXT("there is no world to travel from"));
		return;
	}

	SetState(EStdFlowState::Loading);
	UGameplayStatics::OpenLevelBySoftObjectPtr(World, PendingLevel);
}

void UStdFlowSubsystem::HandleWorldInitialized(const FActorsInitializedParams& Params)
{
	UWorld* World = Params.World;
	if (!World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	EnsureOverlay();

	if (State == EStdFlowState::Loading)
	{
		PendingLevel.Reset();
		FadeInDelayRemaining = UStdProjectSettings::Get()->FadeInDelay;
		SetState(EStdFlowState::WaitingToFadeIn);
	}
}

void UStdFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (State == EStdFlowState::Loading)
	{
		AbortTransition(FString::Printf(TEXT("travel failed (%s): %s"), ETravelFailure::ToString(FailureType), *ErrorString));
	}
}

void UStdFlowSubsystem::FinishTransition()
{
	if (State == EStdFlowState::FadingIn)
	{
		UE_LOG(LogStdCore, Log, TEXT("Transition finished."));
	}
	Alpha = 0.f;
	SetState(EStdFlowState::Idle);
	SetInputBlocked(false);
}

void UStdFlowSubsystem::AbortTransition(const FString& Reason)
{
	UE_LOG(LogStdCore, Error, TEXT("Map transition aborted: %s. Removing the fade and unblocking input."), *Reason);
	PendingLevel.Reset();
	FinishTransition();
}

void UStdFlowSubsystem::SetState(EStdFlowState NewState)
{
	State = NewState;
	StateStartTime = FPlatformTime::Seconds();
}

void UStdFlowSubsystem::EnsureOverlay()
{
	UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (!ViewportClient)
	{
		return;
	}

	if (Overlay.IsValid() && OverlayViewport.Get() == ViewportClient)
	{
		return;
	}

	RemoveOverlay();

	TWeakObjectPtr<UStdFlowSubsystem> WeakThis(this);
	Overlay = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([WeakThis]()
		{
			return FSlateColor(WeakThis.IsValid() ? WeakThis->GetOverlayColor() : FLinearColor::Transparent);
		})
		.Visibility_Lambda([WeakThis]()
		{
			return WeakThis.IsValid() && WeakThis->IsOverlayVisible() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		});

	ViewportClient->AddViewportWidgetContent(Overlay.ToSharedRef(), StdFlow::OverlayZOrder);
	OverlayViewport = ViewportClient;
}

void UStdFlowSubsystem::RemoveOverlay()
{
	if (Overlay.IsValid())
	{
		if (UGameViewportClient* ViewportClient = OverlayViewport.Get())
		{
			ViewportClient->RemoveViewportWidgetContent(Overlay.ToSharedRef());
		}
	}
	Overlay.Reset();
	OverlayViewport.Reset();
}

void UStdFlowSubsystem::SetInputBlocked(bool bBlocked)
{
	if (!FSlateApplication::IsInitialized())
	{
		InputBlocker.Reset();
		return;
	}

	if (bBlocked && !InputBlocker.IsValid())
	{
		InputBlocker = MakeShared<FStdInputBlocker>();
		FSlateApplication::Get().RegisterInputPreProcessor(InputBlocker, 0);
	}
	else if (!bBlocked && InputBlocker.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputBlocker);
		InputBlocker.Reset();
	}
}

FLinearColor UStdFlowSubsystem::GetOverlayColor() const
{
	FLinearColor Color = UStdProjectSettings::Get()->FadeColor;
	Color.A *= Alpha;
	return Color;
}
