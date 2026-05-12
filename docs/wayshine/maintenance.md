# WayShine Maintenance

WayShine is an official GitHub fork of Sunshine. It preserves Sunshine's full
Git history so GitHub can keep the fork relationship intact.

## Invariants

- Stay Linux-only in WayShine CI, packaging, and local tooling.
- Preserve upstream Sunshine history.
- Keep WayShine-specific changes on `main` as focused commits.
- Prefer additive files under `docs/wayshine`, `scripts/wayshine-*`, and
  `.github/workflows`.
- When upstream files must be changed, keep patches small and explain the Linux
  reason in the commit message.
- Do not update Sunshine submodules, frontend packages, or vendored dependency
  pins independently of upstream unless a WayShine feature explicitly requires
  it.

## Initial Baseline

WayShine starts from Sunshine `v2026.508.45922`.

```text
upstream commit: 810783dc7c7200fcb613c7d0919f6c8a7bbbebb9
```

## Upgrade Workflow

1. Confirm the new Sunshine tag on the official release page or in the
   `Upstream Release Watch` workflow summary.
2. Start from a clean tree on `main`.
3. Create an upgrade branch and merge the new upstream tag:

```bash
scripts/wayshine-merge-upstream.sh vYYYY.MDD.HHMMSS
```

4. Resolve conflicts by preserving WayShine's Linux-only intent.
5. Update `UPSTREAM.lock` with the new base tag, base commit, and sync time.
6. Run the Linux build matrix:

```bash
scripts/wayshine-build-linux.sh
```

7. Open a pull request into `main`.

Do not squash upstream release merges. A visible merge commit makes the next
Sunshine upgrade easier to reason about.

## Local Build Stack

Sunshine's Linux build script currently supports Debian-based, Fedora-based, and
Arch-based systems. For native builds on those distributions, use:

```bash
scripts/wayshine-build-native-linux.sh
```

WayShine's local Docker wrapper targets the Linux container environments present
in this baseline:

- `ubuntu-22.04`
- `ubuntu-24.04`
- `ubuntu-26.04`
- `debian-trixie`

The wrapper uses `docker buildx` when available and falls back to `docker build`
for local machines that do not have the buildx plugin installed. Install buildx
for reliable cross-platform builds, especially from arm64 hosts targeting amd64.

## GitHub Workflows

- `CI`: builds the Linux Docker matrix on pushes and pull requests.
- The default CI build passes `--skip-cuda` and targets `sunshine-build` so
  ordinary pushes do not download the multi-GB CUDA runfile on GitHub-hosted
  runners.
- `Upstream Release Watch`: scheduled/manual release check against
  `LizardByte/Sunshine`.
- `GHCR`: manual or tag-triggered image publication to
  `ghcr.io/neokura/wayshine`.

There is no `localize` workflow in WayShine yet. Adding one before WayShine owns
translation changes would create maintenance noise and a misleading badge.

## Conflict Policy

During an upstream merge, treat conflicts in this order:

1. Keep upstream changes in generic Sunshine code unless they break Linux.
2. Keep WayShine changes in Linux packaging, Linux CI, and WayShine docs/scripts.
3. Re-apply local patches as new commits only when a merge conflict cannot keep
   the old patch cleanly.
4. If a Linux-only change becomes unnecessary because Sunshine fixed it
   upstream, remove the local patch in a dedicated commit.
