# ue-ros-bridge

Unreal Engine 5 plugin that talks to ROS 2 over DDS without being a ROS 2 node. Publishers and subscriptions work like rclcpp, and callbacks run on the game thread.

Topics work with ROS 2 Humble, Jazzy, Kilted and Lyrical, running either Cyclone DDS or Fast DDS. ROS on rmw_zenoh can't see the plugin, since Zenoh isn't DDS. Only macOS and Linux are wired up.

## Setup

You need [pixi](https://pixi.sh) and the compiler Unreal uses (Xcode on macOS, clang on Linux).

```sh
pixi run setup   # builds Cyclone DDS and the ROS message types as static libraries into ThirdParty/
```

Put this folder in a C++ project's `Plugins/` folder (for example `Plugins/RosBridge`), open the project and let it build.

## Hello world

Drop a `HelloRos` actor in a level (Window → Place Actors, search for "Hello") and press Play. It publishes `Hello World: N` on `/chatter` every second and logs everything it hears there, its own messages included.

```sh
pixi run listener   # prints what Unreal publishes
pixi run talker     # Unreal logs "I heard: [...]"
```

These run Jazzy. Add `-e humble`, `-e kilted` or `-e lyrical` after `pixi run` to try another distro. Lyrical's demo nodes use `example_interfaces/msg/String`, so for Lyrical use `pixi run -e lyrical ros2 topic echo /chatter std_msgs/msg/String` instead.

On macOS, ROS only sees the editor if it has Local Network access (System Settings → Privacy & Security → Local Network). An editor started from a terminal uses the terminal's access instead.

## Message types

`pixi run setup` generates every message in the packages listed in `Scripts/setup.sh`, from the IDL that ROS Jazzy ships in the default pixi environment. Include `RosMessages.h` and use them under their ROS 2 names, as in rclcpp:

```cpp
ros::Publisher<geometry_msgs::msg::Twist> Publisher = ros::CreatePublisher<geometry_msgs::msg::Twist>(this, TEXT("/cmd_vel"));
```

The messages match the other distros on the wire, except `sensor_msgs/Range`, which has no `variance` field in Humble.

For a package that isn't listed, run `pixi add --feature jazzy ros-jazzy-<pkg>`, add it to `PACKAGES` in `Scripts/setup.sh` and run `pixi run setup` again.

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
