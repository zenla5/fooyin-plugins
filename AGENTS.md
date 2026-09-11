# AGENTS.md — fooyin-plugins (opencode operating guide)

You are working in the `fooyin-plugins` repo: a **fooyin music-player plugin
monorepo**. It currently contains the **Key Analyzer** plugin (`keyanalyzer/`).

This file is auto-loaded into every opencode session for this repo. It captures
the environment, the build contract, and the traps that were solved on the
NixOS host so you don't have to rediscover them.

## Environment

- Runs on the T570 (NixOS 26.05), user `zenlab`, hostname `t570`.
- The NixOS host (separate repo `zenlab/NixOS-T570`) builds fooyin **from git**
  (commit `8b80f417e83d40c7906a9801a4e3794a567e0ead`) with `INSTALL_HEADERS=ON`
  and consumes THIS repo via `overlays.default`. The host overrides `fooyin`
  to that git dev build; the plugin therefore must compile against a **fooyin
  dev install** (headers + `lib/cmake/fooyin`) and link the SAME libs the app
  uses (ABI match).
- `nix/fooyin.nix` in THIS repo replicates that exact git fooyin dev build.
  Because the nixpkgs revision and fooyin commit are pinned to the same values
  as the host's `flake.lock`, the store path is **already cached on the
  machine** — `nix develop`/`nix build` here reuse it (~40 min fooyin build
  does NOT rerun).

## Quick build / test loop

```sh
nix develop --max-jobs 1 --cores 2
cmake -S keyanalyzer -B build -DCMAKE_BUILD_TYPE=Release \
      -DFooyin_DIR="$FOOYIN_DEV/lib/cmake/fooyin" \
      -DKeyFinder_DIR=<libkeyfinder>/lib/cmake/KeyFinder
cmake --build build
# quick GUI test: drop the .so into fooyin's user plugin dir and restart fooyin
cp build/fyplugin_keyanalyzer.so ~/.local/lib/fooyin/plugins/
```

fooyin loads user plugins from `~/.local/lib/fooyin/plugins/`.

### Smoke test (headless)

```sh
timeout 12 env QT_QPA_PLATFORM=offscreen fooyin 2>&1 | grep -iE 'error|crash|keyanalyzer'
```

A clean exit (or only benign ALSA / offscreen warnings) means the plugin loaded.

## Traps solved earlier (do not reintroduce)

1. **fooyin's `create_fooyin_plugin` JSON quirk:** the macro does
   `configure_file("<plugin>.json.in" ...)` from the plugin's SOURCE ROOT, but
   the file lives under `src/`. The derivation copies it up in `preConfigure`
   (`cp src/keyanalyzer.json.in keyanalyzer.json.in`). Without this the header's
   `#include "keyanalyzer.json"` fails at moc ("Plugin Metadata file … does not
   exist" / "Undefined interface").
2. **`FOOYIN_PLUGIN_INSTALL_DIR` absolute-path trap:** the exported
   `FooyinConfig` bakes an ABSOLUTE plugin install dir (into the read-only
   fooyin store), so cmake install gets "Permission denied". The plugin's
   `CMakeLists.txt` overrides it to a relative `lib/fooyin/plugins` right after
   `find_package(Fooyin)`.
3. **`dontWrapQtApps = true`** on the plugin derivation (it's a `.so`, not an
   app; otherwise wrapQtAppsHook's qtPreHook aborts the build).
4. **`-I<fooyin>/include/fooyin`:** the exported `Fooyin::Core` target's
   include dirs are broken (`${_IMPORT_PREFIX}//nix/store/.../include/fooyin/..`),
   which makes CMake reject the imported target. `nix/fooyin.nix` strips those
   bogus lines (`sed '/\/include\/fooyin\/\.\./d'`); the plugin derivation
   supplies `-I<fooyin>/include/fooyin` itself via `NIX_CFLAGS_COMPILE` in the
   host overlay (or relies on `Fooyin_DIR` include dirs on master).
5. **GTK file-dialog crash (host-side):** fooyin's wrapQtAppsHook uses
   `--prefix XDG_DATA_DIRS`, which drops the gtk3 gsettings-schemas dir, so the
   "+"-add-library folder chooser aborts with "Settings schema
   'org.gtk.Settings.FileChooser' is not installed". The host injects the gtk3
   schema dir via `qtWrapperArgs`. Don't remove that from `nix/fooyin.nix`.

## Key Analyzer design notes

- **Decode path** (must match master API): `AudioLoader::loadDecoderForTrack`
  with `AudioDecoder::NoSeeking | NoInfiniteLooping` → `decoder->readBuffer(...)`
  → `Audio::convert(buf, targetFormat)` to mono F32 @ 44100 → feed libkeyfinder
  `AudioData` (`setFrameRate(44100)`, `setChannels(1)`,
  `addToSampleCount(n)`, `setSampleByFrame(i,0,v)`).
- **Detection:** `KeyFinder::KeyFinder::keyOfAudio(audio)` returns a `key_t`
  (or `SILENCE`). Map to notation via the tables in
  `keyanalyzernotations.{h,cpp}` (Camelot/OpenKey/Standard).
- **Tag write:** `Track::replaceExtraTag("INITIALKEY", key)` (and/or
  `"COMMENT"`) then `MusicLibrary::writeTrackMetadata(tracks)`. Comment modes:
  overwrite / append-start / append-end.
- **Settings keys** live in `keyanalyzerdefs.h` (namespace
  `KeyAnalyzer/...`), persisted via `FySettings` (`core/coresettings.h`).

## Contract with the NixOS host

- The host consumes this repo as a flake input and uses `overlays.default`
  (`fooyin-keyanalyzer` built against the host's `fooyin`).
- Keep `nix/fooyin.nix` and `nix/fooyin.nix`'s `fooyin` derivation byte-identical
  to the host's `fooyin` overlay so store paths keep matching (no fooyin rebuild).
- Keep `keyanalyzer.nix` `callPackage`-able with an explicit `fooyin` argument.
- Release via tags (`v0.x.y`); the host updates via
  `nix flake lock --update-input fooyin-plugins`.
- Do NOT touch the NixOS repo from this session; the host session does the
  integration + `nixos-rebuild switch`.
