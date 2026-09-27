#include "RosStaticTransformBroadcaster.h"

namespace ros
{
	StaticTransformBroadcaster::StaticTransformBroadcaster(const UObject* WorldContext)
		: Tf(WorldContext, TEXT("/tf_static"), Qos::StaticBroadcaster)
	{
	}

	void StaticTransformBroadcaster::SendTransform(const Frame& InFrame)
	{
		SendTransform(MakeArrayView(&InFrame, 1));
	}

	void StaticTransformBroadcaster::SendTransform(TConstArrayView<Frame> Frames)
	{
		for (const Frame& Each : Frames)
		{
			if (Frame* Same = Sent.FindByPredicate([&Each](const Frame& Old) { return Old.Child.Equals(Each.Child, ESearchCase::CaseSensitive); }))
				*Same = Each;
			else
				Sent.Add(Each);
		}
		Tf.SendTransform(Sent); // restamps the whole set, which tf2 ignores for static frames
	}
}
