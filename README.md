# ue-ros-bridge

[![CI](https://github.com/alvgaona/ue-ros-bridge/actions/workflows/ci.yaml/badge.svg)](https://github.com/alvgaona/ue-ros-bridge/actions/workflows/ci.yaml)
![Unreal Engine 5](https://img.shields.io/badge/Unreal_Engine-5-313131?logo=unrealengine&logoColor=ffffff)
![ROS 2](https://img.shields.io/badge/ROS_2-Humble_%7C_Jazzy_%7C_Kilted_%7C_Lyrical-22314e?logo=ros&logoColor=ffffff)

Unreal Engine 5 plugin that talks to ROS 2 over DDS without being a ROS 2 node. Publishers and subscriptions work like rclcpp, and callbacks run on the game thread.

It has two modules. RosBridge is the ROS 2 client: publishers, subscriptions, the clock and TF. RosSim adds the components that take real work to build on top of it, a camera for now. Anything else, from an IMU to a vehicle, you model yourself and publish through RosBridge.

Topics work with ROS 2 Humble, Jazzy, Kilted and Lyrical, running either Cyclone DDS or Fast DDS. ROS on rmw_zenoh can't see the plugin, since Zenoh isn't DDS. Only macOS and Linux are wired up.

## Setup

You need [pixi](https://pixi.sh) and the compiler Unreal uses (Xcode on macOS, clang on Linux).

```sh
pixi run setup   # builds Cyclone DDS and the ROS message types as static libraries into ThirdParty/
```

Put this folder in a C++ project's `Plugins/` folder (for example `Plugins/RosBridge`), open the project and let it build. To use it from C++, add `RosBridge` to your module's dependencies in its `.Build.cs`, and `RosSim` too for the camera.

## Hello world

Drop a `HelloRos` actor in a level (Window → Place Actors, search for "Hello") and press Play. It publishes `Hello World: N` on `/chatter` every second and logs everything it hears there, its own messages included. Like the plugin's other check actors, it exists only in the editor and never ends up in a packaged game.

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

## QoS

Publishers and subscriptions default to rclcpp's QoS: reliable, volatile, keep last 10. Pass a depth or a `ros::Qos` to change it, as with `rclcpp::QoS`:

```cpp
ros::Publisher<sensor_msgs::msg::Image> Camera = ros::CreatePublisher<sensor_msgs::msg::Image>(this, TEXT("/image_raw"), ros::SensorDataQos().KeepLast(1));
```

`ros::SensorDataQos`, `ros::DynamicBroadcasterQos` and `ros::StaticBroadcasterQos` copy the rclcpp and tf2_ros presets of the same names. A reliable subscription doesn't match a best-effort publisher, so match what the ROS side subscribes with.

## Images

A raw 1080p image in `bgra8` is 8.3 MB, which DDS sends as thousands of UDP fragments in one burst. A subscriber whose socket receive buffer can't hold the burst loses fragments: reliable QoS resends them at a lower rate, and best effort loses the whole image. Cyclone DDS asks for a 1 MiB receive buffer unless told otherwise, so give ROS nodes that subscribe to images a bigger one:

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

## Cameras

A Ros Camera component publishes what it sees as `sensor_msgs/Image` in `bgr8`: by default 640×480 on `/camera/image_raw`, 30 images per second of sim time, in the frame `camera_optical_frame`. Add it to an actor in the editor (Add → Ros Camera) or in C++, and change those in its Ros properties. It is a scene capture underneath, so field of view and post-processing work as on any scene capture.

Each image carries the sim time of the frame it was captured in, taken once everything in that frame has moved, physics included. Rendering the camera's view costs what any second view of the scene costs, but nothing waits for the image: the GPU copies it back while the game carries on, and a worker thread publishes it. If the worker can't keep up, as with large images on a slow network, the camera drops older images so the newest goes out next.

It also publishes every image as JPEG on `<Topic>/compressed`, as ROS cameras do through `image_transport`, so a node can subscribe to either. RViz's Image display and `rqt_image_view` show it with their transport set to compressed, which needs the `compressed_image_transport` plugin on the ROS side. Like `image_transport`, the camera sends each only while something subscribes to it, so a JPEG is encoded only for a subscriber. Turn off Raw or Compressed to leave that topic out altogether; JpegQuality is 95 by default, as in `image_transport`.

JPEG is about twenty times smaller, so it needs none of the socket buffer tuning above. In Unreal's Open World template, a 1080p image was 0.34 MB instead of 6.2 MB, and a 4K one 1.1 MB instead of 25 MB. With Cyclone's default buffer, 4K JPEG arrived at the camera's full 30 images a second, where raw 4K managed 13.

It publishes with the default QoS. For another, set `Qos` before BeginPlay:

```cpp
Camera = CreateDefaultSubobject<URosCameraComponent>(TEXT("Camera"));
Camera->Qos = ros::SensorDataQos();
```

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

## Contributing

Commits and PR titles follow [Conventional Commits](https://www.conventionalcommits.org/). `bun install` installs a commit-msg hook that checks them. [`AGENTS.md`](AGENTS.md) has the constraints, the layout and how to verify a change in Unreal, and [`.github/workflows/README.md`](.github/workflows/README.md) covers CI and releases.
