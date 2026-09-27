#pragma once

#include "CoreMinimal.h"
#include "RosTransformBroadcaster.h"

namespace ros
{
	// tf2_ros's StaticTransformBroadcaster: frames that never move, on /tf_static, which late subscribers still get. Move-only.
	class ROSBRIDGE_API StaticTransformBroadcaster
	{
	public:
		StaticTransformBroadcaster() = default;
		explicit StaticTransformBroadcaster(const UObject* WorldContext);

		// Adds frames, replacing any with the same child, then sends them all: a late subscriber only gets the last message.
		void SendTransform(TConstArrayView<Frame> Frames);
		void SendTransform(const Frame& InFrame);

	private:
		TransformBroadcaster Tf;
		TArray<Frame> Sent;
	};
}
