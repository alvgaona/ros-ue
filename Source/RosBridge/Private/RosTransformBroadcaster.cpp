#include "RosTransformBroadcaster.h"
#include "Ros.h"
#include "RosConversions.h"

namespace ros
{
	namespace
	{
		FString FrameId(const FString& Id)
		{
			return Id.StartsWith(TEXT("/")) ? Id.RightChop(1) : Id; // tf2 frame ids never start with a slash
		}
	}

	TransformBroadcaster::TransformBroadcaster(const UObject* InWorldContext)
		: Tf(CreatePublisher<tf2_msgs::msg::TFMessage>(InWorldContext, TEXT("/tf"), Qos::DynamicBroadcaster))
		, WorldContext(InWorldContext)
	{
	}

	void TransformBroadcaster::SendTransform(const Frame& InFrame) const
	{
		SendTransform(MakeArrayView(&InFrame, 1));
	}

	void TransformBroadcaster::SendTransform(TConstArrayView<Frame> Frames) const
	{
		const builtin_interfaces::msg::Time Stamp = Now(WorldContext.Get());
		TArray<TArray<ANSICHAR>> Ids; // the message points into these until it's written
		const auto Utf8 = [&Ids](const FString& Id)
		{
			const FTCHARToUTF8 Converted(*Id);
			return Ids.Emplace_GetRef(reinterpret_cast<const ANSICHAR*>(Converted.Get()), Converted.Length() + 1).GetData();
		};
		TArray<geometry_msgs::msg::TransformStamped> Transforms;
		for (const Frame& Each : Frames)
		{
			const FString Parent = FrameId(Each.Parent);
			const FString Child = FrameId(Each.Child);
			// tf2 would drop it on the ROS side, where nobody in Unreal sees the error
			if (!ensureMsgf(!Parent.IsEmpty() && !Child.IsEmpty() && Parent != Child, TEXT("Not sending transform from '%s' to '%s': tf2 needs two different frame ids"), *Each.Parent, *Each.Child))
				continue;
			Transforms.Add({ { Stamp, Utf8(Parent) }, Utf8(Child), ToTransform(Each.Transform) });
		}
		if (Transforms.IsEmpty())
			return;
		tf2_msgs::msg::TFMessage Message{};
		Message.transforms._maximum = Message.transforms._length = Transforms.Num();
		Message.transforms._buffer = Transforms.GetData();
		Tf.Publish(Message);
	}
}
