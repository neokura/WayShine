WayShine is a Linux-only, clean-history fork baseline of LizardByte/Sunshine.

Read `AGENTS.md`, `UPSTREAM.lock`, `FORK.md`, and
`docs/wayshine/maintenance.md` before making structural changes.

Keep platform-specific changes focused on Linux. Do not add Windows, macOS,
FreeBSD, Homebrew, Winget, or Pacman release automation unless the project scope
changes explicitly.

Use `upstream-snapshot` only for imported Sunshine source snapshots. WayShine
changes belong on `main` as small commits that can be reviewed and replayed
during future upstream upgrades.

Prefix build directories with `cmake-build-`.

The test executable is named `test_sunshine` and is located inside the `tests`
directory within the build directory.

The project uses gtest as a test framework.

Always follow the style guidelines defined in `.clang-format` for C/C++ code.
