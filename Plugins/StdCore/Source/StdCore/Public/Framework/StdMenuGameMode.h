#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StdMenuGameMode.generated.h"

/**
 * Main menu game mode: no pawn, AStdMenuPlayerController shows the menu.
 * Set it in the World Settings of the menu map, so the map works without touching the project's config.
 */
UCLASS()
class STDCORE_API AStdMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AStdMenuGameMode();

protected:
	/** No player ever spawns a pawn in the menu. */
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
};
