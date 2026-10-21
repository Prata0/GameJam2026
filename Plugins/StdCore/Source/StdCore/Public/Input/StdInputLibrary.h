#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StdInputLibrary.generated.h"

class APlayerController;
class UInputAction;
class UStdInputSubsystem;

/** Blueprint helpers for input tags. Events for tags come from AStdPlayerController::OnInputTag. */
UCLASS()
class STDCORE_API UStdInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The action paired with Tag in the Input Configs of the Std Core settings, or null. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Input")
	static const UInputAction* GetInputActionForTag(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "Std Core|Input")
	static UStdInputSubsystem* GetStdInputSubsystem(const APlayerController* PlayerController);

	/** The key currently bound to Tag for this player, following remapping. */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	static FKey GetKeyForInputTag(const APlayerController* PlayerController, FGameplayTag Tag, bool bPreferGamepad = false);

	/** The same key as display text, for hints such as "Press E to interact". */
	UFUNCTION(BlueprintCallable, Category = "Std Core|Input")
	static FText GetKeyTextForInputTag(const APlayerController* PlayerController, FGameplayTag Tag, bool bPreferGamepad = false);
};
