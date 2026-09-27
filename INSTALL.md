# Installing

The plugin installs into one Unreal project, in its `Plugins/` folder. Unreal compiles it the first time the project opens.

## What you need

- Unreal Engine 5.8.
- A C++ project. Unreal only compiles plugins in projects that have their own code. A Blueprint-only project becomes one when you add any C++ class (Tools → New C++ Class).
- The compiler Unreal uses: Xcode on macOS, clang on Linux.

You don't need ROS on the machine that runs Unreal. The plugin talks to ROS 2 over DDS on its own.

## From a release (macOS)

1. Download `RosBridge-<version>-Mac.zip` from the [releases](https://github.com/alvgaona/ros-ue/releases). It contains the plugin's source with Cyclone DDS and the ROS message types already built.
2. Unzip it into your project, so you have `<YourProject>/Plugins/RosBridge/RosBridge.uplugin`.
3. Open the project and let Unreal build the plugin when it asks.

To update, replace the folder with the next release.

Releases for Linux will follow. Until then, build from source there.

## From source

You need [pixi](https://pixi.sh) too. It builds Cyclone DDS and the message types, and brings the ROS used for testing.

```sh
git clone https://github.com/alvgaona/ros-ue.git "<YourProject>/Plugins/RosBridge"
cd "<YourProject>/Plugins/RosBridge"
pixi run setup   # builds Cyclone DDS and the ROS message types into ThirdParty/
```

Then open the project, which builds the plugin. Building from source is also how to get messages from packages a release doesn't include; [RosBridge's README](Source/RosBridge/README.md) explains how.

## Using it from C++

Add the modules you use to your module's `.Build.cs`: `RosBridge` for topics, the clock and TF, and `RosSim` for the camera.

```csharp
PublicDependencyModuleNames.AddRange(new[] { "RosBridge", "RosSim" });
```

Without code, you can still add a Ros Camera to any actor in the editor (Add → Ros Camera).

## Check that ROS sees Unreal

Drop a `HelloRos` actor in a level and press Play. It publishes `Hello World: N` on `/chatter` every second. On any machine with ROS 2 on the same network and `ROS_DOMAIN_ID`:

```sh
ros2 topic echo /chatter
```

On macOS, ROS only sees the editor if it has Local Network access (System Settings → Privacy & Security → Local Network). An editor started from a terminal uses the terminal's access instead.
