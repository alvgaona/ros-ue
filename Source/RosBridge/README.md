# RosBridge

The ROS 2 client: publishers, subscriptions, the clock and TF. It speaks DDS directly, so it needs no ROS install and never shows up as a ROS node. Add `RosBridge` to your module's dependencies in its `.Build.cs`, then include `Ros.h` and `RosMessages.h`.

## Message types

`pixi run setup` generates every message in the packages listed in `Scripts/setup.sh`, from the IDL that ROS Jazzy ships in the default pixi environment. Use them under their ROS 2 names, as in rclcpp:

```cpp
ros::Publisher<geometry_msgs::msg::Twist> Publisher = ros::CreatePublisher<geometry_msgs::msg::Twist>(this, TEXT("/cmd_vel"));
```

The messages match the other distros on the wire, except `sensor_msgs/Range`, which has no `variance` field in Humble.

For a package that isn't listed, run `pixi add --feature jazzy ros-jazzy-<pkg>`, add it to `PACKAGES` in `Scripts/setup.sh` and run `pixi run setup` again.

## QoS

Publishers and subscriptions default to rclcpp's QoS: reliable, volatile, keep last 10. Pass a depth or a `ros::Qos` to change it, as with `rclcpp::QoS`:

```cpp
ros::Publisher<sensor_msgs::msg::Image> Camera = ros::CreatePublisher<sensor_msgs::msg::Image>(this, TEXT("/image_raw"), ros::SensorDataQos().KeepLast(1));
```

`ros::SensorDataQos`, `ros::DynamicBroadcasterQos` and `ros::StaticBroadcasterQos` copy the rclcpp and tf2_ros presets of the same names. A reliable subscription doesn't match a best-effort publisher, so match what the ROS side subscribes with.

## Time

Unreal governs time. Each game instance publishes its game time on `/clock` every frame, so run ROS nodes with `--ros-args -p use_sim_time:=true`. ROS time then holds while the game is paused, follows time dilation, and starts again from zero on each Play. Stamp messages with the same time:

```cpp
Message.header.stamp = ros::Now(this);
```

In the editor, turn off Editor Preferences → General → Performance → Use Less CPU when in Background. It is on by default and holds the editor to 3 frames per second whenever another app, such as a terminal or RViz, is in front, so the simulation and `/clock` step only three times a second.

Each frame moves game time on by however long it took, so `/clock` steps vary with the frame rate. For a fixed step, turn on Project Settings → Engine → General Settings → Framerate → Use Fixed Frame Rate. Every frame then moves game time on by exactly 1/rate, and a frame that runs long leaves game time behind the wall clock instead of taking a bigger step.

## TF

A transform broadcaster works like tf2_ros's. It sends frames on `/tf`, converted to ROS and stamped with the time `/clock` carries, and all the frames of one call go out as one message:

```cpp
// In BeginPlay, and clear both with `= {}` in EndPlay
Broadcaster = ros::TransformBroadcaster(this);
Static = ros::StaticTransformBroadcaster(this);
Static.SendTransform({ TEXT("base_link"), TEXT("lidar"), Lidar->GetRelativeTransform() });

// Every tick
Broadcaster.SendTransform({
	{ TEXT("world"), TEXT("base_link"), GetActorTransform() },
	{ TEXT("base_link"), TEXT("arm"), Arm->GetRelativeTransform() },
});
```

Frames that never move, such as a sensor mount, go through the static broadcaster on `/tf_static`, which ROS nodes that start later still receive.

Send the pose of anything physics moves from a tick in `TG_PostPhysics` or later. Before physics runs, a body still has the previous frame's pose while the stamp already has this frame's time, so its transform would arrive a frame late.

ROS nodes can publish into the same tree, such as `map → odom` from localization. That works when:

- Every frame has exactly one publisher.
- ROS nodes that publish moving transforms run with `use_sim_time:=true`. Their wall-clock stamps can't be combined with Unreal's otherwise.
- A transform about something Unreal simulates agrees with where Unreal has it, for example by building both from the same URDF.

## Large messages

A raw 1080p image in `bgra8` is 8.3 MB, which DDS sends as thousands of UDP fragments in one burst. A subscriber whose socket receive buffer can't hold the burst loses fragments: reliable QoS resends them at a lower rate, and best effort loses the whole message. Cyclone DDS asks for a 1 MiB receive buffer unless told otherwise, so give ROS nodes that subscribe to images a bigger one:

```sh
export CYCLONEDDS_URI='<CycloneDDS><Domain><Internal><SocketReceiveBufferSize max="14MiB"/></Internal></Domain></CycloneDDS>'
```

The operating system caps what a socket can get. macOS allows about 7 MiB by default, and `sudo sysctl -w kern.ipc.maxsockbuf=16777216` raises that to about 14 MiB, the most macOS accepts. On Linux, raise `net.core.rmem_max`. Measured on one Mac with Cyclone DDS on both sides, at 30 images a second:

| Receive buffer | What arrives |
|---|---|
| 1 MiB, the default | 1080p with reliable QoS. Best effort loses images even at 640×480. |
| 7 MiB | 1080p with any QoS. |
| 14 MiB | 4K with reliable QoS and keep last 1. |

Publishing holds Unreal's game thread for about 0.45 ms per MB, 3 ms for a 1080p image. Over a network, bandwidth runs out first: gigabit Ethernet carries about 15 raw 1080p images a second.
