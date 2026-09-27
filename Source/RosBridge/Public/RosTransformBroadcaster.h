#pragma once

#include "CoreMinimal.h"
#include "RosMessages.h"
#include "RosPublisher.h"
#include "RosQos.h"

namespace ros
{
	// A frame's pose in its parent frame, in Unreal's units and axes.
	struct Frame
	{
		FString Parent;
		FString Child;
		FTransform Transform;
	};

	// tf2_ros's TransformBroadcaster: sends frames on /tf in ROS terms, stamped with the time /clock carries. Move-only.
	class ROSBRIDGE_API TransformBroadcaster
	{
	public:
		TransformBroadcaster() = default;
		explicit TransformBroadcaster(const UObject* WorldContext);

		// One message, one stamp. Physics runs after TG_PrePhysics, so send what it moves from TG_PostPhysics or later.
		void SendTransform(TConstArrayView<Frame> Frames) const;
		void SendTransform(const Frame& InFrame) const;

	private:
		friend class StaticTransformBroadcaster;
		TransformBroadcaster(const UObject* WorldContext, const TCHAR* Topic, Qos Profile);

		Publisher<tf2_msgs::msg::TFMessage> Tf;
		TWeakObjectPtr<const UObject> WorldContext;
	};
}
