#include "RosConversions.h"

namespace ros
{
	geometry_msgs::msg::Vector3 ToVector3(const FVector& Vector)
	{
		return { Vector.X / 100.0, -Vector.Y / 100.0, Vector.Z / 100.0 };
	}

	geometry_msgs::msg::Quaternion ToQuaternion(const FQuat& Rotation)
	{
		// Mirroring Y reverses the sense of rotations about X and Z
		return { -Rotation.X, Rotation.Y, -Rotation.Z, Rotation.W };
	}

	geometry_msgs::msg::Transform ToTransform(const FTransform& Transform)
	{
		return { ToVector3(Transform.GetTranslation()), ToQuaternion(Transform.GetRotation()) };
	}

	builtin_interfaces::msg::Time ToTime(double Seconds)
	{
		// Round once, on the total, so 1.9999999999 s carries into 2 s
		const int64 Nanoseconds = FMath::RoundToInt64(Seconds * 1e9);
		return { static_cast<int32_t>(Nanoseconds / 1000000000), static_cast<uint32_t>(Nanoseconds % 1000000000) };
	}
}
