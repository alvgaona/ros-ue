#pragma once

#include "Ros.h"

// One include and one ROS_MESSAGE line per type in msg/.
THIRD_PARTY_INCLUDES_START
#include "std_msgs_String.h"
THIRD_PARTY_INCLUDES_END

ROS_MESSAGE(std_msgs_msg_dds__String_)
using FStringMsg = std_msgs_msg_dds__String_;
