#!/bin/sh
# Builds Cyclone DDS and the ROS 2 message types as static libraries under ThirdParty/.
set -eu
cd "$(dirname "$0")/.."

VERSION=0.10.5 # the Cyclone DDS that ROS 2 Jazzy ships
PACKAGES="builtin_interfaces std_msgs geometry_msgs nav_msgs sensor_msgs tf2_msgs rosgraph_msgs rcl_interfaces" # every message in these
DDS="$PWD/ThirdParty/cyclonedds"
MSGS="$PWD/ThirdParty/msgs"
export MACOSX_DEPLOYMENT_TARGET=13.0 # ignored off macOS

if [ ! -f "$DDS/lib/libddsc.a" ]; then
  rm -rf ThirdParty/cyclonedds-src ThirdParty/cyclonedds-build
  git clone --depth 1 --branch "$VERSION" https://github.com/eclipse-cyclonedds/cyclonedds.git ThirdParty/cyclonedds-src
  cmake -S ThirdParty/cyclonedds-src -B ThirdParty/cyclonedds-build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DDS" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DENABLE_SSL=OFF \
    -DENABLE_SECURITY=OFF \
    -DENABLE_SHM=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF \
    -DENABLE_TOPIC_DISCOVERY=OFF \
    -DENABLE_TYPE_DISCOVERY=OFF # match on type names like rmw_cyclonedds; 0.10.5's XTypes matching fails with Kilted's Fast DDS
  cmake --build ThirdParty/cyclonedds-build --target install --parallel
fi

rm -rf "$MSGS" && mkdir -p "$MSGS"
python3 Scripts/idl.py "$CONDA_PREFIX/share" "$MSGS" $PACKAGES
for idl in "$MSGS"/*.idl; do
  "$DDS/bin/idlc" -x final -f case-sensitive -o "$MSGS" "$idl" # ROS names like INT8 differ from IDL keywords only in case
done
(cd "$MSGS" && cc -O2 -fPIC -I "$DDS/include" -c ./*.c && ar rcs libmsgs.a ./*.o)
