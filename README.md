# ue-ros-bridge

Unreal Engine 5 plugin that talks to ROS 2 over DDS without being a ROS 2 node. Publishers and subscriptions work like rclcpp, and callbacks run on the game thread.

Topics work with ROS 2 running either Cyclone DDS or Fast DDS. Only macOS and Linux are wired up.

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

On macOS, ROS only sees the editor if it has Local Network access (System Settings → Privacy & Security → Local Network). An editor started from a terminal uses the terminal's access instead.

## Adding a message type

`pixi run setup` generates every message in the packages listed in `setup.sh`, from the IDL that ROS ships in the pixi environment. To use one, add `#include "<pkg>_<Type>.h"` and a `ROS_MESSAGE` line to `Source/RosBridge/Public/RosMessages.h`.

For a package that isn't listed, run `pixi add ros-jazzy-<pkg>`, add it to `PACKAGES` in `setup.sh` and run `pixi run setup` again.
