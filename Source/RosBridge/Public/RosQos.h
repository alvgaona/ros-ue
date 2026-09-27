#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

namespace ros
{
	enum class Qos { Reliable, SensorData };

	struct QosDeleter { void operator()(dds_qos_t* Profile) const { dds_delete_qos(Profile); } };

	ROSBRIDGE_API TUniquePtr<dds_qos_t, QosDeleter> MakeQos(Qos Profile, const char* TypeHash);
}
