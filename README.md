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
    *append at end*.
- Worker-thread concurrency (auto or a fixed count).

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
