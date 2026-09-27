# Third-party licenses

A release zip carries Cyclone DDS and the ROS 2 message types compiled, so it carries their licenses too, in `ThirdParty/licenses/`.

| Part | License | File |
|---|---|---|
| Cyclone DDS 0.10.5 | EPL-2.0 or EDL-1.0 (BSD-3-Clause) | `cyclonedds-LICENSE` and `cyclonedds-NOTICE.md`, copied from its source by `package.sh` |
| `builtin_interfaces`, `std_msgs`, `geometry_msgs`, `nav_msgs`, `sensor_msgs`, `rosgraph_msgs`, `rcl_interfaces` | Apache 2.0 | The plugin's own `LICENSE` has the text; none of them has a NOTICE file |
| `tf2_msgs` | BSD-3-Clause | `tf2_msgs-LICENSE`, from ros2/geometry2 on the jazzy branch |
