cmake_minimum_required(VERSION 3.28)

project(
    hps
    VERSION 0.3.1
    DESCRIPTION "Modern C++20 HTML parser library"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
include(CompilerFlags)
include(InstallConfig)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(HPS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(HPS_BUILD_BENCHMARKS "Build benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(HPS_ENABLE_CLANG_TIDY "Run clang-tidy for hps targets" OFF)
option(HPS_ENABLE_COVERAGE "Enable compiler coverage instrumentation for supported toolchains" OFF)
option(HPS_INSTALL "Enable install rules and CMake package export" ${PROJECT_IS_TOP_LEVEL})

function(hps_consume_legacy_cache_bool out_var cache_var)
    unset(${out_var} PARENT_SCOPE)

    if(DEFINED CACHE{${cache_var}})
        set(${out_var} "$CACHE{${cache_var}}" PARENT_SCOPE)
        unset(${cache_var} CACHE)
    endif()
endfunction()

hps_consume_legacy_cache_bool(_hps_legacy_build_static HPS_BUILD_STATIC)
hps_consume_legacy_cache_bool(_hps_legacy_build_shared HPS_BUILD_SHARED)
hps_consume_legacy_cache_bool(_hps_legacy_build_testing HPS_BUILD_TESTING)
hps_consume_legacy_cache_bool(_hps_legacy_build_benchmark HPS_BUILD_BENCHMARK)

if(DEFINED _hps_legacy_build_static)
    if(_hps_legacy_build_static)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    endif()
endif()

if(DEFINED _hps_legacy_build_shared)
    set(BUILD_SHARED_LIBS ${_hps_legacy_build_shared} CACHE BOOL "Build shared libraries" FORCE)
endif()

if(DEFINED _hps_legacy_build_testing)
    set(BUILD_TESTING ${_hps_legacy_build_testing} CACHE BOOL "Build tests" FORCE)
endif()

if(DEFINED _hps_legacy_build_benchmark)
    set(HPS_BUILD_BENCHMARKS ${_hps_legacy_build_benchmark} CACHE BOOL "Build benchmarks" FORCE)
endif()

add_subdirectory(src)

if(HPS_INSTALL)
    hps_install_project(hps)
endif()

if(HPS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(HPS_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
