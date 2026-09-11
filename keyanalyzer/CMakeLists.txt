cmake_minimum_required(VERSION 3.16)

project(
    keyanalyzer
    VERSION 0.3.0
    DESCRIPTION "Key Analyzer plugin for fooyin"
    HOMEPAGE_URL "https://github.com/zenla5/fooyin-plugins"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

# Dev-only optional targets (off for the nix derivation / normal releases).
option(KEYANALYZER_BUILD_TOOLS "Build the keyanalyzer_cli verification tool" OFF)
option(KEYANALYZER_BUILD_TESTS "Build the keyanalyzer unit tests" OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Fooyin REQUIRED)
find_package(KeyFinder REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Concurrent Widgets)

# The exported Fooyin::Core/Gui/Utils targets have no usable include dirs
# (fooyin's install(EXPORT ...) records a broken path that is stripped at
# build time), which also breaks AUTOMOC ("Undefined interface" / missing
# headers). Point them at the fooyin dev include root.
if(NOT DEFINED FOOYIN_INCLUDE_DIR)
    # default: derive from Fooyin_DIR (<prefix>/lib/cmake/fooyin)
    get_filename_component(_prefix "${FOOYIN_DIR}/../../../" ABSOLUTE)
    set(FOOYIN_INCLUDE_DIR "${_prefix}/include/fooyin")
endif()
set_target_properties(Fooyin::Core  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FOOYIN_INCLUDE_DIR}")
set_target_properties(Fooyin::Gui   PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FOOYIN_INCLUDE_DIR}")
set_target_properties(Fooyin::Utils PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FOOYIN_INCLUDE_DIR}")

# Fooyin's exported FooyinConfig bakes an ABSOLUTE FOOYIN_PLUGIN_INSTALL_DIR
# (the fooyin store's plugindir), which would make cmake install the .so into
# the read-only fooyin store. Override it to our own prefix's relative dir.
set(FOOYIN_PLUGIN_INSTALL_DIR "lib/fooyin/plugins")

create_fooyin_plugin(
    keyanalyzer
    JSON_IN src/keyanalyzer.json.in
    DEPENDS Fooyin::Core
            Fooyin::Gui
            Qt6::Concurrent
            KeyFinder::keyfinder
    SOURCES src/keyanalyzerdefs.h
            src/keyanalyzernotations.h
            src/keyanalyzernotations.cpp
            src/keyanalyzercomments.h
            src/keyanalyzercomments.cpp
            src/keyanalyzerresult.h
            src/keyanalyzerworker.h
            src/keyanalyzerworker.cpp
            src/keyanalyzerscanner.h
            src/keyanalyzerscanner.cpp
            src/keyanalyzerresultsmodel.h
            src/keyanalyzerresultsmodel.cpp
            src/keyanalyzerresults.h
            src/keyanalyzerresults.cpp
            src/keyanalyzerplugin_settings.h
            src/keyanalyzerplugin_settings.cpp
            src/keyanalyzerplugin.h
            src/keyanalyzerplugin.cpp
)

# ---- Dev-only: unit tests (pure comment/notation logic; no fooyin) ----
if(KEYANALYZER_BUILD_TESTS)
    enable_testing()
    add_executable(keyanalyzer_test_comments
        tests/test_comments.cpp
        src/keyanalyzercomments.cpp
    )
    target_include_directories(keyanalyzer_test_comments PRIVATE src)
    target_link_libraries(keyanalyzer_test_comments PRIVATE Qt6::Core)
    target_compile_features(keyanalyzer_test_comments PRIVATE cxx_std_20)
    add_test(NAME comments COMMAND keyanalyzer_test_comments)
endif()

# ---- Dev-only: headless pipeline verifier ----
if(KEYANALYZER_BUILD_TOOLS)
    find_package(taglib CONFIG REQUIRED)
    add_executable(keyanalyzer_cli
        tools/keyanalyzer_cli.cpp
        src/keyanalyzernotations.cpp
        src/keyanalyzercomments.cpp
    )
    target_include_directories(keyanalyzer_cli PRIVATE src)
    target_link_libraries(keyanalyzer_cli PRIVATE
        Qt6::Core
        KeyFinder::keyfinder
        TagLib::tag
    )
    target_compile_features(keyanalyzer_cli PRIVATE cxx_std_20)
endif()
