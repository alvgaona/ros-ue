# Checks /clock against a headless Unreal running ClockCheck, printing one PASS or FAIL line per behavior.
# Run it through Checks/run.sh, which starts both on a private ROS domain.
import statistics
import sys
import time
from itertools import pairwise

import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.qos import QoSProfile, ReliabilityPolicy
from rosgraph_msgs.msg import Clock
from std_msgs.msg import String

rclpy.init()
node = Node("clock_check", parameter_overrides=[Parameter("use_sim_time", value=True)])
samples = []  # (wall seconds, sim nanoseconds), one per Unreal frame
readings = []  # sim nanoseconds the node's own use_sim_time clock reported
phases = {}  # phase name -> wall seconds it was announced


def on_clock(msg):
    samples.append((time.monotonic(), msg.clock.sec * 10**9 + msg.clock.nanosec))
    readings.append(node.get_clock().now().nanoseconds)


node.create_subscription(Clock, "/clock", on_clock, QoSProfile(depth=1000, reliability=ReliabilityPolicy.RELIABLE))
node.create_subscription(String, "/clock_check/phase", lambda msg: phases.setdefault(msg.data, time.monotonic()), 10)

deadline = time.monotonic() + 300  # Unreal takes a minute or two to start
while "done" not in phases and time.monotonic() < deadline:
    rclpy.spin_once(node, timeout_sec=0.1)


def verdict(name, ok, detail):
    print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}", flush=True)
    return ok


def steps(start, stop):
    # Sim time between consecutive frames, skipping half a second after each phase change
    window = [sim for wall, sim in samples if start + 0.5 < wall < stop]
    return [(b - a) / 1e9 for a, b in pairwise(window)]


sims = [sim for _, sim in samples]
ok = [
    verdict(
        "2a /clock arrives and never goes backwards",
        len(sims) > 20 and all(b >= a for a, b in pairwise(sims)),
        f"{len(sims)} samples",
    )
]
known = set(sims)
# The node's own /clock subscription runs ahead of ours, so readings past our newest sample can't be compared yet
reads = [r for r in readings if 0 < r <= max(sims, default=0)]
ok.append(
    verdict(
        "2b a use_sim_time node takes its time from /clock",
        bool(reads) and all(r in known for r in reads),
        f"{len(reads)} readings",
    )
)

if all(p in phases for p in ("run", "pause", "resume", "slomo", "done")):
    held = max(sim for wall, sim in samples if wall < phases["pause"] + 0.5)
    paused = [sim for wall, sim in samples if phases["pause"] + 0.5 < wall < phases["resume"] - 0.5]
    advanced = [sim for sim in sims if sim > held]
    jump = (advanced[0] - held) / 1e9 if advanced else float("inf")
    ok.append(
        verdict(
            "2c /clock holds while paused and resumes without a jump",
            all(s == held for s in paused) and jump < 0.2,
            f"{len(paused)} samples while paused, first step after {jump:.3f} s",
        )
    )
    run, slomo = (
        statistics.median(steps(phases["run"], phases["pause"])),
        statistics.median(steps(phases["slomo"], phases["done"])),
    )
    ok.append(
        verdict(
            "2c slomo 0.5 halves how far sim time moves per frame",
            0.4 < slomo / run < 0.6,
            f"{slomo / run:.2f} of the normal step",
        )
    )
    running = [(wall, sim) for wall, sim in samples if phases["run"] + 0.5 < wall < phases["pause"]]
    factor = (running[-1][1] - running[0][1]) / 1e9 / (running[-1][0] - running[0][0])
    print(f"INFO 2d real-time factor: {factor:.2f} sim seconds per wall second", flush=True)
else:
    ok.append(verdict("2c phases announced", False, f"saw {sorted(phases)}"))

sys.exit(0 if all(ok) else 1)
