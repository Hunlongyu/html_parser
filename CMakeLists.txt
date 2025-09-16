cmake_minimum_required(VERSION 3.28)

project(hps VERSION 0.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/hps/version.hpp"
    @ONLY
)

option(HPS_BUILD_SHARED "Build shared library" OFF)
option(HPS_BUILD_STATIC "Build static library" ON)
option(HPS_BUILD_EXAMPLES "Build examples" ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include) 

set(SOURCE 
    "src/hps.cpp"
    "src/core/comment_node.cpp"
    "src/core/document.cpp"
    "src/core/element.cpp"
    "src/core/node.cpp"
    "src/core/text_node.cpp"
    "src/parsing/token.cpp"
    "src/parsing/tokenizer.cpp"
    "src/parsing/tree_builder.cpp"
    "src/parsing/html_parser.cpp"
    "src/query/query.cpp"
    "src/query/element_query.cpp"
    "src/query/css/css_selector.cpp"
    "src/query/css/css_parser.cpp"
    "src/query/css/css_matcher.cpp"
)

if (HPS_BUILD_SHARED)
    add_library(hps SHARED ${SOURCE})
endif()

if (HPS_BUILD_STATIC)
    add_library(hps_static STATIC ${SOURCE})
    set_target_properties(hps_static PROPERTIES OUTPUT_NAME "hps_static")
endif()

if(HPS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()