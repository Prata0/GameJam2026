#include "Input/StdInputConfig.h"

#include "InputAction.h"

const UInputAction* UStdInputConfig::FindInputActionForTag(FGameplayTag Tag) const
{
	for (const FStdInputActionBinding& Binding : InputActions)
	{
		if (Binding.InputAction && Binding.InputTag.MatchesTagExact(Tag))
		{
			return Binding.InputAction;
		}
	}
	return nullptr;
}
