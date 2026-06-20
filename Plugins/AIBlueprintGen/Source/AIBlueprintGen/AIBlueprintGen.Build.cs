using UnrealBuildTool;

public class AIBlueprintGen : ModuleRules
{
	public AIBlueprintGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"ToolMenus",
			"Json",
			"JsonUtilities",
			"HTTP",
			"ContentBrowser",
			"AssetRegistry",
			"PythonScriptPlugin",
			"Projects",
			"InputCore",
			"EditorStyle",
			"ApplicationCore",
		});
	}
}
