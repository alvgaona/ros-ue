#pragma once

#include "Ros.h"

// One include and one ROS_MESSAGE line per type used here. setup.sh generates every type in its packages.
THIRD_PARTY_INCLUDES_START
#include "geometry_msgs_Twist.h"
#include "nav_msgs_Odometry.h"
#include "rosgraph_msgs_Clock.h"
#include "std_msgs_String.h"
#include "tf2_msgs_TFMessage.h"
THIRD_PARTY_INCLUDES_END

ROS_MESSAGE(geometry_msgs_msg_dds__Twist_)
using FTwistMsg = geometry_msgs_msg_dds__Twist_;

ROS_MESSAGE(nav_msgs_msg_dds__Odometry_)
using FOdometryMsg = nav_msgs_msg_dds__Odometry_;

ROS_MESSAGE(rosgraph_msgs_msg_dds__Clock_)
using FClockMsg = rosgraph_msgs_msg_dds__Clock_;

ROS_MESSAGE(std_msgs_msg_dds__String_)
using FStringMsg = std_msgs_msg_dds__String_;

ROS_MESSAGE(tf2_msgs_msg_dds__TFMessage_)
using FTFMessage = tf2_msgs_msg_dds__TFMessage_;
