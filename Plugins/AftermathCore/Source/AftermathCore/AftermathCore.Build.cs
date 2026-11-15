using UnrealBuildTool;

public class AftermathCore : ModuleRules
{
	public AftermathCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json", "JsonUtilities", "CableComponent"
		});
	}
}
