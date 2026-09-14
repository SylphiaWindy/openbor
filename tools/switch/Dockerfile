# Build image for Nintendo Switch homebrew (devkitA64 + libnx + SDL2 portlibs).
#
# Lives here rather than beside the machine that happens to build it, so the
# build environment is reproducible from the source it serves.
#
# Published to ghcr.io by hand, not by CI. See README.md: pkg.devkitpro.org
# refuses GitHub's runners, so this cannot be built there.
FROM devkitpro/devkita64:latest

LABEL org.opencontainers.image.source=https://github.com/SylphiaWindy/openbor
LABEL org.opencontainers.image.description="devkitA64 with the portlibs OpenBOR needs for Switch"

# Portlibs needed by OpenBOR and the SDL2 gamepad smoke test.
RUN dkp-pacman -Sy --noconfirm \
 && dkp-pacman -S --noconfirm --needed \
      switch-pkg-config \
      switch-sdl2 \
      switch-sdl2_gfx \
      switch-libpng \
      switch-zlib \
      switch-libogg \
      switch-libvorbisidec \
      switch-libvpx \
      switch-mesa \
      switch-glad \
 && dkp-pacman -Scc --noconfirm

ENV DEVKITPRO=/opt/devkitpro \
    DEVKITA64=/opt/devkitpro/devkitA64 \
    PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:$PATH

WORKDIR /src
