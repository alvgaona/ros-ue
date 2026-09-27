# RosSim

ROS output for things Unreal already has, such as its cameras, built on [RosBridge](../RosBridge/README.md). Sensors Unreal doesn't have, such as lidars and IMUs, and robots or vehicles, you model yourself and publish through RosBridge. Add `RosSim` next to `RosBridge` in your module's `.Build.cs`.

## Camera

A Ros Camera component publishes what it sees as `sensor_msgs/Image` in `bgr8`: by default 640×480 on `/camera/image_raw`, 30 images per second of sim time, in the frame `camera_optical_frame`. Add it to an actor in the editor (Add → Ros Camera) or in C++, and change those in its Ros properties. It is a scene capture underneath, so field of view and post-processing work as on any scene capture.

Each image carries the sim time of the frame it was captured in, taken once everything in that frame has moved, physics included. Rendering the camera's view costs what any second view of the scene costs, but nothing waits for the image: the GPU copies it back while the game carries on, and a worker thread publishes it. If the worker can't keep up, as with large images on a slow network, the camera drops older images so the newest goes out next.

It also publishes every image as JPEG on `<Topic>/compressed`, as ROS cameras do through `image_transport`, so a node can subscribe to either. RViz's Image display and `rqt_image_view` show it with their transport set to compressed, which needs the `compressed_image_transport` plugin on the ROS side. Like `image_transport`, the camera sends each only while something subscribes to it, so a JPEG is encoded only for a subscriber. Turn off Raw or Compressed to leave that topic out altogether; JpegQuality is 95 by default, as in `image_transport`.

Raw images past 640×480 need bigger socket buffers on the subscriber, as [Large messages](../RosBridge/README.md#large-messages) describes. JPEG is about twenty times smaller and needs none of that. In Unreal's Open World template, a 1080p image was 0.34 MB instead of 6.2 MB, and a 4K one 1.1 MB instead of 25 MB. With Cyclone's default buffer, 4K JPEG arrived at the camera's full 30 images a second, where raw 4K managed 13.

It publishes with the default QoS. For another, set `Qos` before BeginPlay:

```cpp
Camera = CreateDefaultSubobject<URosCameraComponent>(TEXT("Camera"));
Camera->Qos = ros::SensorDataQos();
```
