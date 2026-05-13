# WayShine

[![CI](https://github.com/neokura/WayShine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/ci.yml)
[![Upstream Release Watch](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml/badge.svg?branch=main)](https://github.com/neokura/WayShine/actions/workflows/upstream-release-watch.yml)
[![GHCR](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml/badge.svg)](https://github.com/neokura/WayShine/actions/workflows/ghcr.yml)
[![Upstream Sunshine](https://img.shields.io/github/v/release/LizardByte/Sunshine?include_prereleases&label=upstream%20Sunshine)](https://github.com/LizardByte/Sunshine/releases)
![Base](https://img.shields.io/badge/base-v2026.508.45922-blue)

WayShine is a light Linux-focused fork of
[Sunshine](https://github.com/LizardByte/Sunshine).

Its only product goal is to add native Linux virtual display management for
Moonlight streaming while keeping Sunshine's upstream architecture, history, and
upgrade path intact.

## What WayShine Adds

WayShine adds a strict Linux virtual display path on top of Sunshine's existing
`display_device` layer.

The v1 implementation targets:

- Linux hosts
- KDE Plasma Wayland
- NVIDIA proprietary driver
- boot-time SDR EDID virtual display
- exact mode switching before capture
- explicit failure when the requested Moonlight mode is unavailable

The important product rule is:

> WayShine v1 supports exact client resolutions only if they are predeclared in
> the installed EDID profile. Missing modes fail explicitly; there is no silent
> fallback to physical displays or 1080p.

## What WayShine Does Not Add

WayShine is intentionally not a broad rewrite of Sunshine.

The v1 scope does not include:

- arbitrary dynamic resolutions
- runtime EDID regeneration
- Apollo/SudoVDA-style hotplug display creation
- HDR virtual display support
- Windows or macOS virtual display features
- a parallel capture or encoder stack

Encoder internals stay upstream Sunshine unless a virtual display integration
bug proves otherwise.

## Linux Virtual Display Model

The v1 model uses a known-good SDR EDID profile loaded by the kernel at boot:

```text
drm.edid_firmware=<CONNECTOR>:edid/<EDID_FILE> video=<CONNECTOR>:e
```

At stream launch, WayShine performs a synchronous preflight before app launch
and before capture:

1. Parse the requested Moonlight resolution and FPS through Sunshine's
   `display_device` configuration path.
2. Verify the requested mode exists in the installed EDID profile.
3. Verify the mode is visible in `/sys/class/drm/<connector>/modes`.
4. Map the DRM connector to a KDE/KScreen output.
5. Apply the mode atomically through the P0 KDE backend.
6. Verify the applied mode before allowing capture to continue.

If any step is ambiguous or fails, the stream is rejected.

## P0 Backend

The first KDE Wayland backend is intentionally conservative:

- `kscreen-doctor` is used as the P0 display-control backend.
- It is wrapped behind WayShine's `LinuxDisplayControlBackend` abstraction.
- A later native helper linked against libkscreen can replace it without
  changing Sunshine integration points.
- Direct GDBus KScreen control is out of the critical path until a dedicated
  proof of concept validates stability, atomicity, and restore behavior.

This keeps the first implementation practical while avoiding a dependency on an
undocumented DBus contract.

## CLI

Diagnose the virtual display setup:

```bash
wayshine --linux-vdisplay-doctor
wayshine --linux-vdisplay-doctor --json
```

Install the bundled SDR EDID profile on a mutable Fedora-style system:

```bash
sudo wayshine --linux-vdisplay-install --connector DP-2 --profile sdr-default
```

Remove the installed profile and kernel args:

```bash
sudo wayshine --linux-vdisplay-remove --connector DP-2
```

Bazzite and other rpm-ostree systems are treated as partially supported in P0.
The doctor can report the state, but automated persistence of EDID firmware,
initramfs, and kernel args is not considered verified yet.

## Configuration

Minimal virtual display options:

```ini
linux_vdisplay_enabled = enabled
linux_vdisplay_connector = DP-2
linux_vdisplay_profile = sdr-default
linux_vdisplay_backend = kscreen_doctor
linux_vdisplay_mode_policy = exact
linux_vdisplay_restore_on_startup = enabled
```

WayShine reuses Sunshine's existing `dd_*` display-device options for
resolution, refresh rate, primary display, only-display, and revert behavior.

For strict capture mapping, either set `output_name` to the virtual connector
or use a `dd_configuration_option` that makes the virtual output primary or the
only active display.

## Fork Model

WayShine preserves Sunshine's Git history so GitHub can maintain the official
fork relationship.

- GitHub parent: `LizardByte/Sunshine`
- Upstream remote: `https://github.com/LizardByte/Sunshine.git`
- Upstream default branch: `master`
- WayShine default branch: `main`
- WayShine baseline: `v2026.508.45922`

Project-specific changes should remain small, Linux-scoped, and easy to review
on top of Sunshine.

## Build

Initialize dependencies after cloning:

```bash
git submodule update --init --recursive --depth 1
```

Run the Linux Docker matrix:

```bash
scripts/wayshine-build-linux.sh
```

Run the lighter CI-style matrix without CUDA:

```bash
WAYSHINE_BUILD_TARGET=sunshine-build \
WAYSHINE_LINUX_BUILD_ARGS="--skip-cuda" \
scripts/wayshine-build-linux.sh
```

Run a native Linux build:

```bash
scripts/wayshine-build-native-linux.sh
```

## Upgrading From Sunshine

Start from a clean `main`, then merge a new Sunshine release tag on an upgrade
branch:

```bash
scripts/wayshine-merge-upstream.sh vYYYY.MDD.HHMMSS
```

After resolving conflicts, run the Linux build matrix and update
`UPSTREAM.lock`. Do not squash upstream release merges; visible merge commits
make future Sunshine upgrades easier.

For the full workflow, see `docs/wayshine/maintenance.md`.

## Agent Entry Points

For a fresh Codex or Copilot agent, read these first:

- `AGENTS.md`
- `FORK.md`
- `UPSTREAM.lock`
- `docs/wayshine/maintenance.md`

Do not add unrelated fork features. WayShine should stay a small Sunshine fork
focused on native Linux virtual display management.
