#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "GameplayTagContainer.h"

class UInputAction;
class UStdInputConfig;

/**
 * Binding gameplay code to input tags instead of Input Action assets. Works on any UEnhancedInputComponent,
 * so the project keeps the engine's DefaultInputComponentClass.
 *
 *     StdInput::BindActionByTag(EnhancedInputComponent, StdGameplayTags::InputTag_Jump, ETriggerEvent::Started,
 *         this, &AMyCharacter::Jump);
 */
namespace StdInput
{
	/** The action for Tag in the first Input Config of UStdProjectSettings::InputConfigs that has it, or null. */
	STDCORE_API const UInputAction* FindInputActionForTag(const FGameplayTag& Tag);

	/** Every {action, tag} pair of every configured Input Config, in order. Configs that fail to load are skipped. */
	STDCORE_API void GetAllTaggedActions(TArray<TPair<const UInputAction*, FGameplayTag>>& OutActions);

	/** Logs once that Tag has no action, naming where it was searched. */
	STDCORE_API void WarnMissingTag(const FGameplayTag& Tag, const UStdInputConfig* Config);

	/**
	 * Binds Func to the action paired with Tag in Config (or, when Config is null, in the configured Input Configs).
	 * Returns null, after one warning in the log, when the component is null or no action has the tag.
	 * Func takes the same forms as UEnhancedInputComponent::BindAction, including extra payload arguments.
	 */
	template <class UserClass, typename FuncType, typename... VarTypes>
	FEnhancedInputActionEventBinding* BindActionByTag(UEnhancedInputComponent* InputComponent, const UStdInputConfig* Config,
		const FGameplayTag& Tag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, VarTypes... Vars);

	template <class UserClass, typename FuncType, typename... VarTypes>
	FEnhancedInputActionEventBinding* BindActionByTag(UEnhancedInputComponent* InputComponent, const FGameplayTag& Tag,
		ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, VarTypes... Vars)
	{
		return BindActionByTag(InputComponent, nullptr, Tag, TriggerEvent, Object, Func, Vars...);
	}

	namespace Private
	{
		STDCORE_API const UInputAction* FindInConfig(const UStdInputConfig* Config, const FGameplayTag& Tag);
	}
}

template <class UserClass, typename FuncType, typename... VarTypes>
FEnhancedInputActionEventBinding* StdInput::BindActionByTag(UEnhancedInputComponent* InputComponent, const UStdInputConfig* Config,
	const FGameplayTag& Tag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, VarTypes... Vars)
{
	if (!InputComponent)
	{
		return nullptr;
	}

	const UInputAction* Action = Config ? Private::FindInConfig(Config, Tag) : FindInputActionForTag(Tag);
	if (!Action)
	{
		WarnMissingTag(Tag, Config);
		return nullptr;
	}

	return &InputComponent->BindAction(Action, TriggerEvent, Object, Func, Vars...);
}
