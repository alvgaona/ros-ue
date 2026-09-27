# Checks /image_raw against a headless Unreal running ImageCheck, printing one PASS, FAIL or INFO line per behavior.
# IMAGE_SIZE and IMAGE_QOS pick the run, as for ImageCheck. Run it through Checks/run.sh, which starts both on a private ROS domain.
import os
import sys
import time

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from rosgraph_msgs.msg import Clock
from sensor_msgs.msg import Image
from std_msgs.msg import Float64

width, height = (int(n) for n in os.environ.get("IMAGE_SIZE", "640x480").split("x"))
qos_name = os.environ.get("IMAGE_QOS", "default")
qos = {
    "default": QoSProfile(depth=10),
    "keep1": QoSProfile(depth=1),
    "sensor": QoSProfile(depth=5, reliability=ReliabilityPolicy.BEST_EFFORT),
}[qos_name]

# The pattern ImageCheck sends: blue and green carry x and y, red their high bits
x, y = np.arange(width)[None, :], np.arange(height)[:, None]
pattern = np.zeros((height, width, 4), np.uint8)
pattern[..., 0] = x & 0xFF
pattern[..., 1] = y & 0xFF
pattern[..., 2] = (x >> 8) | ((y >> 8) << 4)
pattern[..., 3] = 0xFF
pattern = pattern.reshape(-1)

rclpy.init()
node = Node("image_check")
reliable = QoSProfile(depth=1000, reliability=ReliabilityPolicy.RELIABLE)
clock = []  # every /clock value, in nanoseconds
images = []  # (stamp in nanoseconds, arrival in wall seconds, exact) per image
writes = []  # seconds each write held Unreal's game thread


def ns(stamp):
    return stamp.sec * 10**9 + stamp.nanosec


def on_image(msg):
    exact = (msg.width, msg.height, msg.encoding, msg.step, msg.is_bigendian) == (
        width,
        height,
        "bgra8",
        width * 4,
        0,
    ) and np.array_equal(np.frombuffer(msg.data, np.uint8), pattern)
    images.append((ns(msg.header.stamp), time.monotonic(), exact))


node.create_subscription(Clock, "/clock", lambda msg: clock.append(ns(msg.clock)), reliable)
node.create_subscription(Image, "/image_raw", on_image, qos)
node.create_subscription(Float64, "/image_check/write_seconds", lambda msg: writes.append(msg.data), reliable)

deadline = time.monotonic() + 300  # Unreal takes a while to start
while time.monotonic() < deadline and not images:
    rclpy.spin_once(node, timeout_sec=0.1)
end = time.monotonic() + 10
while time.monotonic() < end:
    rclpy.spin_once(node, timeout_sec=0.1)


def verdict(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}", flush=True)
    return ok


run = f"{width}x{height}, {qos_name} QoS"
wrong = [s for s, _, exact in images if not exact]
ok = [
    verdict(
        f"1a every image arrives exact ({run})", bool(images) and not wrong, f"{len(images)} images, {len(wrong)} wrong"
    )
]

# Stamps outside the /clock samples we heard can't be compared
known = set(clock)
checked = [s for s, _, _ in images if min(clock, default=0) <= s <= max(clock, default=0)]
ok.append(
    verdict(
        "1b every stamp is a /clock value", bool(checked) and all(s in known for s in checked), f"{len(checked)} stamps"
    )
)

# ImageCheck sends one every 1/30 s of sim time, so the first and last stamps say how many went out meanwhile
stamps = sorted({s for s, _, _ in images})
walls = [w for _, w, _ in images]
sent = round((stamps[-1] - stamps[0]) * 30 / 1e9) + 1 if stamps else 0
detail = f"{len(stamps)} of {sent} sent"
if len(walls) > 1:
    wall = walls[-1] - walls[0]
    detail += f", {(len(walls) - 1) / wall:.1f} per wall second, {(stamps[-1] - stamps[0]) / 1e9 / wall:.2f} sim seconds per wall second"
# Only reliable keep last 10 promises every image; keep last 1 and best effort drop some by design
if (width, height, qos_name) == (640, 480, "default"):
    ok.append(verdict("1c every image is delivered", sent > 0 and len(stamps) == sent, detail))
else:
    print(f"INFO 1c delivery: {detail}", flush=True)

if writes:
    ms = np.array(writes) * 1000
    print(
        f"INFO 1d each write held the game thread: median {np.median(ms):.2f} ms, 95th percentile {np.percentile(ms, 95):.2f} ms, "
        f"max {ms.max():.2f} ms over {len(ms)} writes",
        flush=True,
    )

sys.exit(0 if all(ok) else 1)
