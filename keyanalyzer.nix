# Nix derivation for the fooyin Key Analyzer plugin.
#
# This is callPackage-able and takes an explicit `fooyin` argument (the fooyin
# dev install to compile against). `pkgs.callPackage` auto-fills the rest
# (libkeyfinder, fftw, qtbase, cmake, ...) from the surrounding nixpkgs.
#
# Consumers:
#   - This repo's own flake builds it against nix/fooyin.nix (self-contained).
#   - The NixOS host's overlay (`overlays.default`) builds it against its own
#     `fooyin` (which it overrides to a git dev build) — same source, same ABI.

{ lib
, stdenv
, cmake
, ninja
, pkg-config
, kdePackages
, fooyin
, libkeyfinder
, fftw
}:

stdenv.mkDerivation {
  pname = "fooyin-keyanalyzer";
  version = "0.2.0";

  src = ./keyanalyzer;

  nativeBuildInputs = [ cmake ninja pkg-config ];

  buildInputs = [
    fooyin
    kdePackages.qtbase   # Qt6::Core/Widgets/Concurrent CMake configs
    libkeyfinder
    fftw                 # KeyFinder::keyfinder's INTERFACE_LINK dependency
  ];

  dontWrapQtApps = true;  # this is a library/.so, not an application

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DFooyin_DIR=${fooyin}/lib/cmake/fooyin"
    "-DFOOYIN_INCLUDE_DIR=${fooyin}/include/fooyin"
    "-DKeyFinder_DIR=${libkeyfinder}/lib/cmake/KeyFinder"
  ];

  # fooyin's create_fooyin_plugin macro configure_file()s "<plugin>.json.in"
  # from the plugin's source root while the file lives under src/; copy it up
  # so the metadata JSON is generated and keyanalyzerplugin.h can
  # #include "keyanalyzer.json".
  preConfigure = ''
    cp src/keyanalyzer.json.in keyanalyzer.json.in
  '';

  # create_fooyin_plugin installs to $out/lib/fooyin/plugins; collect the .so
  # into $out/lib so the host can drop it into ~/.local/lib/fooyin/plugins.
  postInstall = ''
    mkdir -p "$out/lib"
    cp -v "$out/lib/fooyin/plugins/"*.so "$out/lib/" 2>/dev/null || true
  '';

  meta = with lib; {
    homepage = "https://github.com/zenla5/fooyin-plugins";
    description = "In-GUI musical key analyzer (Camelot/Open Key/Standard) for the fooyin player, using libkeyfinder";
    longDescription = ''
      Adds a "Utilities -> Key Analyzer..." action to fooyin that detects the
      musical key of selected tracks (Krumhansl-Schmuckler via libkeyfinder)
      and writes it as Camelot (default), Open Key, or Standard notation to the
      INITIALKEY tag and/or the Comment tag (overwrite / append start / append
      end).
    '';
    license = licenses.gpl3Only;
    platforms = platforms.linux;
  };
}
