# WayShine

[![CI](https://github.com/neokura/WayShine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/ci.yml)
[![Upstream Release Watch](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml)
[![GHCR](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml/badge.svg)](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml)
[![Upstream Sunshine](https://img.shields.io/github/v/release/LizardByte/Sunshine?include_prereleases&label=upstream%20Sunshine)](https://github.com/LizardByte/Sunshine/releases)
![Base](https://img.shields.io/badge/base-v2026.508.45922-blue)

WayShine is the official GitHub fork of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine) used for a
Linux-only fork line.

The goal is to build a major Linux-focused feature on top of Sunshine while
keeping the WayShine patch surface small enough to merge future Sunshine
releases.

## Fork Model

This repository preserves Sunshine's Git history so GitHub can track it as a
real fork. That is intentional and now takes priority over the earlier
clean-history idea.

- GitHub parent: `LizardByte/Sunshine`
- Upstream remote: `https://github.com/LizardByte/Sunshine.git`
- Upstream default branch: `master`
- WayShine default branch: `main`
- WayShine baseline: `v2026.508.45922`

The old clean-history experiment was renamed to
`neokura/WayShine-clean-history` and should be treated as backup only.

## Current Baseline

- Sunshine tag: `v2026.508.45922`
- Sunshine commit: `810783dc7c7200fcb613c7d0919f6c8a7bbbebb9`
- WayShine branch: `main`

Check upstream releases with:

```bash
gh release list --repo LizardByte/Sunshine --limit 5
```

The scheduled `Upstream Release Watch` workflow performs the same check and
warns when `UPSTREAM.lock` no longer matches the newest upstream release.

## Linux Scope

WayShine is scoped to Linux hosts only. The source tree still contains upstream
platform code because deleting it would make future merges noisier. WayShine
metadata, docs, automation, packaging entrypoints, and new feature work should
stay Linux-focused.

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

Build the Docker Linux matrix. By default this uses the Dockerfiles as imported
from Sunshine, including CUDA setup when enabled by the build script:

```bash
scripts/wayshine-build-linux.sh
```

Run the lighter CI-style matrix without CUDA:

```bash
WAYSHINE_BUILD_TARGET=sunshine-build \
WAYSHINE_LINUX_BUILD_ARGS="--skip-cuda" \
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

Start from a clean `main`, then merge the new Sunshine tag on an upgrade branch:

```bash
scripts/wayshine-merge-upstream.sh vYYYY.MDD.HHMMSS
```

After resolving conflicts:

```bash
scripts/wayshine-build-linux.sh
```

Update `UPSTREAM.lock`, open a pull request into `main`, and keep the merge
commit visible. Do not squash upstream release merges; preserving the merge
helps future upgrades.

## GitHub Automation

- `CI`: Linux Docker build matrix without CUDA, so every push validates the
  project without exhausting GitHub-hosted runner disk.
- `Upstream Release Watch`: scheduled/manual check against Sunshine releases.
- `GHCR`: manual or tag-triggered container publication to
  `ghcr.io/neokura/wayshine`. The default GHCR build is also no-CUDA; pass
  custom `linux_build_args` from the workflow dispatch form for heavier builds.
- Dependabot: GitHub Actions and Dockerfiles only. Sunshine source
  dependencies, submodules, and frontend packages are upgraded through upstream
  Sunshine merges.

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
