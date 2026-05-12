WayShine is the official GitHub fork of LizardByte/Sunshine for a Linux-only
fork line.

Read `AGENTS.md`, `UPSTREAM.lock`, `FORK.md`, and
`docs/wayshine/maintenance.md` before making structural changes.

Keep platform-specific changes focused on Linux. Do not add Windows, macOS,
FreeBSD, Homebrew, Winget, or Pacman release automation unless the project scope
changes explicitly.

WayShine changes belong on `main` as small commits that can be reviewed during
future upstream release merges.

Prefix build directories with `cmake-build-`.

The test executable is named `test_sunshine` and is located inside the `tests`
directory within the build directory.

The project uses gtest as a test framework.

Always follow the style guidelines defined in `.clang-format` for C/C++ code.
