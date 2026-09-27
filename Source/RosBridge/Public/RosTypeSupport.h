#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
THIRD_PARTY_INCLUDES_END

// What DDS needs to carry a message type: its descriptor and its ROS 2 type hash.
struct RosTypeSupport
{
	const dds_topic_descriptor_t* Descriptor;
	const char* Hash;
};

template<class T> RosTypeSupport RosTypeSupportOf();
#define ROS_MESSAGE(T) \
	template<> inline RosTypeSupport RosTypeSupportOf<T>() { return { &T##_desc, T##_typehash }; }
