#include "Input/StdInputBinding.h"

#include "Input/StdInputConfig.h"
#include "InputAction.h"
#include "Settings/StdProjectSettings.h"
#include "StdLog.h"

namespace StdInput::Private
{
	void LoadConfigs(TArray<const UStdInputConfig*>& OutConfigs)
	{
		const UStdProjectSettings* Settings = UStdProjectSettings::Get();
		for (int32 Index = 0; Index < Settings->InputConfigs.Num(); ++Index)
		{
			const FString SettingName = FString::Printf(TEXT("InputConfigs[%d]"), Index);
			if (const UStdInputConfig* Config = StdSettings::LoadAsset(Settings->InputConfigs[Index], SettingName, StdCoreDefaults::InputConfig))
			{
				OutConfigs.Add(Config);
			}
		}

		if (Settings->InputConfigs.IsEmpty())
		{
			StdLog::WarnOnce(TEXT("InputConfigs|Empty"), FString::Printf(
				TEXT("Std Core setting 'InputConfigs' is empty (expected at least '%s'). No input tag can be bound."),
				StdCoreDefaults::InputConfig));
		}
	}
}

const UInputAction* StdInput::Private::FindInConfig(const UStdInputConfig* Config, const FGameplayTag& Tag)
{
	return Config ? Config->FindInputActionForTag(Tag) : nullptr;
}

const UInputAction* StdInput::FindInputActionForTag(const FGameplayTag& Tag)
{
	TArray<const UStdInputConfig*> Configs;
	Private::LoadConfigs(Configs);
	for (const UStdInputConfig* Config : Configs)
	{
		if (const UInputAction* Action = Config->FindInputActionForTag(Tag))
		{
			return Action;
		}
	}
	return nullptr;
}

void StdInput::GetAllTaggedActions(TArray<TPair<const UInputAction*, FGameplayTag>>& OutActions)
{
	TArray<const UStdInputConfig*> Configs;
	Private::LoadConfigs(Configs);
	for (const UStdInputConfig* Config : Configs)
	{
		for (const FStdInputActionBinding& Binding : Config->InputActions)
		{
			if (Binding.InputAction && Binding.InputTag.IsValid())
			{
				OutActions.Emplace(Binding.InputAction, Binding.InputTag);
			}
		}
	}
}

void StdInput::WarnMissingTag(const FGameplayTag& Tag, const UStdInputConfig* Config)
{
	const FString Where = Config ? FString::Printf(TEXT("Input Config '%s'"), *Config->GetPathName()) : FString(TEXT("the Input Configs of Std Core settings"));
	StdLog::WarnOnce(TEXT("MissingInputTag|") + Tag.ToString() + TEXT("|") + Where, FString::Printf(
		TEXT("No Input Action has the tag '%s' in %s: nothing is bound to it."), *Tag.ToString(), *Where));
}
