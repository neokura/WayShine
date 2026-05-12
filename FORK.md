# WayShine Fork Notes

WayShine is a private, Linux-only, clean-history fork baseline of
[LizardByte/Sunshine](https://github.com/LizardByte/Sunshine).

The repository is intentionally not a GitHub network fork. GitHub requires a
shared Git history to show the official "forked from" relationship, while this
project keeps only a parentless source snapshot and local WayShine commits.

This is the correct trade-off for the current requirement: private, clean
history, Linux-only, and easy to rebase conceptually onto future Sunshine source
snapshots. If the GitHub fork badge becomes mandatory, the project must give up
the "no upstream history" constraint and be recreated as a normal GitHub fork.

## Current Upstream

- Upstream repo: `https://github.com/LizardByte/Sunshine.git`
- Imported tag: `v2026.508.45922`
- Upstream commit: `810783dc7c7200fcb613c7d0919f6c8a7bbbebb9`
- Snapshot branch: `upstream-snapshot`

## Branch Model

- `upstream-snapshot`: source snapshots imported from Sunshine with
  `scripts/wayshine-import-upstream.sh`. Do not edit this branch manually.
- `main`: WayShine's Linux-only branch. Keep project-specific changes as small,
  reviewable commits on top of the imported snapshot.

For the full upgrade workflow, see `docs/wayshine/maintenance.md`.

## GitHub Integration

- `CI` validates the Linux Docker build matrix.
- `Upstream Release Watch` checks whether Sunshine has a newer release than
  `UPSTREAM.lock`.
- `GHCR` publishes Linux images manually or from `wayshine-v*` tags.
- No WayShine `localize` workflow exists yet because localization remains
  upstream-owned.
