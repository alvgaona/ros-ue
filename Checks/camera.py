# Checks /camera/image_raw and its /compressed twin against a headless Unreal running CameraCheck, printing one PASS or FAIL line per behavior.
# The camera needs a run that renders, so run it through Checks/run.sh with RHI=-RenderOffscreen. CAMERA_RAW=off, as for
# CameraCheck, checks that only JPEGs come.
import io
import os
import sys
import time

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from PIL import Image as Pil
from rosgraph_msgs.msg import Clock
from sensor_msgs.msg import CompressedImage, Image

width, height = 640, 480
f = width / 2  # focal length in pixels, for Unreal's default horizontal field of view of 90 degrees


def project(y, z):
    # Where a point on the cubes' front faces, 9.75 m ahead, y right of and z above the camera's axis, lands in the image
    return np.array([width / 2 + f * y / 975, height / 2 - f * z / 975])


def swinging(t):
    return project(600 * np.sin(np.pi * t), -200)  # CameraCheck's green cube


rclpy.init()
node = Node("camera_check")
reliable = QoSProfile(depth=1000, reliability=ReliabilityPolicy.RELIABLE)
clock = []  # every /clock value, in nanoseconds
images = []  # (stamp in nanoseconds, right header and layout, red cube's center, green cube's center) per image
raw = {}  # the pixels of the latest raw images, by stamp
jpegs = []  # (stamp, right format and frame, mean difference from the raw image with its stamp or None without one) per JPEG


def ns(stamp):
    return stamp.sec * 10**9 + stamp.nanosec


def center(mask):
    rows, cols = np.nonzero(mask)
    return np.array([cols.mean(), rows.mean()]) + 0.5 if len(cols) else None


def on_image(msg):
    layout = (msg.header.frame_id, msg.width, msg.height, msg.encoding, msg.step, msg.is_bigendian, len(msg.data)) == \
        ("camera_optical_frame", width, height, "bgr8", width * 3, 0, width * height * 3)
    red = green = None
    if layout:
        b, g, r = np.frombuffer(msg.data, np.uint8).reshape(height, width, 3).astype(int).transpose(2, 0, 1)
        red = center((r > 50) & (r > 2 * g) & (r > 2 * b))
        green = center((g > 50) & (g > 2 * r) & (g > 2 * b))
        raw[ns(msg.header.stamp)] = np.frombuffer(msg.data, np.uint8).reshape(height, width, 3)
        for old in sorted(raw)[:-30]:
            del raw[old]
    images.append((ns(msg.header.stamp), layout, red, green))


def on_jpeg(msg):
    labeled = (msg.format, msg.header.frame_id) == ("bgr8; jpeg compressed bgr8", "camera_optical_frame")
    decoded = np.asarray(Pil.open(io.BytesIO(bytes(msg.data))).convert("RGB"))[..., ::-1]  # to bgr, as the raw image
    same = raw.get(ns(msg.header.stamp))
    off = np.abs(decoded.astype(int) - same).mean() if same is not None and decoded.shape == same.shape else None
    jpegs.append((ns(msg.header.stamp), labeled, off))


node.create_subscription(Clock, "/clock", lambda msg: clock.append(ns(msg.clock)), reliable)
node.create_subscription(Image, "/camera/image_raw", on_image, 10)
node.create_subscription(CompressedImage, "/camera/image_raw/compressed", on_jpeg, 10)

deadline = time.monotonic() + 300  # Unreal takes a while to start
while time.monotonic() < deadline and not images and not jpegs:
    rclpy.spin_once(node, timeout_sec=0.1)
end = time.monotonic() + 10
while time.monotonic() < end:
    rclpy.spin_once(node, timeout_sec=0.1)


def verdict(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}", flush=True)
    return ok


def delivered(stamps):
    # The camera takes one every 1/30 s of sim time, so the first and last stamps say how many it sent meanwhile
    stamps = sorted(set(stamps))
    sent = round((stamps[-1] - stamps[0]) * 30 / 1e9) + 1 if stamps else 0
    return sent > 0 and len(stamps) == sent, f"{len(stamps)} of {sent}"


if os.environ.get("CAMERA_RAW") == "off":
    ok, detail = delivered(s for s, *_ in jpegs)
    publishers = node.count_publishers("/camera/image_raw")
    sys.exit(0 if verdict("2g with Raw off, no raw topic, and every JPEG still comes", ok and not images and publishers == 0,
                          f"{detail} JPEGs, {len(images)} raw images, {publishers} raw publishers") else 1)

laid_out = [image for image in images if image[1]]
ok = [verdict("2a every image is 640x480 bgr8 in camera_optical_frame", bool(images) and len(laid_out) == len(images),
              f"{len(images)} images, {len(images) - len(laid_out)} wrong")]

# Stamps outside the /clock samples we heard can't be compared
known = set(clock)
checked = [s for s, *_ in images if min(clock, default=0) <= s <= max(clock, default=0)]
ok.append(verdict("2b every stamp is a /clock value", bool(checked) and all(s in known for s in checked), f"{len(checked)} stamps"))

ok.append(verdict("2c every image is delivered, 30 per second of sim time", *delivered(s for s, *_ in images)))

# The cubes are gray until their material's shaders have loaded, and the first image in color can show the green cube a
# frame behind as Unreal rebuilds it, so judge the images after the first that shows both
first = next((i for i, (_, _, red, green) in enumerate(laid_out) if red is not None and green is not None), len(laid_out)) + 1
shown = laid_out[first:]
warmup = f", after {first} while shaders loaded"

# Flipped, mirrored or with red and blue swapped, the red cube isn't where it should be
still = project(-300, 200)
off = [np.linalg.norm(red - still) if red is not None else np.inf for _, _, red, _ in shown]
ok.append(verdict("2d the image is upright and red is red", bool(off) and max(off) < 2,
                  f"red cube at most {max(off, default=np.inf):.1f} px from ({still[0]:.1f}, {still[1]:.1f}) in {len(off)} images{warmup}"))

# The green cube moves up to 10 px a frame, so an image of an earlier frame would put it well off
off = [np.linalg.norm(green - swinging(s / 1e9)) if green is not None else np.inf for s, _, _, green in shown]
ok.append(verdict("2e each image shows the scene at its stamp", bool(off) and max(off) < 2,
                  f"green cube at most {max(off, default=np.inf):.1f} px from where its stamp puts it in {len(off)} images"))

# JPEG's error on flat cubes against black stays well under a level in 255
matched = [off for _, _, off in jpegs if off is not None]
ok.append(verdict("2f every JPEG on /compressed is labeled for image_transport and decodes to the raw image with its stamp",
                  bool(matched) and all(labeled for _, labeled, _ in jpegs) and max(matched) < 1,
                  f"{len(jpegs)} JPEGs, {len(matched)} matched to a raw image, off by {max(matched, default=np.inf):.2f} on average at most"))

sys.exit(0 if all(ok) else 1)
