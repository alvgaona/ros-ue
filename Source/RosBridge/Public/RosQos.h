#pragma once

#include "CoreMinimal.h"

namespace ros
{
	enum class ReliabilityPolicy
	{
		Reliable,
		BestEffort
	};
	enum class DurabilityPolicy
	{
		Volatile,
		TransientLocal
	};

	// rclcpp's QoS: keep last Depth, reliable and volatile until set otherwise. A plain depth converts to one, as in rclcpp.
	struct Qos
	{
		Qos(int32 InDepth)
			: Depth(InDepth)
		{}

		Qos& KeepLast(int32 InDepth)
		{
			Depth = InDepth;
			return *this;
		}
		Qos& Reliable()
		{
			Reliability = ReliabilityPolicy::Reliable;
			return *this;
		}
		Qos& BestEffort()
		{
			Reliability = ReliabilityPolicy::BestEffort;
			return *this;
		}
		Qos& DurabilityVolatile()
		{
			Durability = DurabilityPolicy::Volatile;
			return *this;
		}
		Qos& TransientLocal()
		{
			Durability = DurabilityPolicy::TransientLocal;
			return *this;
		}

		int32 Depth;
		ReliabilityPolicy Reliability = ReliabilityPolicy::Reliable;
		DurabilityPolicy Durability = DurabilityPolicy::Volatile;
	};

	// rclcpp's SensorDataQoS: best effort, keep last 5
	struct SensorDataQos : Qos
	{
		SensorDataQos()
			: Qos(5)
		{ BestEffort(); }
	};

	// tf2_ros's DynamicBroadcasterQoS: keep last 100
	struct DynamicBroadcasterQos : Qos
	{
		DynamicBroadcasterQos()
			: Qos(100)
		{}
	};

	// tf2_ros's StaticBroadcasterQoS: transient local, keep last 1
	struct StaticBroadcasterQos : Qos
	{
		StaticBroadcasterQos()
			: Qos(1)
		{ TransientLocal(); }
	};
}
