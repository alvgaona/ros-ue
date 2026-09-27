# ros-ue

Unreal Engine 5 plugin, `RosBridge`, that talks to ROS 2 as a plain Cyclone DDS participant. Read `README.md` first; it is the user-facing spec.

## Constraints

- It is not a ROS 2 node and must not become one. Don't add rclcpp, rcl or rmw. Building it needs no ROS install, and ROS in `pixi.toml` is only the test peer.
- There is no node concept inside either. `URos`, one per game instance, owns the DDS participant and serves every publisher and subscription in that instance. If Unreal ever shows up in the ROS graph, it is one entry per game instance, with PIE clients numbered so names stay unique.
- The API follows rclcpp's shape and names. `ros::Qos` works like `rclcpp::QoS`, and each named preset copies an rclcpp or tf2_ros one.
- `RosBridge` is only the ROS 2 client. `RosSim` holds only components that are hard to build on it, the camera for now. Vehicles, controllers and simple sensors such as an IMU are not the library's: users model them and publish through `RosBridge`.
- Callbacks run on the game thread because `URos::Tick` drains every reader each frame. Don't move them to DDS listeners, which fire on Cyclone's threads.
- Cyclone DDS is linked statically and pinned in `Scripts/setup.sh` to 0.10.5, the release Humble, Jazzy and Kilted ship. Lyrical ships Cyclone 11, which talks to it fine.
- `pixi.toml` has one environment per distro. The default is Jazzy, the source of the generated messages, so `setup` exists only there; Humble ships no type hashes, which `idl.py` needs. `humble`, `kilted` and `lyrical` are test peers only.
- macOS arm64 and Linux x86_64 only, the platforms in `pixi.toml` and `RosBridge.uplugin`.

## Layout

One concept per file, named after the rclcpp or tf2_ros header it copies. Our types live in namespace `ros`; only UCLASSes stay outside it, because Unreal's header tool doesn't reflect namespaced types by default. File names keep a `Ros` prefix because Unreal shares include paths across plugins. Logic lives in `.cpp` files; a template only adds the typed surface on top, as rclcpp's `Publisher<T>` does on `PublisherBase`. Users include `Ros.h` and `RosMessages.h`. `Public/` holds only what users call or the templates need; everything else stays in `Private/`.

