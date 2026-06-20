using UnrealBuildTool;

public class AIAssetNamer : ModuleRules
{
	public AIAssetNamer(ReadOnlyTargetRules Target) : base(Target)
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
			"AssetTools",
			"PythonScriptPlugin",
			"Projects",
			"InputCore",
			"EditorStyle",
		});
	}
}
