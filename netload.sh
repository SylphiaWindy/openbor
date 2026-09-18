#!/usr/bin/env bash
# Push the engine to a Switch running hbmenu's netloader and launch it.
#
# On the Switch: open hbmenu and press Y to start the netloader.
#
#   ./netload.sh <ip> <dest>
#
#   ./netload.sh 10.1.1.81 sorx/SoRX.nro
#   NRO_DIR=build.debug ./netload.sh 10.1.1.81 sorx/SoRX.nro   (debug build)
#
# <dest> is the destination path INCLUDING the file name, relative to hbmenu's
# own root, which is sdmc:/switch. Getting it wrong fails in two distinct ways:
#
#   switch/SoRX             file-extension/filename not recognized
#                           nxlink sends this string verbatim as the file name
#                           and never appends the basename, so hbmenu sees no
#                           .nro extension.
#   switch/sorx/SoRX.nro        No such file or directory
#                           hbmenu prepends its root, giving
#                           /switch/switch/sorx/SoRX.nro.
#   sorx/SoRX.nro               works.
#
# The destination has to be the directory the paks live in: on Switch the engine
# leaves rootDir empty and looks for "Paks" relative to the working directory,
# which is wherever the NRO sits.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
NRO="$HERE/${NRO_DIR:-build.switch}/SoRX.nro"
IMAGE="nx-builder:latest"

[ -f "$NRO" ] || { echo "not built yet: $NRO — run ./build-switch.sh first" >&2; exit 1; }

ADDR="${1:-}"
SDPATH="${2:-}"

ARGS=()
[ -n "$ADDR" ]   && ARGS+=(-a "$ADDR")
[ -n "$SDPATH" ] && ARGS+=(-p "$SDPATH")

if [ -z "$SDPATH" ]; then
    echo "warning: no destination given, the engine will look for Paks next to" >&2
    echo "         wherever nxlink drops the NRO and will likely find none." >&2
fi

TTY_ARG=()
[ -t 0 ] && [ -t 1 ] && TTY_ARG=(-it)

# --network host: nxlink needs UDP broadcast to find the Switch and a direct
# TCP connection back to it.
exec docker run --rm "${TTY_ARG[@]}" --network host \
    -v "$HERE":/src -w /src \
    "$IMAGE" nxlink -s "${ARGS[@]}" "${NRO_DIR:-build.switch}/SoRX.nro"
