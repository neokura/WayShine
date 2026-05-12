# WayShine Docker

WayShine uses the Linux Dockerfiles imported from Sunshine as reproducible build
environments and optional runtime images.

## Build Locally

Build every supported Linux target:

```bash
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
`wayshine-v*`.

Expected image names:

```text
ghcr.io/neokura/wayshine:<version>-ubuntu-22.04
ghcr.io/neokura/wayshine:<version>-ubuntu-24.04
ghcr.io/neokura/wayshine:<version>-ubuntu-26.04
ghcr.io/neokura/wayshine:<version>-debian-trixie
```

The workflow also updates `latest-<target>` tags for tag-triggered releases.
