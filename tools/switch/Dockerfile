# Build image for Nintendo Switch homebrew (devkitA64 + libnx + SDL2 portlibs).
#
# Lives here rather than beside the machine that happens to build it, so the
# build environment is reproducible from the source it serves.
#
# Published to ghcr.io by .github/workflows/nx-builder-image.yml and pulled by
# the release workflow, which therefore never talks to pkg.devkitpro.org. That
# server answers 403 often enough to fail roughly one release in three when the
# packages are fetched at build time.
FROM devkitpro/devkita64:latest

LABEL org.opencontainers.image.source=https://github.com/SylphiaWindy/openbor
LABEL org.opencontainers.image.description="devkitA64 with the portlibs OpenBOR needs for Switch"

# Portlibs needed by OpenBOR and the SDL2 gamepad smoke test.
#
# Retried: pkg.devkitpro.org sits behind a CDN that answers 403 to some
# datacentre addresses, intermittently rather than consistently, so another
# attempt usually gets through.
RUN for attempt in 1 2 3 4 5; do \
      dkp-pacman -Sy --noconfirm \
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
   && break; \
      echo "package fetch failed, attempt $attempt"; \
      [ "$attempt" = 5 ] && exit 1; \
      sleep 20; \
    done \
 && dkp-pacman -Scc --noconfirm

ENV DEVKITPRO=/opt/devkitpro \
    DEVKITA64=/opt/devkitpro/devkitA64 \
    PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:$PATH

WORKDIR /src
