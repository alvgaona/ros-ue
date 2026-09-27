#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

namespace ros
{
	// What DDS needs to carry a message type: its descriptor and its ROS 2 type hash.
	struct TypeSupport
	{
		const dds_topic_descriptor_t* Descriptor;
		const char* Hash;
	};

	template<class T>
	TypeSupport TypeSupportOf();
}

#define ROS_MESSAGE(T)                                                                \
	namespace ros                                                                     \
	{                                                                                 \
		template<>                                                                    \
		inline TypeSupport TypeSupportOf<T>() { return { &T##_desc, T##_typehash }; } \
	}
