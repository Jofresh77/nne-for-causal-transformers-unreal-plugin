// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LLM_Integrator : ModuleRules
{
	public LLM_Integrator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CPP_Tokenizer",
				"NNE",
				"RHI",
				"NNEEditor",
				"NNEOnnxruntime",
				"NNERuntimeORT"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}