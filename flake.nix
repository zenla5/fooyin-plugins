{
  description = "fooyin-plugins: Key Analyzer plugin (Camelot / Open Key / Standard) for fooyin";

  inputs = {
    # Pin to the exact nixpkgs revision used by the NixOS host
    # (zenlab/NixOS-T570, see its flake.lock). Matching the revision makes
    # every store path here (fooyin-from-git dev build, libkeyfinder, Qt, ...)
    # equal the ones already cached on the machine, so `nix develop` and
    # `nix build` reuse the cache instead of rebuilding ~40 min of fooyin.
    nixpkgs.url = "github:NixOS/nixpkgs/0954f7ee2f6bb3dc7d4e3d0d8bcb8fd4bde4cfc5";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      # libvgm (a fooyin input) is marked unfree in nixpkgs; allow it, matching
      # the NixOS host's configuration.
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
    in
    {
      packages.${system} = {
        # fooyin built from git with dev headers (see nix/fooyin.nix).
        fooyin = pkgs.callPackage ./nix/fooyin.nix { };

        # The Key Analyzer plugin, built against this repo's own git fooyin
        # (self-contained: works standalone and on other machines / GitHub).
        keyanalyzer = pkgs.callPackage ./keyanalyzer.nix {
          fooyin = self.packages.${system}.fooyin;
        };

        default = self.packages.${system}.keyanalyzer;
      };

      # Dev shell for authoring the plugin: gives cmake/ninja + the fooyin dev
      # headers + libkeyfinder, with Fooyin_DIR/KeyFinder_DIR pre-pointed so
      # you can build & test quickly.
      #
      # FOOYIN_DEV defaults to the host's ALREADY-CACHED git fooyin dev build
      # when present (so `nix develop` here never recompiles fooyin ~40 min);
      # on other machines / GitHub it falls back to this repo's own
      # `packages.fooyin` (built once).
      devShells.${system}.default =
        let
          hostFooyin = "/nix/store/q622sdkqxj9wp0i51rzmmhgkpskn7wl9-fooyin-0.9.2-git-20260823";
          fooyinDev = if builtins.pathExists hostFooyin
            then hostFooyin
            else self.packages.${system}.fooyin;
        in
        pkgs.mkShell {
          nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config ];
          buildInputs = [
            pkgs.kdePackages.qtbase
            pkgs.libkeyfinder
            pkgs.fftw
          ];
          shellHook = ''
            export FOOYIN_DEV="${fooyinDev}"
            echo "fooyin dev headers: $FOOYIN_DEV"
            echo "Build:  cmake -S keyanalyzer -B build -DCMAKE_BUILD_TYPE=Release"
            echo "        -DFooyin_DIR=$FOOYIN_DEV/lib/cmake/fooyin"
            echo "        -DKeyFinder_DIR=${pkgs.libkeyfinder}/lib/cmake/KeyFinder"
            echo "        && cmake --build build"
          '';
        };

      # Overlay for consumers (used by the NixOS host): adds
      # `fooyin-keyanalyzer` built against the CONSUMER's `fooyin` (which the
      # host overrides to its git dev build). On a plain nixpkgs without a dev
      # fooyin this will fail; use `packages.keyanalyzer` (self-contained)
      # instead.
      overlays.default = final: prev: {
        fooyin-keyanalyzer = final.callPackage ./keyanalyzer.nix {
          fooyin = final.fooyin;
        };
      };
    };
}
