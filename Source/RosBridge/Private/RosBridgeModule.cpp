#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Ros.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(LogRos);

class RosBridgeModule : public IModuleInterface
{
	virtual void StartupModule() override
	{
		// For the camera's shader, which is also why the module loads at PostConfigInit
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/RosBridge"), FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RosBridge"))->GetBaseDir(), TEXT("Shaders")));
	}
};

IMPLEMENT_MODULE(RosBridgeModule, RosBridge)
