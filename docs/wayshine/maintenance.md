# WayShine Maintenance

WayShine follows a vendor-branch model so it can absorb future Sunshine releases
without keeping Sunshine's full commit history.

## Invariants

- Stay Linux-only in WayShine CI, packaging, and local tooling.
- Keep `upstream-snapshot` as imported Sunshine source only.
- Keep WayShine-specific changes on `main` as focused commits.
- Prefer additive files under `docs/wayshine`, `scripts/wayshine-*`, and
  `.github/workflows/wayshine-linux.yml`.
- When upstream files must be changed, keep patches small and explain the Linux
  reason in the commit message.

## Initial Baseline

The first snapshot was imported from Sunshine `v2026.508.45922`.

```text
upstream commit: 810783dc7c7200fcb613c7d0919f6c8a7bbbebb9
snapshot commit: 4b666914617c406f9e3197dbaf210e808867f8f0
```

The snapshot commit has no parent from the Sunshine repository, so pushing this
repository does not publish Sunshine's commit history.

## Upgrade Workflow

1. Confirm the new Sunshine tag on the official release page.
2. Start from a clean tree on `main`.
3. Import the new source snapshot:

```bash
scripts/wayshine-import-upstream.sh vYYYY.MDD.HHMMSS
```

4. Merge the updated vendor branch into `main`:

```bash
git switch main
git merge --no-ff upstream-snapshot
```

5. Resolve conflicts by preserving WayShine's Linux-only intent.
6. Update `UPSTREAM.lock` with the new tag, upstream commit, snapshot commit,
   and import time.
7. Run the Linux build matrix:

```bash
scripts/wayshine-build-linux.sh
```

8. Commit the resolved merge and lock update together.

## Local Build Stack

Sunshine's Linux build script currently supports Debian-based, Fedora-based, and
Arch-based systems. For native builds on those distributions, use:

```bash
scripts/wayshine-build-native-linux.sh
```

WayShine's local Docker wrapper targets the Linux container environments present
in this snapshot:

- `ubuntu-22.04`
- `ubuntu-24.04`
- `ubuntu-26.04`
- `debian-trixie`

The wrapper uses `docker buildx` when available and falls back to `docker build`
for local machines that do not have the buildx plugin installed. Install buildx
for reliable cross-platform builds, especially from arm64 hosts targeting amd64.

## Conflict Policy

During an upstream merge, treat conflicts in this order:

1. Keep upstream changes in generic Sunshine code unless they break Linux.
2. Keep WayShine changes in Linux packaging, Linux CI, and WayShine docs/scripts.
3. Re-apply local patches as new commits instead of rewriting old snapshot
   commits.
4. If a Linux-only change becomes unnecessary because Sunshine fixed it
   upstream, remove the local patch in a dedicated commit.
