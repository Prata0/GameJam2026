#include "Framework/StdGameMode.h"

#include "Framework/StdPlayerController.h"

AStdGameMode::AStdGameMode()
{
	PlayerControllerClass = AStdPlayerController::StaticClass();
}
