using UnrealBuildTool;

public class RosBridgeChecks : ModuleRules
{
	public RosBridgeChecks(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "RosBridge" });
	}
}
