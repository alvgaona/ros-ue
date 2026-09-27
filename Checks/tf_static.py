# Checks /tf_static against a headless Unreal running TfCheck, and a tree Unreal shares with ROS nodes, printing one
# PASS or FAIL line per behavior. Run it through Checks/run.sh, which starts both on a private ROS domain.
import math
import sys
import time

import rclpy
from geometry_msgs.msg import TransformStamped
from rclpy.executors import SingleThreadedExecutor
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from rclpy.time import Time
from rosgraph_msgs.msg import Clock
from tf2_msgs.msg import TFMessage
from tf2_ros import Buffer, StaticTransformBroadcaster, TransformBroadcaster, TransformException, TransformListener

try:
    from rclpy.event_handler import SubscriptionEventCallbacks
except ImportError:  # Humble
    from rclpy.qos_event import SubscriptionEventCallbacks

rclpy.init()
sim = Node("tf_static_check", parameter_overrides=[Parameter("use_sim_time", value=True)])
wall = Node("tf_static_check_wall")
executor = SingleThreadedExecutor()
executor.add_node(sim)
executor.add_node(wall)
reliable = QoSProfile(depth=1000, reliability=ReliabilityPolicy.RELIABLE)
clock = []  # every /clock value, in nanoseconds
first = []  # the stamp of Unreal's first /tf message


def ns(stamp):
    return stamp.sec * 10**9 + stamp.nanosec


def offset(parent, child, x, y, node):
    t = TransformStamped()
    t.header.stamp = node.get_clock().now().to_msg()
    t.header.frame_id, t.child_frame_id = parent, child
    t.transform.translation.x, t.transform.translation.y = x, y
    t.transform.rotation.w = 1.0
    return t


# 4e: ROS nodes hang frames off Unreal's slide, one on sim time, one on wall time and one static on wall time
sim_tf, wall_tf, static_tf = TransformBroadcaster(sim), TransformBroadcaster(wall), StaticTransformBroadcaster(wall)
static_tf.sendTransform(offset("slide", "ros_static", 0.0, 1.0, wall))
wall.create_timer(0.05, lambda: wall_tf.sendTransform(offset("slide", "ros_wall", 1.0, 0.0, wall)))


def on_clock(msg):
    clock.append(ns(msg.clock))
    if sim.get_clock().now().nanoseconds:  # zero until the node's own clock has heard /clock
        sim_tf.sendTransform(offset("slide", "ros_sim", 1.0, 0.0, sim))


def on_tf(msg):
    if not first and msg.transforms[0].header.frame_id == "world":  # Unreal's, not this checker's own frames on slide
        first.append(ns(msg.transforms[0].header.stamp))


sim.create_subscription(Clock, "/clock", on_clock, reliable)
sim.create_subscription(TFMessage, "/tf", on_tf, reliable)

# Unreal sends its second static frame a second after its first /tf, so join well after that, as a late subscriber
deadline = time.monotonic() + 300  # Unreal takes a minute or two to start
while time.monotonic() < deadline and not (first and clock and clock[-1] - first[0] > 3e9):
    executor.spin_once(timeout_sec=0.1)
late = []  # /tf_static messages the late subscription got
incompatible = []
static_qos = QoSProfile(depth=100, reliability=ReliabilityPolicy.RELIABLE, durability=DurabilityPolicy.TRANSIENT_LOCAL)
sim.create_subscription(
    TFMessage,
    "/tf_static",
    late.append,
    static_qos,
    event_callbacks=SubscriptionEventCallbacks(incompatible_qos=incompatible.append),
)
buffer = Buffer()
listener = TransformListener(buffer, sim)
end = time.monotonic() + 3
while time.monotonic() < end:
    executor.spin_once(timeout_sec=0.1)


def verdict(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}", flush=True)
    return ok


def yaw_error(q, yaw):
    # Angle in radians between rotation q and a pure yaw
    return 2 * math.acos(min(1.0, abs(q.z * math.sin(yaw / 2) + q.w * math.cos(yaw / 2))))


unreal = [m for m in late if any(t.child_frame_id in ("mount", "arm") for t in m.transforms)]
children = [sorted(t.child_frame_id for t in m.transforms) for m in unreal]
ok = [
    verdict(
        "4a a subscriber that joins late still gets Unreal's static frames", bool(unreal), f"{len(unreal)} messages"
    )
]
ok.append(
    verdict("4b what it gets holds both frames, sent a second apart", ["arm", "mount"] in children, f"got {children}")
)
ok.append(verdict("4c /tf_static has no incompatible QoS", not incompatible, f"{len(incompatible)} events"))

name = "4d world, a moving frame and its static child resolve as one chain"
try:
    mount = buffer.lookup_transform("world", "mount", Time())
    arm = buffer.lookup_transform("world", "arm", Time())
    tm, ta = ns(mount.header.stamp) / 1e9, ns(arm.header.stamp) / 1e9
    m, a = mount.transform.translation, arm.transform.translation
    error = max(
        abs(m.x),
        abs(m.y + tm),
        abs(m.z - 0.5),
        abs(a.x - math.cos(math.pi / 2 * ta)),
        abs(a.y + math.sin(math.pi / 2 * ta)),
        abs(a.z),
        yaw_error(arm.transform.rotation, -math.pi / 2 * ta),
    )
    ok.append(verdict(name, error < 1e-6, f"mount and arm off by {error:.1e}"))
except TransformException as e:
    ok.append(verdict(name, False, e))

name = "4e ROS frames on Unreal's resolve from sim time or /tf_static, not wall time"
try:
    on_sim = buffer.lookup_transform("world", "ros_sim", Time())
    static = buffer.lookup_transform("world", "ros_static", Time())
    s, st = on_sim.transform.translation, static.transform.translation
    error = max(
        abs(s.x - 1), abs(s.y + ns(on_sim.header.stamp) / 1e9), abs(st.x), abs(st.y - 1 + ns(static.header.stamp) / 1e9)
    )
    try:
        buffer.lookup_transform("world", "ros_wall", Time())
        wall_result = "resolves"
    except TransformException as e:
        wall_result = type(e).__name__
    ok.append(
        verdict(
            name,
            error < 1e-6 and wall_result == "ExtrapolationException",
            f"sim and static off by {error:.1e}, wall time gives {wall_result}",
        )
    )
except TransformException as e:
    ok.append(verdict(name, False, e))

sys.exit(0 if all(ok) else 1)
