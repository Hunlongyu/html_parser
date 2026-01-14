cmake_minimum_required(VERSION 3.28)

project(hps VERSION 0.0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hps/version.hpp"
    @ONLY
)

option(HPS_BUILD_SHARED "Build shared library" OFF)
option(HPS_BUILD_STATIC "Build static library" ON)
option(HPS_BUILD_EXAMPLES "Build examples" ON)

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

if(HPS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(HPS_BUILD_TESTING "Build the testing tree." ${PROJECT_IS_TOP_LEVEL})
include(CTest)
if(HPS_BUILD_TESTING)
    add_subdirectory(tests)
endif()
