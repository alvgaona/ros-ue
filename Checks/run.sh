#!/bin/sh
# Runs a check actor in a headless Unreal against its Python checker, on a private ROS domain.
# Usage: Checks/run.sh <check actor> <checker> [pixi environment] [rmw]
# Set HOST to another .uproject when the editor has the default one open. Unreal's output goes to $LOG.
set -u
UE="${UE:-/Users/Shared/Epic Games/UE_5.8/Engine}"
HOST="${HOST:-$HOME/Documents/Unreal Projects/RosBridgeHost/RosBridgeHost.uproject}"
LOG="${LOG:-/tmp/rosbridge-check.log}"
export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-57}" # away from anything on the default domain
cd "$(dirname "$0")/.."

# The checker starts first, so it hears everything the actor announces
pixi run -e "${3:-default}" env RMW_IMPLEMENTATION="${4:-rmw_cyclonedds_cpp}" python "Checks/$2" &
CHECKER=$!
"$UE/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$HOST" /Engine/Maps/Entry -game -nullrhi -unattended -nosplash -nosound -stdout -FullStdOutLogOutput \
  -ExecCmds="summon /Script/RosBridge.$1" > "$LOG" 2>&1 &
UNREAL=$!
wait $CHECKER
STATUS=$?
kill $UNREAL
wait $UNREAL 2>/dev/null
exit $STATUS
