using UnrealBuildTool;

/** Automation tests of the Std Core plugin. Editor only: never part of a packaged game. */
public class StdCoreTests : ModuleRules
{
	public StdCoreTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"UMG",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"StdCore"
		});
	}
}
