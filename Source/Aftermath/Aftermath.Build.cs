using UnrealBuildTool;

// The game module stays thin: all game logic lives in Plugins/AftermathCore.
public class Aftermath : ModuleRules
{
	public Aftermath(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AftermathCore" });
	}
}
