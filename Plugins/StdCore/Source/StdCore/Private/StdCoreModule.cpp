#include "StdCoreModule.h"

#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/PackageName.h"
#include "Settings/StdProjectSettings.h"
#include "StdLog.h"
#include "UObject/ICookInfo.h"

namespace StdCook
{
	/**
	 * Most plugin assets are reached only through soft pointers in UStdProjectSettings, which the cooker does not
	 * follow, and a plugin's Config/Game.ini does not reach the cooker's Directories to Always Cook. So the plugin
	 * adds them itself: every package under /StdCore, plus every asset the settings point to (the game's own maps,
	 * widgets or theme included).
	 */
	void ModifyCook(UE::Cook::ICookInfo& CookInfo, TArray<UE::Cook::FPackageCookRule>& InOutRules)
	{
		TSet<FName> Packages;

		TArray<FAssetData> PluginAssets;
		IAssetRegistry::GetChecked().GetAssetsByPath(TEXT("/StdCore"), PluginAssets, /*bRecursive*/ true);
		for (const FAssetData& Asset : PluginAssets)
		{
			Packages.Add(Asset.PackageName);
		}

		const UStdProjectSettings* Settings = UStdProjectSettings::Get();
		auto AddSetting = [&Packages](const FSoftObjectPath& Path)
		{
			if (!Path.IsNull() && FPackageName::DoesPackageExist(Path.GetLongPackageName()))
			{
				Packages.Add(Path.GetLongPackageFName());
			}
		};
		AddSetting(Settings->MainMenuMap.ToSoftObjectPath());
		AddSetting(Settings->GameplayMap.ToSoftObjectPath());
		AddSetting(Settings->VolumesSoundMix.ToSoftObjectPath());
		AddSetting(Settings->MasterSoundClass.ToSoftObjectPath());
		AddSetting(Settings->MusicSoundClass.ToSoftObjectPath());
		AddSetting(Settings->SfxSoundClass.ToSoftObjectPath());
		AddSetting(Settings->UiSoundClass.ToSoftObjectPath());
		for (const TSoftObjectPtr<UStdInputConfig>& Config : Settings->InputConfigs)
		{
			AddSetting(Config.ToSoftObjectPath());
		}
		for (const FStdMappingContextSetting& Entry : Settings->MappingContexts)
		{
			AddSetting(Entry.MappingContext.ToSoftObjectPath());
		}
		AddSetting(Settings->MainMenuWidgetClass.ToSoftObjectPath());
		AddSetting(Settings->PauseMenuWidgetClass.ToSoftObjectPath());
		AddSetting(Settings->SettingsWidgetClass.ToSoftObjectPath());
		AddSetting(Settings->KeyBindingRowWidgetClass.ToSoftObjectPath());
		AddSetting(Settings->CreditsWidgetClass.ToSoftObjectPath());
		AddSetting(Settings->Theme.ToSoftObjectPath());

		for (const FName& Package : Packages)
		{
			UE::Cook::FPackageCookRule& Rule = InOutRules.AddDefaulted_GetRef();
			Rule.PackageName = Package;
			Rule.InstigatorName = TEXT("StdCore");
			Rule.CookRule = UE::Cook::EPackageCookRule::AddToCook;
		}
		UE_LOG(LogStdCore, Display, TEXT("Std Core: %d packages added to the cook."), Packages.Num());
	}
}
#endif // WITH_EDITOR

void FStdCoreModule::StartupModule()
{
#if WITH_EDITOR
	ModifyCookHandle = UE::Cook::FDelegates::ModifyCook.AddStatic(&StdCook::ModifyCook);
#endif
}

void FStdCoreModule::ShutdownModule()
{
#if WITH_EDITOR
	UE::Cook::FDelegates::ModifyCook.Remove(ModifyCookHandle);
#endif
}

IMPLEMENT_MODULE(FStdCoreModule, StdCore)
