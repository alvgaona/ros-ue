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

## Adding a message type

`pixi run setup` generates every message in the packages listed in `setup.sh`, from the IDL that ROS Jazzy ships in the default pixi environment. To use one, add `#include "<pkg>_<Type>.h"` and a `ROS_MESSAGE` line to `Source/RosBridge/Public/RosMessages.h`.

The messages match the other distros on the wire, except `sensor_msgs/Range`, which has no `variance` field in Humble.

For a package that isn't listed, run `pixi add --feature jazzy ros-jazzy-<pkg>`, add it to `PACKAGES` in `setup.sh` and run `pixi run setup` again.
