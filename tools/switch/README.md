# Switch build image

`ghcr.io/sylphiawindy/nx-builder` is devkitA64 plus the portlibs this engine
needs. The release workflow builds inside it, so a release never fetches
packages of its own.

## Why it is built by hand

`pkg.devkitpro.org` sits behind a CDN that answers 403 to GitHub's runners.
Ten attempts across two runs, on different runners, all refused. It is not
intermittent and retrying does not help: the block follows the address, not the
moment. A machine of your own gets through, which is the whole reason the image
exists rather than the packages being installed per build.

## Rebuilding

On a machine that can reach pkg.devkitpro.org, with `docker login ghcr.io`
already done under a token carrying `write:packages`:

    docker build --network host \
        -t ghcr.io/sylphiawindy/nx-builder:latest \
        -t ghcr.io/sylphiawindy/nx-builder:$(date +%Y-%m-%d) \
        -f tools/switch/Dockerfile tools/switch

    docker push ghcr.io/sylphiawindy/nx-builder:latest
    docker push ghcr.io/sylphiawindy/nx-builder:$(date +%Y-%m-%d)

`--network host` matters: the 403 appears on Docker's bridge network and not on
the host's.

Rebuild when the Dockerfile changes, or to pick up newer portlibs. Pinning the
image also pins those versions, so local builds and CI compile against the same
libraries instead of each fetching whatever is current.

## Visibility

The package is public. Nothing in it is private, and a public package is pulled
without credentials, which keeps the workflow free of them. A package pushed
from outside Actions is private by default and is not linked to the repository,
so the workflow's token cannot read it: that combination fails with a bare
`denied` at container startup.
