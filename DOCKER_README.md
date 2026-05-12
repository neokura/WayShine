# WayShine Docker

WayShine uses the Linux Dockerfiles imported from Sunshine as reproducible build
environments and optional runtime images.

## Build Locally

Build every supported Linux target:

```bash
scripts/wayshine-build-linux.sh
```

Build the lighter CI-style matrix without CUDA:

```bash
WAYSHINE_BUILD_TARGET=sunshine-build \
WAYSHINE_LINUX_BUILD_ARGS="--skip-cuda" \
scripts/wayshine-build-linux.sh
```

Build one target:

```bash
scripts/wayshine-build-linux.sh ubuntu-24.04
```

Supported targets:

- `ubuntu-22.04`
- `ubuntu-24.04`
- `ubuntu-26.04`
- `debian-trixie`

## Runtime Ports

Expose these ports when running a WayShine container:

```bash
-p 47984-47990:47984-47990/tcp
-p 48010:48010
-p 47998-48000:47998-48000/udp
```

Mount persistent configuration at `/config`.

## GHCR

The `GHCR` workflow publishes images manually or from tags matching
`wayshine-v*`. By default it passes `--skip-cuda` to keep GitHub-hosted builds
within runner disk limits. Use the workflow dispatch `linux_build_args` input
for heavier CUDA-enabled builds on a suitable runner.

Expected image names:

```text
ghcr.io/neokura/wayshine:<version>-ubuntu-22.04
ghcr.io/neokura/wayshine:<version>-ubuntu-24.04
ghcr.io/neokura/wayshine:<version>-ubuntu-26.04
ghcr.io/neokura/wayshine:<version>-debian-trixie
```

The workflow also updates `latest-<target>` tags for tag-triggered releases.
