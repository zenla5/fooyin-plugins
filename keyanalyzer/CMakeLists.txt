cmake_minimum_required(VERSION 3.16)

project(
    keyanalyzer
    VERSION 0.1.0
    DESCRIPTION "Key Analyzer plugin for fooyin"
    HOMEPAGE_URL "https://forgejo.zenlab.vip/zenlab/fooyin-plugins"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
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
