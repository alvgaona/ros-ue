#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

enum class RosQos { Reliable, SensorData };

struct QosDeleter { void operator()(dds_qos_t* Qos) const { dds_delete_qos(Qos); } };

ROSBRIDGE_API TUniquePtr<dds_qos_t, QosDeleter> MakeQos(RosQos Qos, const char* TypeHash);
