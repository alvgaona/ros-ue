#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "RosMessages.h"
#include "RosPublisher.h"

class UGameInstance;

namespace ros
{
	// Sim time from the game instance's world, published on /clock once per frame before actors tick, so no stamp runs
	// ahead of it. The plugin's only source of time: another clock mode would change only this class.
	class ROSBRIDGE_API Clock
	{
	public:
		Clock(const UGameInstance& InGameInstance, Publisher<rosgraph_msgs::msg::Clock> InPublisher);
		~Clock();
		Clock(const Clock&) = delete;
		Clock& operator=(const Clock&) = delete;

		builtin_interfaces::msg::Time Now() const;

	private:
		void Tick(UWorld* World, ELevelTick, float);

		const UGameInstance& GameInstance;
		Publisher<rosgraph_msgs::msg::Clock> ClockPublisher;
		FDelegateHandle TickHandle;
	};

	// The sim time of WorldContext's game instance, for message stamps.
	ROSBRIDGE_API builtin_interfaces::msg::Time Now(const UObject* WorldContext);
}
