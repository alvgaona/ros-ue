#include "RosClock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Ros.h"
#include "RosConversions.h"

namespace ros
{
	Clock::Clock(const UGameInstance& InGameInstance, Publisher<rosgraph_msgs::msg::Clock> InPublisher)
		: GameInstance(InGameInstance), ClockPublisher(MoveTemp(InPublisher))
	{
		// Fires once the frame's game time has advanced and before any actor ticks, and not while paused
		TickHandle = FWorldDelegates::OnWorldPreActorTick.AddRaw(this, &Clock::Tick);
	}

	Clock::~Clock()
	{
		FWorldDelegates::OnWorldPreActorTick.Remove(TickHandle);
	}

	builtin_interfaces::msg::Time Clock::Now() const
	{
		const UWorld* World = GameInstance.GetWorld();
		return ToTime(World ? World->GetTimeSeconds() : 0.0);
	}

	void Clock::Tick(UWorld* World, ELevelTick, float)
	{
		// Every world ticks through this delegate, the editor's included
		if (World->GetGameInstance() == &GameInstance)
			ClockPublisher.Publish({ ToTime(World->GetTimeSeconds()) });
	}

	builtin_interfaces::msg::Time Now(const UObject* WorldContext)
	{
		const URos* Ros = URos::Get(WorldContext);
		return Ros ? Ros->Now() : builtin_interfaces::msg::Time{};
	}
}
