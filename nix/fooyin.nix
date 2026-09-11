# Build fooyin from git WITH public development headers + exported CMake config.
#
# This MUST stay byte-identical to the `fooyin` overlay in the NixOS host repo
# (maintained on the NixOS host) — same nixpkgs rev, same git commit, same
# flags, same postFixup. That way the derivation's store path matches the one
# already built on the machine, and `nix develop`/`nix build` here reuse the
# cache instead of recompiling fooyin (~40 min).
#
# The plugin needs the dev install (INSTALL_HEADERS=ON -> headers +
# lib/cmake/fooyin) so third-party plugins can find_package(Fooyin) and link
# the SAME libs the running app uses (guaranteed ABI match).

{ lib
, stdenv
, fetchFromGitHub
, cmake
, ninja
, pkg-config
, kdePackages
, taglib
, ffmpeg
, icu
, kdsingleapplication
, alsa-lib
, pipewire
, SDL2
, libebur128
, libvgm
, libsndfile
, libarchive
, libopenmpt
, game-music-emu
, gtk3
, gnused
}:

stdenv.mkDerivation {
  pname = "fooyin";
  version = "0.9.2-git-20260823";

  src = fetchFromGitHub {
    owner = "fooyin";
    repo = "fooyin";
    rev = "8b80f417e83d40c7906a9801a4e3794a567e0ead";
    hash = "sha256-FU08nluQK5sWEjZeM/8K8pISN6r+aEvCLlXV2QGQ7Ao=";
  };

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    kdePackages.qttools
    kdePackages.wrapQtAppsHook
  ];

  buildInputs = [
    kdePackages.qcoro
    kdePackages.qtbase
    kdePackages.qtsvg
    kdePackages.qtwayland
    taglib
    ffmpeg
    icu
    kdsingleapplication
    alsa-lib
    pipewire
    SDL2
    libebur128
    libvgm
    libsndfile
    libarchive
    libopenmpt
    game-music-emu
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DINSTALL_HEADERS=ON"
    "-DBUILD_TESTING=OFF"
  ];

  # The "+"-to-add-a-library folder chooser uses Qt's GTK3 platform dialog
  # (libqgtk3.so), and GSettings aborts with "Settings schema
  # 'org.gtk.Settings.FileChooser' is not installed" (SIGABRT) when that schema
  # dir is missing from XDG_DATA_DIRS. wrapQtAppsHook uses --prefix
  # XDG_DATA_DIRS, which resets it to just the app's own share dir when the
  # session doesn't already export it. Inject the gtk3 schema dir (same fix the
  # NixOS host applies to Gittyup).
  qtWrapperArgs = [
    "--prefix" "XDG_DATA_DIRS" ":" "${gtk3}/share/gsettings-schemas/${gtk3.name}"
  ];

  # Fooyin's install(EXPORT ... INCLUDES DESTINATION) records a broken
  # relative-to-store include path
  # (${_IMPORT_PREFIX}//nix/store/<store>/include/fooyin/..) which makes CMake
  # reject the imported Fooyin::Core target ("includes non-existent path").
  # Strip those bogus INTERFACE_INCLUDE_DIRECTORIES lines; the plugin
  # derivation supplies -I<store>/include/fooyin itself. (On master this export
  # may already be correct; if so the sed just no-ops.)
  postFixup = ''
    if [ -f "$out/lib/cmake/fooyin/FooyinTargets.cmake" ]; then
      ${gnused}/bin/sed -i '/\/include\/fooyin\/\.\./d' \
        "$out/lib/cmake/fooyin/FooyinTargets.cmake"
    fi
  '';

  meta = with lib; {
    homepage = "https://www.fooyin.org/";
    description = "Customisable music player (git build; ABI matches fooyin-keyanalyzer)";
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    mainProgram = "fooyin";
  };
}
