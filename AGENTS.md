# WayShine Agent Guide

WayShine is a Linux-only, clean-history fork baseline of
`LizardByte/Sunshine`.

## Non-Negotiables

- Keep the project Linux-only unless the user explicitly changes scope.
- Do not re-add release automation for other operating systems.
- Do not push to `upstream`; local push URL is intentionally `no_push`.
- Do not edit `upstream-snapshot` manually.
- Keep WayShine-specific changes small and replayable on top of Sunshine
  snapshots.

## Fork Reality

This repository is not an official GitHub network fork because the project keeps
Sunshine's history out of `main`. GitHub cannot display "forked from Sunshine"
without shared upstream history.

Use these files as the source of truth:

- `UPSTREAM.lock`
- `FORK.md`
- `docs/wayshine/maintenance.md`

## Preferred Patch Shape

- Add WayShine docs under `docs/wayshine`.
- Add project automation under `.github/workflows`.
- Add helper scripts as `scripts/wayshine-*`.
- When modifying imported Sunshine files, keep diffs narrow and explain the
  Linux reason in the commit message.
- Prefer feature flags or isolated Linux modules for major features.

## Build Commands

Initialize submodules:

```bash
git submodule update --init --recursive --depth 1
```

Run the Linux Docker matrix:

```bash
scripts/wayshine-build-linux.sh
```

Run one Docker target:

```bash
scripts/wayshine-build-linux.sh ubuntu-24.04
```

Run a native Linux build:

```bash
scripts/wayshine-build-native-linux.sh
```

## Upgrade Command

```bash
scripts/wayshine-import-upstream.sh <sunshine-tag>
git switch main
git merge --no-ff upstream-snapshot
```

Resolve conflicts by preserving WayShine Linux intent, update `UPSTREAM.lock`,
then run the Linux build matrix before committing.
