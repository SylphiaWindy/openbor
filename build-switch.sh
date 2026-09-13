#!/usr/bin/env bash
# Build the FFLNS engine into a Switch NRO.
#
#   ./build-switch.sh          incremental build
#   ./build-switch.sh clean    wipe the build directory first
#
# Output: build.switch/FFLNS.nro
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
IMAGE="nx-builder:latest"
BUILD="build.switch"

[ "${1:-}" = "clean" ] && rm -rf "$HERE/$BUILD"

# --network host: the bridge network gets 403 from pkg.devkitpro.org.
docker run --rm --network host \
    -v "$HERE":/src -w /src \
    -u "$(id -u):$(id -g)" \
    "$IMAGE" bash -lc "
set -euo pipefail
cmake -S . -B $BUILD -G 'Unix Makefiles' \
      -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build $BUILD -j \"\$(nproc)\"
"

echo
ls -lh "$HERE/$BUILD"/*.nro
