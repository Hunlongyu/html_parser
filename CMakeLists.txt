cmake_minimum_required(VERSION 3.28)

project(hps VERSION 0.0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)

    if (NOT DEFINED ENV{INCLUDE})
        get_filename_component(_hps_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        get_filename_component(_hps_msvc_root "${_hps_cl_dir}/../../.." ABSOLUTE)
        set(_hps_msvc_include "${_hps_msvc_root}/include")

        if (EXISTS "${_hps_msvc_include}/cstdint")
            include_directories(SYSTEM "${_hps_msvc_include}")
        endif()

        set(_hps_windows_kits_root "C:/Program Files (x86)/Windows Kits/10")
        if (EXISTS "${_hps_windows_kits_root}/Include")
            file(GLOB _hps_sdk_include_dirs LIST_DIRECTORIES true "${_hps_windows_kits_root}/Include/*")
            list(SORT _hps_sdk_include_dirs COMPARE NATURAL ORDER DESCENDING)
            list(GET _hps_sdk_include_dirs 0 _hps_sdk_include_root)
            get_filename_component(_hps_windows_sdk_version "${_hps_sdk_include_root}" NAME)

            include_directories(SYSTEM
                "${_hps_windows_kits_root}/Include/${_hps_windows_sdk_version}/shared"
                "${_hps_windows_kits_root}/Include/${_hps_windows_sdk_version}/um"
                "${_hps_windows_kits_root}/Include/${_hps_windows_sdk_version}/ucrt"
            )

            if (CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(_hps_arch x64)
            else()
                set(_hps_arch x86)
            endif()

            if (EXISTS "${_hps_msvc_root}/lib/${_hps_arch}")
                link_directories("${_hps_msvc_root}/lib/${_hps_arch}")
            endif()
            if (EXISTS "${_hps_windows_kits_root}/Lib/${_hps_windows_sdk_version}/um/${_hps_arch}")
                link_directories("${_hps_windows_kits_root}/Lib/${_hps_windows_sdk_version}/um/${_hps_arch}")
            endif()
            if (EXISTS "${_hps_windows_kits_root}/Lib/${_hps_windows_sdk_version}/ucrt/${_hps_arch}")
                link_directories("${_hps_windows_kits_root}/Lib/${_hps_windows_sdk_version}/ucrt/${_hps_arch}")
            endif()
        endif()
    endif()
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hps/version.hpp"
    @ONLY
)

option(HPS_BUILD_SHARED "Build shared library" OFF)
option(HPS_BUILD_STATIC "Build static library" ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include) 

set(SOURCE
    "src/core/comment_node.cpp"
    "src/core/document.cpp"
    "src/core/element.cpp"
    "src/core/node.cpp"
    "src/core/text_node.cpp"
    "src/parsing/token.cpp"
    "src/parsing/tokenizer.cpp"
    "src/parsing/tree_builder.cpp"
    "src/parsing/html_parser.cpp"
    "src/query/css/css_lexer.cpp"
    "src/query/css/css_selector.cpp"
    "src/query/css/css_parser.cpp"
    "src/query/css/css_matcher.cpp"

    "src/query/element_query.cpp"
    "src/query/query.cpp"
    "src/hps.cpp"
)

if (HPS_BUILD_SHARED)
    add_library(hps SHARED ${SOURCE})
    set_target_properties(hps PROPERTIES OUTPUT_NAME "hps" WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if (HPS_BUILD_STATIC)
    add_library(hps_static STATIC ${SOURCE})
    set_target_properties(hps_static PROPERTIES OUTPUT_NAME "hps_static")
endif()

option(HPS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
if(HPS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(HPS_BUILD_TESTING "Build the testing tree." ${PROJECT_IS_TOP_LEVEL})
include(CTest)
if(HPS_BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(HPS_BUILD_BENCHMARK "Build the benchmark" ${PROJECT_IS_TOP_LEVEL})
if(HPS_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()
