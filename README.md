# ue-ros-bridge

Unreal Engine 5 plugin that talks to ROS 2 over DDS without being a ROS 2 node. Publishers and subscriptions work like rclcpp, and callbacks run on the game thread.

Topics work with ROS 2 running either Cyclone DDS or Fast DDS. Only macOS and Linux are wired up.

## Setup

You need [pixi](https://pixi.sh) and the compiler Unreal uses (Xcode on macOS, clang on Linux).

```sh
pixi run setup   # builds Cyclone DDS and msg/*.idl as static libraries into ThirdParty/
```

Put this folder in a C++ project's `Plugins/` folder (for example `Plugins/RosBridge`), open the project and let it build.

## Hello world

Drop a `HelloRos` actor in a level and press Play. It publishes `Hello World: N` on `/chatter` every second and logs everything it hears there, its own messages included.

```sh
pixi run listener   # prints what Unreal publishes
pixi run talker     # Unreal logs "I heard: [...]"
```

## Adding a message type

1. Copy the type's IDL from a ROS install (`share/<pkg>/msg/<Type>.idl`) to `msg/<pkg>_<Type>.idl`, along with the types it uses. Wrap it in `module dds_`, add `_` to the struct name and mark it `@final`, as in `msg/std_msgs_String.idl`.
2. Run `pixi run setup`.
3. Add its include and a `ROS_MESSAGE` line to `Source/RosBridge/Public/RosMessages.h`.
