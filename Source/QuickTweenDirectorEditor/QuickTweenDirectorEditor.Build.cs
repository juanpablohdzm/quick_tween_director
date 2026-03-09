// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class QuickTweenDirectorEditor : ModuleRules
{
	public QuickTweenDirectorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Public"),
			Path.Combine(ModuleDirectory, "Public", "Widgets"),
			Path.Combine(ModuleDirectory, "Public", "Toolkit"),   // kept for empty stub
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Private"),
			Path.Combine(ModuleDirectory, "Private", "Widgets"),
			Path.Combine(ModuleDirectory, "Private", "Toolkit"),  // kept for empty stub
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"QuickTween",
			"QuickTweenDirector",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"DesktopPlatform",
			"EditorStyle",
			"EditorFramework",
			"ToolMenus",
			"AssetTools",
			"AssetRegistry",
			"PropertyEditor",
			"InputCore",
			"Projects",
			"AppFramework",
			// Blueprint editor integration
			"Kismet",
			"BlueprintGraph",
			"KismetCompiler",
		});
	}
}
