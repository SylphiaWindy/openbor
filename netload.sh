#!/usr/bin/env bash
# Push the engine to a Switch running hbmenu's netloader and launch it.
#
# On the Switch: open hbmenu and press Y to start the netloader.
#   ./netload.sh              auto-discover by broadcast
#   ./netload.sh 10.1.1.81    send to a known address
#
# The pak is read from the SD card as usual, so it does not need copying.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
NRO="$HERE/build.switch/FFLNS.nro"
IMAGE="nx-builder:latest"

[ -f "$NRO" ] || { echo "not built yet: $NRO — run ./build-switch.sh first" >&2; exit 1; }

ADDR_ARG=()
[ $# -ge 1 ] && ADDR_ARG=(-a "$1")

TTY_ARG=()
[ -t 0 ] && [ -t 1 ] && TTY_ARG=(-it)

# --network host: nxlink needs UDP broadcast to find the Switch and a direct
# TCP connection back to it.
exec docker run --rm "${TTY_ARG[@]}" --network host \
    -v "$HERE":/src -w /src \
    "$IMAGE" nxlink -s "${ADDR_ARG[@]}" build.switch/FFLNS.nro
