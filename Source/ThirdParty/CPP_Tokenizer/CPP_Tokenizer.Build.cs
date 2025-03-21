using System.IO;
using Microsoft.Extensions.Logging;
using UnrealBuildTool;

public class CPP_Tokenizer : ModuleRules
{
	public CPP_Tokenizer(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp17; // tokenizers-cpp was made using C++ 17 standards

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Include"));

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraryPaths.Add(Path.Combine(ModuleDirectory, "Lib"));

			PublicAdditionalLibraries.AddRange(
			[
				Path.Combine(ModuleDirectory, "Lib", "tokenizers_c.lib"),
				Path.Combine(ModuleDirectory, "Lib", "tokenizers_cpp.lib")
			]);

			PublicSystemLibraries.AddRange(
			[
				"Userenv.lib",
				"ntdll.lib",
				"Bcrypt.lib",
				"ws2_32.lib"
			]);
		}
		else
		{
			Logger.LogError("Cannot load tokenizers-cpp on {0}!", Target.Platform.ToString());
		}
	}
}