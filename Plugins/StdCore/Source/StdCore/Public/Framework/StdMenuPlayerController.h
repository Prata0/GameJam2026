#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StdMenuPlayerController.generated.h"

class UStdMainMenuWidget;

/** Player controller of the main menu map: shows the main menu, UI-only input, visible cursor, no pause. */
UCLASS()
class STDCORE_API AStdMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AStdMenuPlayerController();

	UFUNCTION(BlueprintPure, Category = "Std Core|UI")
	UStdMainMenuWidget* GetMainMenu() const { return MainMenu; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Std Core|UI")
	int32 MainMenuZOrder = 10;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStdMainMenuWidget> MainMenu;
};
