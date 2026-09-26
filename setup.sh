#!/bin/sh
# Builds Cyclone DDS and the message types (msg/*.idl) as static libraries under ThirdParty/.
set -eu
cd "$(dirname "$0")"

VERSION=0.10.5 # the Cyclone DDS that ROS 2 Jazzy ships
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
    -DBUILD_TESTING=OFF
  cmake --build ThirdParty/cyclonedds-build --target install --parallel
fi

rm -rf "$MSGS" && mkdir -p "$MSGS"
for idl in msg/*.idl; do
  "$DDS/bin/idlc" -o "$MSGS" "$idl"
done
(cd "$MSGS" && cc -O2 -fPIC -I "$DDS/include" -c ./*.c && ar rcs libmsgs.a ./*.o)
