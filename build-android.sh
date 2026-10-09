#!/usr/bin/env bash
# Build the engine into an Android APK, in the android-ndk21 container.
#
#   ./build-android.sh            debug APK (signed with app/debug.keystore)
#   ./build-android.sh release    release APK; needs android/keystore.properties
#   ./build-android.sh clean      wipe Gradle's outputs first
#
# Output: engine/android/app/build/outputs/apk/<type>/OpenBOR2018.apk
#
# The image is built from /data/dockerfiles/android-ndk21 on the build host:
# JDK 17, SDK platform 35, build-tools 36.0.0, NDK 21.4.7075529 -- the NDK
# app/build.gradle pins and the one the prebuilt SDL2 and libvpx were made with.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
IMAGE="android-ndk21:latest"
TASK=assembleDebug
CLEAN=
for arg in "$@"; do
    case "$arg" in
        release) TASK=assembleRelease ;;
        clean)   CLEAN=clean ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

# version.sh needs git, and a linked worktree's .git points outside the tree;
# mount whatever git names, read only, at the same path. Same as build-switch.sh.
GIT_MOUNTS=()
for d in "$(git -C "$HERE" rev-parse --absolute-git-dir 2>/dev/null)" \
         "$(git -C "$HERE" rev-parse --path-format=absolute --git-common-dir 2>/dev/null)"; do
    case "$d" in
        "" | "$HERE"/*) ;;
        *) GIT_MOUNTS+=(-v "$d:$d:ro") ;;
    esac
done

# Gradle and the Android plugin cache several hundred MB of downloads; keep
# them in a named volume so only the first build pays for them. Docker
# creates the volume owned by root, and the build runs as the invoking user
# so its outputs are not root-owned on the host -- so hand the volume over
# first. It also doubles as HOME, which Gradle insists on being writable.
docker run --rm -v openbor-gradle-cache:/gradle-cache "$IMAGE" \
    chown "$(id -u):$(id -g)" /gradle-cache
docker run --rm --network host \
    -v "$HERE":/src -w /src/engine/android \
    -v openbor-gradle-cache:/gradle-cache \
    ${GIT_MOUNTS+"${GIT_MOUNTS[@]}"} \
    -u "$(id -u):$(id -g)" -e HOME=/gradle-cache/home \
    "$IMAGE" bash -lc "
set -euo pipefail
mkdir -p /gradle-cache/home
(cd /src/engine && bash version.sh)
./gradlew --no-daemon -q $CLEAN $TASK
"

echo
ls -lh "$HERE"/engine/android/app/build/outputs/apk/*/*.apk
