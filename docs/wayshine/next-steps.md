# WayShine Next Steps

This file tracks the work needed after the P0 Linux virtual display base is merged. WayShine stays intentionally small: a light Sunshine fork whose only product feature is native Linux virtual display handling for Moonlight.

## P0 Stabilization Gates

- Keep `linux_vdisplay_enabled=false` as the default and verify upstream Sunshine behavior stays unchanged when the feature is disabled.
- Keep exact-mode failure semantics: if the requested mode is not visible on the configured virtual DRM connector, the stream must fail before capture.
- Verify that every P0 path logs the DRM connector, KScreen output, Sunshine capture output, requested mode, and applied mode.
- Keep `kscreen-doctor` as the only P0 KDE Wayland backend behind `LinuxDisplayControlBackend`.
- Keep HDR, runtime hotplug, arbitrary runtime EDID generation, and custom EDID generation out of P0.
- Keep Bazzite and other rpm-ostree systems marked as partially supported until persistence is validated on real hardware.

## Manual Validation Matrix

- Fedora KDE Wayland, mutable install, NVIDIA proprietary driver.
- P0 SDR EDID installed at boot with `drm.edid_firmware=<CONNECTOR>:edid/wayshine-sdr-default.bin video=<CONNECTOR>:e`.
- 1920x1080, 2560x1440, and 3840x2160 streams only when those modes appear in `/sys/class/drm/<connector>/modes`.
- Missing-mode request rejected before app launch and before capture.
- Ambiguous DRM/KScreen/capture mapping rejected without fallback to a physical display.
- Session stop, failed preflight, app failure, cancellation, and Sunshine restart restore the captured display snapshot.

## P1 Implementation Work

- Add a native helper linked against libkscreen while preserving the existing `LinuxDisplayControlBackend` interface.
- Add deeper capture-output correlation once the capture stack exposes stable output identifiers.
- Expand automated doctor fixtures for KDE power management edge cases: screen blanking, autolock, DPMS, and energy saving.
- Add GitHub Actions coverage for Linux virtual display unit tests on the smallest practical Linux build target.
- Add packaging notes for Fedora KDE users once the install/remove flow is validated end to end.

## P2 Exploration

- Add custom EDID generation through `wayshine --linux-vdisplay-generate-edid`.
- Store generated profile metadata next to each `.bin` file and validate checksums before install.
- Run `edid-decode` validation when the tool is available.
- Investigate a direct KScreen DBus backend only after a proof of concept confirms stability, atomic application, and reliable restore semantics.
- Revisit Bazzite/rpm-ostree automation after confirming persistent EDID firmware, initramfs, and kernel argument behavior.

## Release Checklist

- `git diff --check`
- Linux CI green for build and tests.
- Non-Linux CI green with the feature compiled out.
- Doctor output reviewed on a real Fedora KDE Wayland host.
- README and `FORK.md` still describe WayShine as a single-feature fork with no legacy backup naming.
