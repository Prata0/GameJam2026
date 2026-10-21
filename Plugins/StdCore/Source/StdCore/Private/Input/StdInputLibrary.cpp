#include "Input/StdInputLibrary.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/StdInputBinding.h"
#include "Input/StdInputSubsystem.h"

#define LOCTEXT_NAMESPACE "StdCore"

const UInputAction* UStdInputLibrary::GetInputActionForTag(FGameplayTag Tag)
{
	return StdInput::FindInputActionForTag(Tag);
}

UStdInputSubsystem* UStdInputLibrary::GetStdInputSubsystem(const APlayerController* PlayerController)
{
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UStdInputSubsystem>() : nullptr;
}

FKey UStdInputLibrary::GetKeyForInputTag(const APlayerController* PlayerController, FGameplayTag Tag, bool bPreferGamepad)
{
	const UStdInputSubsystem* InputSubsystem = GetStdInputSubsystem(PlayerController);
	return InputSubsystem ? InputSubsystem->GetKeyForTag(Tag, bPreferGamepad) : EKeys::Invalid;
}

FText UStdInputLibrary::GetKeyTextForInputTag(const APlayerController* PlayerController, FGameplayTag Tag, bool bPreferGamepad)
{
	const UStdInputSubsystem* InputSubsystem = GetStdInputSubsystem(PlayerController);
	return InputSubsystem ? InputSubsystem->GetKeyDisplayTextForTag(Tag, bPreferGamepad) : LOCTEXT("KeyUnbound", "Unbound");
}

#undef LOCTEXT_NAMESPACE
