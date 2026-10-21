#include "Framework/StdMenuGameMode.h"

#include "Framework/StdMenuPlayerController.h"

AStdMenuGameMode::AStdMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AStdMenuPlayerController::StaticClass();
}
