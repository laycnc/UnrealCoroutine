// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealCoroutine : ModuleRules
{
	public UnrealCoroutine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// コルーチンにはC++20が必要
		CppStandard = CppStandardVersion.Cpp20;

		PrivateIncludePaths.Add("UnrealCoroutine/Private");
		PublicIncludePaths.Add("UnrealCoroutine/Public");
		
		PublicDependencyModuleNames.Add("Core");
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);

	}
}
