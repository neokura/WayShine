# WayShine Agent Guide

WayShine is the official GitHub fork of `LizardByte/Sunshine` for a Linux-only
fork line.

## Non-Negotiables

- Keep the project Linux-only unless the user explicitly changes scope.
- Preserve upstream Sunshine history; this is required for GitHub's fork
  relationship.
- Do not re-add release automation for other operating systems.
- Do not push to `upstream`; local push URL should be `no_push`.
- Keep WayShine-specific changes small and replayable on top of Sunshine tags.

## Fork Reality

This repository is a real GitHub fork. Do not return to the clean-history model
unless the user explicitly gives up the official fork relationship.

Use these files as the source of truth:

- `UPSTREAM.lock`
- `FORK.md`
- `docs/wayshine/maintenance.md`

## Preferred Patch Shape

- Add WayShine docs under `docs/wayshine`.
- Add project automation under `.github/workflows`.
- Add helper scripts as `scripts/wayshine-*`.
- When modifying upstream Sunshine files, keep diffs narrow and explain the
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
scripts/wayshine-merge-upstream.sh <sunshine-tag>
```

Resolve conflicts by preserving WayShine Linux intent, update `UPSTREAM.lock`,
then run the Linux build matrix before opening a pull request into `main`.
