#pragma once

#include "CoreMinimal.h"
#include "RosQos.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

namespace ros
{
	struct QosDeleter
	{
		void operator()(dds_qos_t* Profile) const { dds_delete_qos(Profile); }
	};

	TUniquePtr<dds_qos_t, QosDeleter> MakeQos(const Qos& Profile, const char* TypeHash);
}
