# WayShine Fork Notes

WayShine is an official GitHub fork of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine).

The project preserves upstream Sunshine history because GitHub requires shared
history to display and maintain the `forked from LizardByte/Sunshine`
relationship.

## Current Upstream

- Upstream repo: `https://github.com/LizardByte/Sunshine.git`
- GitHub parent: `LizardByte/Sunshine`
- Base tag: `v2026.508.45922`
- Base commit: `810783dc7c7200fcb613c7d0919f6c8a7bbbebb9`
- Upstream default branch: `master`
- WayShine branch: `main`

## Branch Model

- `master`: inherited fork branch matching upstream's default branch.
- `main`: WayShine's Linux-only working branch, currently based on
  `v2026.508.45922`.
- `upgrade/sunshine-*`: temporary branches created by
  `scripts/wayshine-merge-upstream.sh` for upstream release merges.

Keep project-specific changes as small, reviewable commits on top of Sunshine.
When Sunshine releases a new version, merge the release tag into an upgrade
branch and review conflicts deliberately.

## Fork Feature Boundary

WayShine's only product divergence from Sunshine is the Linux virtual display
path for Moonlight. The v1 implementation is KDE Wayland/NVIDIA focused, uses a
boot-time SDR EDID profile, and rejects missing exact modes before capture rather
than falling back to physical displays.

For the full upgrade workflow, see `docs/wayshine/maintenance.md`.

## GitHub Integration

- `CI` validates the Linux Docker build matrix.
- `Upstream Release Watch` checks whether Sunshine has a newer release than
  `UPSTREAM.lock`.
- `GHCR` publishes Linux images manually or from `wayshine-v*` tags.
- No WayShine `localize` workflow exists yet because localization remains
  upstream-owned.