- `Source/RosBridge/Public/Ros.h` has `URos`, the game-instance subsystem that owns the DDS participant, creates publishers and subscriptions from type support, and spins them. After it come `ros::CreatePublisher<T>` and `ros::CreateSubscription<T>`, which find the `URos` of their world-context object's game instance and pass the message's type support on, and `ros::Now`, that game instance's time for message stamps. `Private/Ros.cpp` does the work and names topics. `LogRos` is declared here.
- `RosPublisher.h` has `ros::Publisher<T>`, a handle that only adds a typed `Publish` to `ros::PublisherBase`, which owns a DDS writer. `RosSubscription.h` has `ros::Subscription`, a handle to a `ros::Reader`, which owns a DDS reader and runs the take loop. Neither side is a template underneath, since `ros::CreateSubscription<T>` wraps the typed callback.
- `RosQos.h` has `ros::Qos`, rclcpp's `QoS`: a depth, which a plain number converts to, with reliability and durability set by chained calls. The presets `SensorDataQos`, `DynamicBroadcasterQos` and `StaticBroadcasterQos` are subclasses that only set fields, as in rclcpp. `Private/RosQosProfile.h` turns one into DDS QoS when an endpoint is created, with the type hash in USER_DATA.
- `RosConversions.h` turns Unreal values into ROS messages: centimeters to meters with Y flipped, rotations mirrored to match, and game time to `builtin_interfaces::msg::Time`. Unreal's axes and units can't be configured (World to Meters only scales VR), and Epic's GeoReferencing plugin converts the same way.
- `Private/RosClock.h` has `ros::Clock`, which `URos` owns and users never see, since a second one would publish a second `/clock`. Each frame it publishes the game instance's game time on `/clock` after the time advances and before any actor ticks, so no stamp is ahead of it. `ros::Now` returns that time. It is the plugin's only source of time, so a real-time or lockstep mode would change only this class.
- `RosTransformBroadcaster.h` has `ros::TransformBroadcaster`, tf2_ros's broadcaster taking `ros::Frame`s: one `/tf` message per call, every frame stamped with `ros::Now`. It strips a leading slash from frame ids and refuses frames tf2 would drop.
- `RosStaticTransformBroadcaster.h` has `ros::StaticTransformBroadcaster`, on `/tf_static` with `StaticBroadcasterQos`, which is transient local. Like tf2_ros's, it keeps every frame it was given and resends them all on each call, since a late subscriber only gets the last message.
- `Source/RosSim` is a second runtime module, built only on `RosBridge`'s public headers, as a user's module would be. Its `RosCameraComponent.h` has `URosCameraComponent`, a scene capture that publishes `sensor_msgs/Image` in `bgr8`. It captures in `TG_PostUpdateWork`, once everything has moved, and stamps the image with that frame's time. On the render thread, a compute shader, `Shaders/Private/RosCamera.usf`, packs each image into `bgr8` in a buffer, which `FRHIGPUBufferReadback` copies back without waiting on the GPU. A worker thread then publishes, one image at a time; while the worker is behind, only the newest ready image waits. Like `image_transport`, it sends raw on the topic and JPEG on `<topic>/compressed`, each only while something subscribes, and the `Raw` and `Compressed` flags leave either topic out. That state is `ros::CameraStream`, shared because render commands and the worker can outlive `EndPlay`. The shader is why `RosSim` loads at `PostConfigInit` and maps `/Plugin/RosBridge` in `StartupModule`: Unreal refuses global shaders from modules that load later. Keeping it apart leaves `RosBridge` without renderer dependencies, at the default loading phase.
- `RosTypeSupport.h` has `ROS_MESSAGE`, which gives a generated message struct its DDS descriptor and ROS 2 type hash. It and `RosQos.h` are the only public headers without a `.cpp`.
- `RosMessages.h` is generated into `ThirdParty/msgs/`. It gives every generated message its ROS 2 C++ name (`std_msgs::msg::String`) and registers it with `ROS_MESSAGE`, so there's nothing to register by hand.
- `Source/RosBridgeChecks` is a third module, `UncookedOnly` in `RosBridge.uplugin`, so the editor loads it and packaged games never contain it. It uses only `RosBridge`'s and `RosSim`'s public headers, as a user's module would. Its `HelloRos` is `demo_nodes_cpp`'s talker and listener in one actor, and the end-to-end check. Actors that only exist for checks go in it, and the Python that judges them goes in `Checks/` at the root, as with `ClockCheck` and `clock.py`, `TfCheck` with `tf.py` and `tf_static.py`, `ImageCheck` with `image.py`, or `CameraCheck` with `camera.py`.
- Its `Private/Tests/` has automation tests for code that needs no ROS, one behavior per test.
- `Scripts/setup.sh`, run as `pixi run setup`, builds Cyclone DDS and every message in its `PACKAGES` into `ThirdParty/`. That directory is generated and ignored; don't edit it.
- `Scripts/idl.py` rewrites the IDL that ROS ships in the pixi environment (`share/<pkg>/msg/`) under the names ROS 2 uses on the wire, and writes `RosMessages.h`. Never hand-write message IDL.

## Things that bite

