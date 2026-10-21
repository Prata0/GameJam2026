#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StdInputConfig.generated.h"

class UInputAction;

/** One Input Action and the gameplay tag gameplay code uses to refer to it. */
USTRUCT(BlueprintType)
struct STDCORE_API FStdInputActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * Pairs of {Input Action, tag}. Gameplay binds to tags, so actions, mappings and keys can change without
 * touching gameplay code. List the configs to use in UStdProjectSettings::InputConfigs.
 */
UCLASS(BlueprintType, Const)
class STDCORE_API UStdInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The action paired with Tag (exact match), or null. */
	UFUNCTION(BlueprintPure, Category = "Std Core|Input")
	const UInputAction* FindInputActionForTag(FGameplayTag Tag) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputTag"))
	TArray<FStdInputActionBinding> InputActions;
};
