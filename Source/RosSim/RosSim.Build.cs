using UnrealBuildTool;

public class RosSim : ModuleRules
{
	public RosSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "RosBridge" });
		PrivateDependencyModuleNames.AddRange(new[] { "ImageWrapper", "Projects", "RenderCore", "RHI" }); // the camera's JPEG, shader and GPU readback
	}
}