- ROS 2 matches on mangled names. The topic `/chatter` is `rt/chatter` on the wire (`URos::MakeTopic` adds the prefix), and `std_msgs/msg/String` is `std_msgs::msg::dds_::String_` (`idl.py` renames it). Get either wrong and nothing matches, with no error.
- Keep `-x final` in `setup.sh` even though idlc 0.10.5 defaults to it. idlc warns the default may become appendable, and ROS 2 messages are final.
- Keep type discovery off in `setup.sh`, so endpoints match on type names, as `rmw_cyclonedds`'s do. With it on, idlc embeds XTypes type objects in every message and Cyclone 0.10.5 matches on those instead. Kilted's Fast DDS 3.2 then never matched `tf2_msgs/msg/TFMessage`, and in one run Cyclone's discovery thread hung validating a type object, after which Unreal matched nothing new and froze on exit in `dds_delete`.
- The setup script skips Cyclone when `ThirdParty/cyclonedds/lib/libddsc.a` exists. After changing its version or CMake flags, delete `ThirdParty/cyclonedds` and rerun. `ThirdParty/msgs` is rebuilt on every run.
- `ros::CreatePublisher` and `ros::CreateSubscription` return move-only handles, and the endpoint lives exactly as long as its handle; `URos` only keeps weak pointers to readers. Hold handles in members and clear them with `= {}` in `EndPlay`, as `HelloRos` does. Otherwise they live until the actor is garbage-collected, and callbacks keep firing after `EndPlay`.
- `FString`'s `==` ignores case, and ROS names don't. Compare frame ids and topics with `Equals(Other, ESearchCase::CaseSensitive)`.
- Cyclone serializes a sample inside `dds_write`, so message fields can point at temporaries such as `TCHAR_TO_UTF8`.
- Every endpoint sends its ROS 2 type hash in USER_DATA as `typehash=RIHS01_…;`. `idl.py` takes the hash from the `.json` next to each IDL file, and `ROS_MESSAGE` exposes it. Without it, ROS nodes on Cyclone log `Failed to parse type hash` and `ros2 topic info -v` shows `INVALID`, though topics still match.
- On macOS, Unreal started as its own app (Dock, Epic launcher) needs Local Network access, or ROS never sees it, although `HelloRos` still hears itself. Started from a terminal, it uses the terminal's access and inherits the shell's `ROS_DOMAIN_ID`; from the Dock it runs on domain 0. To see what Cyclone does inside Unreal, point `CYCLONEDDS_URI` at a tracing config; `open --env` passes it to an app launch.
- The macOS firewall asks once for every new binary that receives network traffic, and each pixi environment has its own talker, listener and python. Until someone clicks Allow, that process hears nothing, which looks like a discovery bug.
- Messages come from Jazzy and are the same on the wire in Humble through Lyrical, except `sensor_msgs/Range`, which gained `variance` after Humble. Lyrical dropped `geometry_msgs/Pose2D`.
- When one shell's ROS tools see nothing while others do, suspect that shell, not Unreal. On 2026-09-26 a fresh shell fixed exactly that. A healthy participant answers a new participant's multicast hello within milliseconds. `tcpdump` shows the multicast on `en0` and the replies on `lo0`, and needs no sudo on Alvaro's Mac.
- UE 5.8's build accelerator (UBA) loses the object files when the plugin folder is a symlink, and the link fails with `no such file or directory`. The host project turns it off with `bAllowUBAExecutor` set to false in its own `Saved/UnrealBuildTool/BuildConfiguration.xml`. `AdditionalPluginDirectories` is no way around it, since it only finds plugins one folder down and this repo is the plugin folder.
- Building while any editor runs, even one still at the Project Browser, links a hot-reload binary such as `libUnrealEditor-RosBridge-0001.dylib` that the module list doesn't name, so the project opens with the old plugin. Pass `-NoHotReload`, as below. If the editor has the host project open, build the `/private/tmp` copy instead.
- Every game instance publishes its own `/clock`, so several PIE clients on one domain give ROS several clocks. Game time starts from zero on each Play and level load, which ROS nodes see as time jumping back.
- Unreal advances game time before any actor ticks, and physics moves bodies between `TG_PrePhysics` and `TG_PostPhysics`. A physics body read before physics is the previous frame's pose under this frame's stamp, so send it from `TG_PostPhysics` or later. `TfCheck` measures this as 3f.
- Game time in a headless run goes about six times faster than the wall clock. Frames there beat 2000 fps, and Unreal counts each as at least `MinUndilatedFrameTime`, 0.5 ms in the engine's `BaseGame.ini`. `Checks/run.sh` fixes the frame rate at 60 with an `-ini` override, which makes every step 1/60 s and holds game time to the wall clock. Don't read timing from a headless run without it.
- On macOS, don't read the GPU back through a texture. Metal's `RHIMapStagingSurface` and its texture lock both wait for the GPU to go idle, even when the copy is done, which stalls the render thread; its staging buffers only wait for a copy that isn't done. Vulkan waits for neither.
- Large messages need big socket receive buffers where they're received. Cyclone asks for 1 MiB unless `SocketReceiveBufferSize` says otherwise, and macOS caps a socket at about 7 MiB until `kern.ipc.maxsockbuf` goes up, which it accepts only to 16 MiB, a sixteenth of the kernel's 256 MiB network buffer pool on Alvaro's Mac. On 2026-09-27, at 1 MiB even 640×480 best-effort images got lost, and at 14 MiB reliable 4K arrived at 30 per second. The README has the table.

## Verifying changes

