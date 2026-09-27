using System.IO;
using UnrealBuildTool;

public class RosBridge : ModuleRules
{
	public RosBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "Projects", "RenderCore", "RHI" }); // the camera's shader and GPU readback

		// Static Cyclone DDS and message types, built by setup.sh
		string Dds = Path.Combine(PluginDirectory, "ThirdParty", "cyclonedds");
		string Msgs = Path.Combine(PluginDirectory, "ThirdParty", "msgs");
		PublicIncludePaths.AddRange(new[] { Path.Combine(Dds, "include"), Msgs });
		PublicAdditionalLibraries.AddRange(new[] { Path.Combine(Msgs, "libmsgs.a"), Path.Combine(Dds, "lib", "libddsc.a") });
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicSystemLibraries.AddRange(new[] { "pthread", "dl" });
		}
	}
}
