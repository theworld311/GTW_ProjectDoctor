using UnrealBuildTool;

public class ProjectDoctor : ModuleRules
{
	public ProjectDoctor(ReadOnlyTargetRules Target) : base(Target)
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
			"EditorStyle",
			"EditorWidgets",
			"UnrealEd",
			"ToolMenus",
			"WorkspaceMenuStructure",
			"InputCore",
			"Json",
			"JsonUtilities",
			"ContentBrowser",
			"AssetRegistry",
			"PythonScriptPlugin",
			"DesktopPlatform",
		});
	}
}
