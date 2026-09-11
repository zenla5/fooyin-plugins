# fooyin-plugins

Third-party plugins for the [fooyin](https://www.fooyin.org/) music player.

## Key Analyzer

An in-GUI musical key analyzer for fooyin. Select tracks, right-click →
**Utilities → Key Analyzer…**, and fooyin detects the key of each track using
[libkeyfinder](https://mixxxdj.github.io/libkeyfinder/) (Krumhansl–Schmuckler
key detection) and lets you write the result to tags.

- **Notations** (configurable in Settings → Plugins → Key Analyzer):
  - **Camelot** (default), e.g. `8B`
  - **Open Key**, e.g. `1m`
  - **Standard**, e.g. `C# Minor`
- **Write targets:**
  - `INITIALKEY` tag (the standard musical-key tag) — on by default, with a
    "skip tracks that already have a key" option.
  - **Comment tag** — optional, with modes *overwrite*, *append at start*,
    *append at end* (whitespace-stripped and idempotent: comment markers are
    never duplicated on re-save).
- **Results dialog** (right-click a selection → Utilities → Key Analyzer…):
  - **Re-analyze (force)** — re-scan a selection ignoring the skip-existing option.
  - Per-row context menu: *Copy Keys* and *Re-analyze Selected*.
  - **Music Theory** column, e.g. `C Major (rel. A minor)`.
- Worker-thread concurrency (auto or a fixed count).
- Decoding resamples to 44.1 kHz mono before analysis (libkeyfinder is tuned
  for that rate), independent of the source sample rate/channel layout.

## Building

The plugin must be built against a **fooyin dev install** (headers +
`lib/cmake/fooyin`). `nix/fooyin.nix` builds fooyin from git with those dev
headers (pinned to the same revision the NixOS host uses, so the store path is
cached on that machine).

```sh
# From this repo, with nix:
# --impure so the devShell reuses the NixOS host's cached fooyin dev build
nix develop --impure --max-jobs 1 --cores 2
cmake -S keyanalyzer -B build -DCMAKE_BUILD_TYPE=Release \
      -DFooyin_DIR="$FOOYIN_DEV/lib/cmake/fooyin" \
      -DKeyFinder_DIR=<libkeyfinder>/lib/cmake/KeyFinder
cmake --build build
```

Install the produced `fyplugin_keyanalyzer.so` into fooyin's user plugin dir
(`~/.local/lib/fooyin/plugins/`) and restart fooyin.

### Dev-only verification

Optional CMake targets (both default OFF; they do not affect the nix
derivation):

- `-DKEYANALYZER_BUILD_TOOLS=ON` builds `keyanalyzer_cli`, a headless verifier
  that decodes with ffmpeg and reuses the plugin's notation/comment helpers:

  ```sh
  build/keyanalyzer_cli analyze <file> [--notation camelot|openkey|standard]
  build/keyanalyzer_cli write-init <file> <key>            # TagLib FLAC/MP3 round-trip
  build/keyanalyzer_cli write-comment <file> --mode overwrite|start|end <key>
  build/keyanalyzer_cli check <file>
  ```
- `-DKEYANALYZER_BUILD_TESTS=ON` builds `keyanalyzer_test_comments` (unit tests
  for the comment assembly) runnable via `ctest`.

## Nix

This repo is a flake. It exposes:

- `packages.<system>.keyanalyzer` — the plugin, built self-contained against
  this repo's own git fooyin (`nix/fooyin.nix`).
- `packages.<system>.fooyin` — the git fooyin dev build.
- `devShells.<system>.default` — the authoring environment.
- `overlays.default` — adds `fooyin-keyanalyzer` built against the *consumer's*
  `fooyin` (use this on the NixOS host, where `fooyin` is overridden to the git
  dev build). On a plain nixpkgs without a dev fooyin, use
  `packages.<system>.keyanalyzer` instead.

### Publishing

The plan is to eventually publish this on GitHub so others can use the Key
Analyzer the same way the BPM Analyzer plugin is used. When ready: create a
GitHub mirror and point the flake/consumer at it.
