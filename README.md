# WayShine

[![CI](https://github.com/neokura/WayShine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/ci.yml)
[![Upstream Release Watch](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml)
[![GHCR](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml/badge.svg)](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml)
[![Upstream Sunshine](https://img.shields.io/github/v/release/LizardByte/Sunshine?include_prereleases&label=upstream%20Sunshine)](https://github.com/LizardByte/Sunshine/releases)
![Base](https://img.shields.io/badge/base-v2026.508.45922-blue)

WayShine is a private, Linux-only, clean-history fork baseline of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine), the self-hosted
game streaming host for Moonlight.

The goal is to build a major Linux-focused feature on top of Sunshine while
keeping the local patch surface small enough to absorb future Sunshine releases.

## Fork Model

This repository is not a GitHub network fork. That is intentional.

GitHub only shows the official "forked from" relationship when the repository
shares upstream Git history. WayShine keeps a parentless Sunshine source snapshot
instead, so GitHub reports `isFork=false`. The upstream relationship is tracked
through:

- remote `upstream` pointing at `https://github.com/LizardByte/Sunshine.git`
- branch `upstream-snapshot`
- `UPSTREAM.lock`
- `FORK.md`
- `docs/wayshine/maintenance.md`

If the official GitHub fork badge ever becomes more important than clean
history, create a normal GitHub fork and accept that Sunshine's upstream history
will be part of the repository model.

## Current Baseline

- Sunshine tag: `v2026.508.45922`
- Sunshine commit: `810783dc7c7200fcb613c7d0919f6c8a7bbbebb9`
- Snapshot commit: `4b666914617c406f9e3197dbaf210e808867f8f0`
- WayShine branch: `main`
- Vendor branch: `upstream-snapshot`

The latest upstream Sunshine release should be checked with:

```bash
gh release list --repo LizardByte/Sunshine --limit 5
```

The scheduled `Upstream Release Watch` workflow performs the same check and
warns when `UPSTREAM.lock` no longer matches the newest upstream release.

## Linux Scope

WayShine is scoped to Linux hosts only. The imported source tree may still
contain upstream platform code because removing source files would create a
larger upgrade burden. WayShine project metadata, docs, automation, and build
entrypoints stay Linux-focused.

Supported Linux build environments in this baseline:

- Ubuntu 22.04
- Ubuntu 24.04
- Ubuntu 26.04
- Debian Trixie
- Native Debian-based, Fedora-based, and Arch-based systems through
  `scripts/linux_build.sh`

Useful Linux capture paths:

- KMS/DRM
- X11
- NvFBC on X11
- Wayland wlroots
- XDG Desktop Portal capture

Useful Linux encoding paths:

- NVENC
- VAAPI
- Vulkan Video where supported by drivers
- software encoding

## Build

Initialize dependencies after cloning:

```bash
git submodule update --init --recursive --depth 1
```

Build the Docker Linux matrix:

```bash
scripts/wayshine-build-linux.sh
```

Build one target:

```bash
scripts/wayshine-build-linux.sh ubuntu-24.04
```

Run a native Linux build:

```bash
scripts/wayshine-build-native-linux.sh
```

On hosts without `docker buildx`, the Docker wrapper falls back to `docker
build`. Install buildx for reliable cross-platform builds, especially from arm64
hosts targeting amd64.

## Upgrade From Sunshine

Start from a clean `main`, then import the new Sunshine tag:

```bash
scripts/wayshine-import-upstream.sh vYYYY.MDD.HHMMSS
git switch main
git merge --no-ff upstream-snapshot
```

After resolving conflicts:

```bash
scripts/wayshine-build-linux.sh
```

Update `UPSTREAM.lock` and commit the merge plus lock update together.

## GitHub Automation

- `CI`: Linux Docker build matrix.
- `Upstream Release Watch`: scheduled/manual check against Sunshine releases.
- `GHCR`: manual or tag-triggered container publication to
  `ghcr.io/neokura/wayshine`.
- Dependabot: GitHub Actions, npm, Dockerfiles, and git submodules.

There is intentionally no WayShine `localize` workflow yet. Localization remains
upstream-owned unless WayShine starts maintaining translation changes directly.

## Agent Entry Points

For a fresh Codex or Copilot agent, read these first:

- `AGENTS.md`
- `UPSTREAM.lock`
- `FORK.md`
- `docs/wayshine/maintenance.md`

Do not reintroduce non-Linux release automation unless the project scope changes
explicitly.