The host project on Alvaro's machine is `~/Documents/Unreal Projects/RosBridgeHost`, a blank C++ project with this repo symlinked as `Plugins/RosBridge`. If his editor has it open, copy the project and the plugin under `/private/tmp` and work there instead; `/tmp` is a symlink, which UBA can't handle. Build from the command line, run the automation tests, then run the hello world without a window:

```sh
UE="/Users/Shared/Epic Games/UE_5.8/Engine"
HOST="$HOME/Documents/Unreal Projects/RosBridgeHost/RosBridgeHost.uproject"
"$UE/Build/BatchFiles/Mac/Build.sh" RosBridgeHostEditor Mac Development -Project="$HOST" -NoHotReload
"$UE/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$HOST" -ExecCmds="Automation RunTests RosBridge" -testexit="Automation Test Queue Empty" -unattended -nullrhi -nosplash -stdout
"$UE/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$HOST" /Engine/Maps/Entry -game -nullrhi -unattended -stdout -ExecCmds="summon /Script/RosBridgeChecks.HelloRos"
```

Each automation test logs `Test Completed. Result={Success}` with its name.

While it runs, `pixi run listener` should print Unreal's messages, and messages from `pixi run talker` should show up in Unreal's log as `I heard`. Fast DDS is supported too, so repeat both against it:

```sh
pixi run env RMW_IMPLEMENTATION=rmw_fastrtps_cpp ros2 run demo_nodes_cpp listener
```

Every pixi environment sets `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` over the shell's value, hence `env` after activation. Unreal never shows in `ros2 node list`, since it isn't a node. Its endpoints appear in `pixi run ros2 topic info /chatter -v` under `_CREATED_BY_BARE_DDS_APP_`. The first `ros2 topic` call starts the ROS daemon and can answer before it has discovered Unreal, so run it twice or add `--no-daemon --spin-time 3`.

Checks with a Python side run through `Checks/run.sh`. It starts the checker, then the check actor in a headless Unreal on `ROS_DOMAIN_ID` 57, and exits with the checker's status. Every line should say PASS. The pixi environment and RMW go third and fourth, and `HOST` points it at another project:

```sh
sh Checks/run.sh ClockCheck clock.py
sh Checks/run.sh ClockCheck clock.py default rmw_fastrtps_cpp
sh Checks/run.sh TfCheck tf.py
sh Checks/run.sh TfCheck tf_static.py
IMAGE_SIZE=1920x1080 IMAGE_QOS=keep1 sh Checks/run.sh ImageCheck image.py
RHI=-RenderOffscreen sh Checks/run.sh CameraCheck camera.py
CAMERA_RAW=off RHI=-RenderOffscreen sh Checks/run.sh CameraCheck camera.py
```

`ImageCheck` and `image.py` both read `IMAGE_SIZE`, 640x480 by default, and `IMAGE_QOS`, one of `default`, `keep1` and `sensor`. Only 640×480 with the default QoS has to deliver every image; the rest report what arrived, and past 640×480 best effort needs the socket buffers the README describes.

`CameraCheck` needs pictures, so `RHI=-RenderOffscreen` has Unreal render with Metal and no window instead of `-nullrhi`. Its cubes show gray in the first few images, until their material's shaders have loaded, and the first image in color can show the green cube a frame behind, as Unreal rebuilds it; `camera.py` judges the images after that one.

Repeat against the other distros with `-e humble`, `-e kilted` and `-e lyrical` after `pixi run`. Lyrical's `demo_nodes_cpp` uses `example_interfaces/msg/String` instead of `std_msgs/msg/String`, so its nodes don't reach Unreal. Test Lyrical with `pixi run -e lyrical ros2 topic echo /chatter std_msgs/msg/String` and `ros2 topic pub` instead. On 2026-09-27 the hello world and every check passed on all four distros and both RMWs, except `ImageCheck`, which ran only on Jazzy with Cyclone.

## Style

- Unreal C++ conventions (tabs, Unreal containers and strings), without the `F`, `T` and `E` prefixes on our own types. `U` and `A` stay on UCLASSes only, because Unreal's header tool rejects a UCLASS without them. Wrap Cyclone and generated message headers in `THIRD_PARTY_INCLUDES_START`/`END`.
- Comments are one line and only for a non-obvious constraint. A `ponytail:` comment marks a deliberate shortcut and names its upgrade path.
- Conventional commits. Markdown is not hard-wrapped.
