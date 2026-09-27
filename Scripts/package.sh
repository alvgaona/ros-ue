#!/bin/sh
# Zips the plugin with Cyclone DDS and the message types prebuilt into dist/ros-ue-<version>-<os>-<arch>.zip, after setup.sh.
set -eu
cd "$(dirname "$0")/.."

VERSION=$(python3 -c "import json; print(json.load(open('RosBridge.uplugin'))['VersionName'])")
case "$(uname -s)" in Darwin) OS=macos ;; *) OS=linux ;; esac
NAME="ros-ue-$VERSION-$OS-$(uname -m)"
PLUGIN="dist/$NAME/RosBridge" # the folder name Unreal expects under Plugins/

rm -rf "dist/$NAME" "dist/$NAME.zip"
mkdir -p "$PLUGIN/ThirdParty/cyclonedds/lib" "$PLUGIN/ThirdParty/msgs" "$PLUGIN/ThirdParty/licenses"
cp -R RosBridge.uplugin Source Shaders README.md INSTALL.md LICENSE "$PLUGIN/"
cp -R ThirdParty/cyclonedds/include "$PLUGIN/ThirdParty/cyclonedds/"
cp ThirdParty/cyclonedds/lib/libddsc.a "$PLUGIN/ThirdParty/cyclonedds/lib/"
cp ThirdParty/msgs/*.h ThirdParty/msgs/libmsgs.a "$PLUGIN/ThirdParty/msgs/"
cp ThirdParty/cyclonedds-src/LICENSE "$PLUGIN/ThirdParty/licenses/cyclonedds-LICENSE"
cp ThirdParty/cyclonedds-src/NOTICE.md "$PLUGIN/ThirdParty/licenses/cyclonedds-NOTICE.md"
cp Scripts/licenses/* "$PLUGIN/ThirdParty/licenses/"
(cd "dist/$NAME" && zip -qr "../$NAME.zip" RosBridge)
echo "dist/$NAME.zip"
