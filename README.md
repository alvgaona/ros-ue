# ros-ue

[![CI](https://github.com/alvgaona/ros-ue/actions/workflows/ci.yaml/badge.svg)](https://github.com/alvgaona/ros-ue/actions/workflows/ci.yaml)
![Version](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fraw.githubusercontent.com%2Falvgaona%2Fros-ue%2Fmain%2FRosBridge.uplugin&query=%24.VersionName&label=version&prefix=v&color=blue)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue)](LICENSE)
![Unreal Engine 5](https://img.shields.io/badge/Unreal_Engine-5-313131?logo=unrealengine&logoColor=ffffff)
![ROS 2](https://img.shields.io/badge/ROS_2-Humble_%7C_Jazzy_%7C_Kilted_%7C_Lyrical-22314e?logo=ros&logoColor=ffffff)

Unreal Engine 5 plugin that talks to ROS 2 over DDS without being a ROS 2 node. Publishers and subscriptions work like rclcpp, and callbacks run on the game thread. It works with ROS 2 Humble, Jazzy, Kilted and Lyrical on Cyclone DDS or Fast DDS.

| Module | What it has |
|---|---|
| [RosBridge](Source/RosBridge/README.md) | The ROS 2 client: publishers, subscriptions, QoS, the clock and TF. |
| [RosSim](Source/RosSim/README.md) | ROS output for things Unreal already has, such as its cameras. |

## Supported operating systems

- macOS on Apple silicon. ROS only sees the editor if it has Local Network access (System Settings → Privacy & Security → Local Network).
- Linux on x86_64.
- Windows is coming soon.

## Install

[INSTALL.md](INSTALL.md) covers installing from a release or from source, and using the plugin from C++.

## Hello world

Drop a `HelloRos` actor in a level and press Play. It publishes `Hello World: N` on `/chatter` every second and logs what it hears there.

```sh
pixi run listener   # prints what Unreal publishes
pixi run talker     # Unreal logs "I heard: [...]"
```

## Contributing

Commits and PR titles follow [Conventional Commits](https://www.conventionalcommits.org/). [`AGENTS.md`](AGENTS.md) has the constraints, the layout and how to verify a change.

### Development

```sh
bun install                  # installs the commit-msg hook
pixi run -e lint format      # formats C++ and Python
pixi run -e lint lint        # checks formatting, as CI does
```

[`AGENTS.md`](AGENTS.md) covers testing a change in Unreal.

### AI

AI tools are welcome, and [`AGENTS.md`](AGENTS.md) is written for them. Outside contributions that use them follow these rules:

- Say which tool you used and how much of the work it did.
- Understand every line you submit. If you can't explain how your change works with the rest of the plugin without an AI's help, don't send it.
- Review and edit AI-written issues and PR descriptions before posting. Cut the noise.
- No AI-generated images, video or audio.

## License

[Apache 2.0](LICENSE).
