#pragma once

#include "CoreMinimal.h"
#include "RosMessages.h"

// Unreal to ROS. Unreal is fixed to left-handed centimeters (X forward, Y right, Z up) and ROS to right-handed
// meters (X forward, Y left, Z up), so this never changes. Epic's GeoReferencing plugin converts the same way.
namespace ros
{
	// Positions, translations and linear velocities. Angular velocities need their own conversion.
	ROSBRIDGE_API geometry_msgs::msg::Vector3 ToVector3(const FVector& Vector);
	ROSBRIDGE_API geometry_msgs::msg::Quaternion ToQuaternion(const FQuat& Rotation);
	ROSBRIDGE_API geometry_msgs::msg::Transform ToTransform(const FTransform& Transform);
	ROSBRIDGE_API builtin_interfaces::msg::Time ToTime(double Seconds);
}
