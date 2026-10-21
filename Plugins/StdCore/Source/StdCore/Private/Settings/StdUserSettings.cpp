#include "Settings/StdUserSettings.h"

void UStdUserSettings::ResetToDefaults()
{
	MasterVolume = DefaultMasterVolume;
	MusicVolume = DefaultMusicVolume;
	SfxVolume = DefaultSfxVolume;
}

void UStdUserSettings::Save()
{
	SaveConfig();
}
