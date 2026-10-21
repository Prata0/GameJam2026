#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StdGameMode.generated.h"

/** Gameplay game mode: AStdPlayerController and the default pawn. Extend it, or replace it and keep the controller. */
UCLASS()
class STDCORE_API AStdGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStdGameMode();
};
