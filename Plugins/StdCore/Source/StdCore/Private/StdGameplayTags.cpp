#include "StdGameplayTags.h"

namespace StdGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Movement on two axes (Axis2D).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look, "InputTag.Look", "Camera look on two axes (Axis2D).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "Interact with the world.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Pause, "InputTag.Pause", "Open and close the pause menu.");
}
