#!/usr/bin/env bash
# Build the stock OpenBOR engine into a Switch NRO.
#
#   ./build-switch.sh          incremental build
#   ./build-switch.sh clean    wipe the build directory first
#
# Output: build.switch/OpenBOR.nro
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
IMAGE="nx-builder:latest"
BUILD="build.switch"

[ "${1:-}" = "clean" ] && rm -rf "$HERE/$BUILD"

# The version in the NACP comes from git, so the container has to be able to
# run it. A plain checkout keeps everything under $HERE and is already mounted,
# but a linked worktree's .git is a file pointing outside the tree, and an
# unreachable target makes git fail silently and the build fall back to a bare
# "3.0". Mount whatever git names, read only, at the same path it names.
GIT_MOUNTS=()
for d in "$(git -C "$HERE" rev-parse --absolute-git-dir 2>/dev/null)" \
         "$(git -C "$HERE" rev-parse --path-format=absolute --git-common-dir 2>/dev/null)"; do
    case "$d" in
        "" | "$HERE"/*) ;;
        *) GIT_MOUNTS+=(-v "$d:$d:ro") ;;
    esac
done

# --network host: the bridge network gets 403 from pkg.devkitpro.org.
docker run --rm --network host \
    -v "$HERE":/src -w /src \
    ${GIT_MOUNTS+"${GIT_MOUNTS[@]}"} \
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
