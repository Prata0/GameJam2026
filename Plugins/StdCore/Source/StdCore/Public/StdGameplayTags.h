#pragma once

#include "NativeGameplayTags.h"

/**
 * Input tags defined by the plugin. Gameplay code binds to these instead of Input Action assets.
 * A game adds its own tags (in its config or with its own UE_DEFINE_GAMEPLAY_TAG) without touching the plugin.
 */
namespace StdGameplayTags
{
	STDCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	STDCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look);
	STDCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	STDCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);
	STDCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Pause);
}
