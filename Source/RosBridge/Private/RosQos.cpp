#include "RosQos.h"

namespace ros
{
	TUniquePtr<dds_qos_t, QosDeleter> MakeQos(Qos Profile, const char* TypeHash)
	{
		dds_qos_t* Result = dds_create_qos();
		switch (Profile)
		{
		case Qos::Reliable: // rclcpp default: reliable, keep last 10
			dds_qset_reliability(Result, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
			dds_qset_history(Result, DDS_HISTORY_KEEP_LAST, 10);
			break;
		case Qos::SensorData: // rclcpp SensorDataQoS: best effort, keep last 5
			dds_qset_reliability(Result, DDS_RELIABILITY_BEST_EFFORT, 0);
			dds_qset_history(Result, DDS_HISTORY_KEEP_LAST, 5);
			break;
		}
		// ROS 2 nodes read the type hash from USER_DATA and warn when it's missing
		const FTCHARToUTF8 UserData(*FString::Printf(TEXT("typehash=%s;"), UTF8_TO_TCHAR(TypeHash)));
		dds_qset_userdata(Result, UserData.Get(), UserData.Length());
		return TUniquePtr<dds_qos_t, QosDeleter>(Result);
	}
}
