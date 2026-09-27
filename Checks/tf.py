# Checks /tf against a headless Unreal running TfCheck, printing one PASS or FAIL line per behavior.
# Run it through Checks/run.sh, which starts both on a private ROS domain.
import bisect
import math
import statistics
import sys
import time

import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.qos import QoSProfile, ReliabilityPolicy
from rclpy.time import Time
from rosgraph_msgs.msg import Clock
from tf2_msgs.msg import TFMessage
from tf2_ros import Buffer, TransformListener

rclpy.init()
node = Node("tf_check", parameter_overrides=[Parameter("use_sim_time", value=True)])
buffer = Buffer()
listener = TransformListener(buffer, node)
reliable = QoSProfile(depth=1000, reliability=ReliabilityPolicy.RELIABLE)
clock = []  # every /clock value, in nanoseconds
messages = []  # every raw /tf message


def ns(stamp):
    return stamp.sec * 10**9 + stamp.nanosec


node.create_subscription(Clock, "/clock", lambda msg: clock.append(ns(msg.clock)), reliable)
node.create_subscription(TFMessage, "/tf", messages.append, reliable)

deadline = time.monotonic() + 300  # Unreal takes a minute or two to start
while time.monotonic() < deadline and not (messages and ns(messages[-1].transforms[0].header.stamp) - ns(messages[0].transforms[0].header.stamp) > 5e9):
    rclpy.spin_once(node, timeout_sec=0.1)


def verdict(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}", flush=True)
    return ok


def yaw_error(q, yaw):
    # Angle in radians between rotation q and a pure yaw
    return 2 * math.acos(min(1.0, abs(q.z * math.sin(yaw / 2) + q.w * math.cos(yaw / 2))))


frames = {}  # child frame -> {stamp: transform}
for m in messages:
    for t in m.transforms:
        frames.setdefault(t.child_frame_id, {})[ns(t.header.stamp)] = t.transform
slide, turn, body, pre = (frames.get(child, {}) for child in ("slide", "turn", "body", "body_pre"))
ok = []

name = "3a tf2 reads the fixed frame at the step 1 values"
try:
    fixed = buffer.lookup_transform("world", "fixed", Time()).transform
    v = fixed.translation
    error = max(abs(v.x - 1), abs(v.y + 2), abs(v.z - 0.5), yaw_error(fixed.rotation, -math.pi / 2))
    ok.append(verdict(name, error < 1e-6, f"off by {error:.1e}"))
except Exception as e:
    ok.append(verdict(name, False, e))

# /tf stamps past our newest /clock sample can't be compared yet
known = set(clock)
stamps = [s for poses in frames.values() for s in poses if s <= max(clock, default=0)]
ok.append(verdict("3b every /tf stamp is a /clock value", bool(stamps) and all(s in known for s in stamps), f"{len(stamps)} stamps"))

errors = [max(abs(v.translation.x), abs(v.translation.y + s / 1e9), abs(v.translation.z), yaw_error(v.rotation, 0)) for s, v in slide.items()]
errors += [max(abs(v.translation.x), abs(v.translation.y), abs(v.translation.z), yaw_error(v.rotation, -math.pi / 2 * s / 1e9)) for s, v in turn.items()]
error = max(errors, default=math.inf)
ok.append(verdict("3c every pose matches its own stamp", error < 1e-6, f"{len(slide)} slide and {len(turn)} turn poses, off by {error:.1e}"))

name = "3d tf2 interpolates between frames"
try:
    ends = sorted(slide)[10:-10]  # away from the edges of what the buffer holds
    errors = []
    for middle in [(a + b) // 2 for a, b in zip(ends, ends[1:])][::10]:
        t = middle / 1e9
        at = Time(nanoseconds=middle)
        s = buffer.lookup_transform("world", "slide", at).transform
        r = buffer.lookup_transform("world", "turn", at).transform
        errors.append(max(abs(s.translation.y + t), yaw_error(r.rotation, -math.pi / 2 * t)))
    error = max(errors, default=math.inf)
    ok.append(verdict(name, error < 1e-6, f"{len(errors)} lookups halfway between frames, off by {error:.1e}"))
except Exception as e:
    ok.append(verdict(name, False, e))

batches = [m for m in messages if any(t.child_frame_id == "slide" for t in m.transforms)]
together = all(sorted(t.child_frame_id for t in m.transforms) == ["body", "fixed", "slide", "turn"] and len({ns(t.header.stamp) for t in m.transforms}) == 1 for m in batches)
slashed = [i for m in messages for t in m.transforms for i in (t.header.frame_id, t.child_frame_id) if i.startswith("/")]
ok.append(verdict("3e frames sent together arrive as one message, without slashes", bool(batches) and together and not slashed, f"{len(batches)} messages, {len(slashed)} slashed ids"))

# The box moves toward ROS -Y at 1 m/s, so physics should move it by the step since the previous frame
ticks = sorted(clock)
moves, errors = [], []
for s in sorted(set(body) & set(pre)):
    i = bisect.bisect_left(ticks, s)
    if i == 0 or ticks[i - 1] not in body:
        continue
    previous = ticks[i - 1]
    moves.append(pre[s].translation.y - body[s].translation.y)
    errors.append(abs(moves[-1] - (s - previous) / 1e9))
    errors += [abs(getattr(pre[s].translation, axis) - getattr(body[previous].translation, axis)) for axis in "xyz"]
error = max(errors, default=math.inf)
detail = f"{len(moves)} frames, {statistics.mean(moves) * 100:.4f} cm per frame, off by {error:.1e}" if moves else "no frames"
ok.append(verdict("3f a pose read before physics is one frame older than its stamp", error < 1e-6, detail))

sys.exit(0 if all(ok) else 1)
