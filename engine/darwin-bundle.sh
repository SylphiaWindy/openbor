#!/usr/bin/env bash
#
# Assemble a macOS .app around an already built engine binary.
#
#   cd engine && ./darwin-bundle.sh [AppName]
#
# The engine chdir()s into the bundle's Resources directory at startup (see
# the DARWIN block in sdl/sdlport.c), so Paks, Saves, Logs and ScreenShots all
# live there. Drop pak files into <App>.app/Contents/Resources/Paks.
#
# darwin.sh, the old script for this, hunts for SDL 1 and a libpngNN that no
# Homebrew has had in years, and pairs libraries by hand in an order its own
# comment calls critical. This walks otool -L instead, so it picks up whatever
# the binary actually links - including the SDL2 that is really sdl2-compat
# sitting on top of SDL3, which no hand written list would have caught.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
NAME="${1:-FFLNS}"
BIN="$HERE/OpenBOR"
OUT="$HERE/releases/DARWIN"
APP="$OUT/$NAME.app"

[ -f "$BIN" ] || { echo "no engine binary at $BIN - build it first" >&2; exit 1; }

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks"
mkdir -p "$APP/Contents/Resources"/{Paks,Saves,Logs,ScreenShots}

cp "$BIN" "$APP/Contents/MacOS/$NAME"
cp "$HERE/resources/PkgInfo" "$APP/Contents/PkgInfo"
cp "$HERE/resources/OpenBOR.icns" "$APP/Contents/Resources/$NAME.icns"
cp "$HERE/resources/Info.plist" "$APP/Contents/Info.plist"

# version.sh regenerates Info.plist on every build with the engine's own build
# number already in it, so only the identity needs adjusting per app: two
# engines with the same bundle id would share preferences and fight over
# LSMultipleInstancesProhibited.
plutil -replace CFBundleName -string "$NAME" "$APP/Contents/Info.plist"
plutil -replace CFBundleExecutable -string "$NAME" "$APP/Contents/Info.plist"
plutil -replace CFBundleIconFile -string "$NAME" "$APP/Contents/Info.plist"
plutil -replace CFBundleIdentifier -string "org.sylphia.openbor.$(echo "$NAME" | tr '[:upper:]' '[:lower:]')" \
    "$APP/Contents/Info.plist"
# LSMinimumSystemVersion still claims 10.5, which predates the arm64 this is
# built for by thirteen years.
plutil -replace LSMinimumSystemVersion -string "11.0" "$APP/Contents/Info.plist"

FW="$APP/Contents/Frameworks"

# Copy every non-system library the binary reaches, transitively, and rewrite
# each reference to @rpath. Recursion is bounded by the copy already being
# there: a library that two others share is walked once.
bundle_libs() {
    local target="$1" lib base
    otool -L "$target" | tail -n +2 | awk '{print $1}' | while read -r lib; do
        case "$lib" in
            /usr/lib/* | /System/* | @* ) continue ;;
        esac
        base="$(basename "$lib")"
        if [ ! -f "$FW/$base" ]; then
            cp "$lib" "$FW/$base"
            chmod u+w "$FW/$base"
            install_name_tool -id "@rpath/$base" "$FW/$base"
            bundle_libs "$FW/$base"
        fi
        install_name_tool -change "$lib" "@rpath/$base" "$target"
    done
}

bundle_libs "$APP/Contents/MacOS/$NAME"

# A library that is dlopen()ed rather than linked leaves no trace in otool -L.
# sdl2-compat is one: what calls itself libSDL2 here is a shim that loads SDL3
# at runtime, and it looks for it at @loader_path first. Anything announcing a
# companion that way gets it copied in beside it, which is exactly where
# @loader_path points once both are in Frameworks.
LIBDIR="${DWNDEV:-$(brew --prefix)}/lib"
for dylib in "$FW"/*.dylib; do
    strings - "$dylib" | sed -n 's|^@loader_path/\(lib[^/]*\.dylib\)$|\1|p' | sort -u | while read -r want; do
        [ -f "$FW/$want" ] && continue
        [ -f "$LIBDIR/$want" ] || { echo "warning: $(basename "$dylib") wants $want, not in $LIBDIR" >&2; continue; }
        cp "$LIBDIR/$want" "$FW/$want"
        chmod u+w "$FW/$want"
        install_name_tool -id "@rpath/$want" "$FW/$want"
        bundle_libs "$FW/$want"
    done
done

install_name_tool -add_rpath "@executable_path/../Frameworks" "$APP/Contents/MacOS/$NAME"

# install_name_tool invalidates the signature every binary now carries, so sign
# again. Ad hoc: enough for the machine that built it and for a copy carried by
# hand, not enough to survive Gatekeeper on a download.
find "$FW" -name '*.dylib' -exec codesign --force --sign - --timestamp=none {} + 2>/dev/null
codesign --force --sign - --timestamp=none "$APP"

echo
echo "$APP"
otool -L "$APP/Contents/MacOS/$NAME" | tail -n +2 | grep -v '/usr/lib/\|/System/' || true
echo
echo "Put pak files in $APP/Contents/Resources/Paks"
