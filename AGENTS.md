# ue-ros-bridge

Unreal Engine 5 plugin, `RosBridge`, that talks to ROS 2 as a plain Cyclone DDS participant. Read `README.md` first; it is the user-facing spec.

## Constraints

- It is not a ROS 2 node and must not become one. Don't add rclcpp, rcl or rmw. Building it needs no ROS install, and ROS in `pixi.toml` is only the test peer.
- The API follows rclcpp's shape and names, and each `ERosQos` profile copies an rclcpp one.
- Callbacks run on the game thread because `URos::Tick` drains every reader each frame. Don't move them to DDS listeners, which fire on Cyclone's threads.
- Cyclone DDS is linked statically and pinned in `setup.sh` to 0.10.5, the release ROS 2 Jazzy ships. Move the pin with the distro in `pixi.toml`.
- macOS arm64 and Linux x86_64 only, the platforms in `pixi.toml`.

## Layout

- `Source/RosBridge/Public/Ros.h` is the whole API: the `URos` game-instance subsystem, `TPublisher`, `TSubscription`, `ERosQos` and `ROS_MESSAGE`. `Private/Ros.cpp` sets up the participant and QoS, names topics and spins readers.
- `Source/RosBridge/Public/RosMessages.h` registers the message types the plugin uses, each with one include, one `ROS_MESSAGE` line and a short `F` alias.
- `HelloRos` is `demo_nodes_cpp`'s talker and listener in one actor, and the end-to-end check.
- `setup.sh`, run as `pixi run setup`, builds Cyclone DDS and every message in its `PACKAGES` into `ThirdParty/`. That directory is generated and ignored; don't edit it.
- `idl.py` rewrites the IDL that ROS ships in the pixi environment (`share/<pkg>/msg/`) under the names ROS 2 uses on the wire. Never hand-write message IDL.

## Things that bite

- ROS 2 matches on mangled names. The topic `/chatter` is `rt/chatter` on the wire (`URos::MakeTopic` adds the prefix), and `std_msgs/msg/String` is `std_msgs::msg::dds_::String_` (`idl.py` renames it). Get either wrong and nothing matches, with no error.
- Keep `-x final` in `setup.sh` even though idlc 0.10.5 defaults to it. idlc warns the default may become appendable, and ROS 2 messages are final.
- The setup script skips Cyclone when `ThirdParty/cyclonedds/lib/libddsc.a` exists. After changing its version or CMake flags, delete `ThirdParty/cyclonedds` and rerun. `ThirdParty/msgs` is rebuilt on every run.
- Dropping the pointer that `CreatePublisher` or `CreateSubscription` returns deletes the endpoint, since `URos` keeps only weak pointers. Hold it in a member and reset it in `EndPlay`, as `HelloRos` does.
- Cyclone serializes a sample inside `dds_write`, so message fields can point at temporaries such as `TCHAR_TO_UTF8`.
- ROS nodes on Cyclone log `Failed to parse type hash` once per Unreal endpoint, because the plugin sends no type hash in USER_DATA. It is harmless; topics still match.
- Unreal reads `ROS_DOMAIN_ID` from its own environment. On macOS an editor launched from the Dock doesn't inherit the shell's and runs on domain 0.

## Verifying changes

There is no test suite. Build in a host project and run the README's hello world against `pixi run listener` and `pixi run talker`. Fast DDS is supported too, so repeat the listener against it:

```sh
pixi run env RMW_IMPLEMENTATION=rmw_fastrtps_cpp ros2 run demo_nodes_cpp listener
```

The pixi environment sets `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` over the shell's value, hence `env` after activation. Unreal never shows in `ros2 node list`, since it isn't a node. Its endpoints appear in `pixi run ros2 topic info /chatter -v` under `_NODE_NAME_UNKNOWN_`.

## Style

- Unreal C++ conventions: tabs, `F`/`T`/`U`/`A`/`E` prefixes, Unreal containers and strings. Wrap Cyclone and generated message headers in `THIRD_PARTY_INCLUDES_START`/`END`.
- Comments are one line and only for a non-obvious constraint. A `ponytail:` comment marks a deliberate shortcut and names its upgrade path.
- Conventional commits. Markdown is not hard-wrapped.
