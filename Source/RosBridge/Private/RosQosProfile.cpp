#include "RosQosProfile.h"

namespace ros
{
	TUniquePtr<dds_qos_t, QosDeleter> MakeQos(const Qos& Profile, const char* TypeHash)
	{
		dds_qos_t* Result = dds_create_qos();
		const bool Reliable = Profile.Reliability == ReliabilityPolicy::Reliable;
		dds_qset_reliability(Result, Reliable ? DDS_RELIABILITY_RELIABLE : DDS_RELIABILITY_BEST_EFFORT, DDS_MSECS(100));
		const bool TransientLocal = Profile.Durability == DurabilityPolicy::TransientLocal;
		dds_qset_durability(Result, TransientLocal ? DDS_DURABILITY_TRANSIENT_LOCAL : DDS_DURABILITY_VOLATILE);
		dds_qset_history(Result, DDS_HISTORY_KEEP_LAST, Profile.Depth);
		// ROS 2 nodes read the type hash from USER_DATA and warn when it's missing
		const FTCHARToUTF8 UserData(*FString::Printf(TEXT("typehash=%s;"), UTF8_TO_TCHAR(TypeHash)));
		dds_qset_userdata(Result, UserData.Get(), UserData.Length());
		return TUniquePtr<dds_qos_t, QosDeleter>(Result);
	}
}
