cmake_minimum_required(VERSION 3.28)

project(hps VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

option(HPS_BUILD_SHARED "Build shared library" ON)
option(HPS_BUILD_STATIC "Build static library" ON)
option(HPS_BUILD_EXAMPLES "Build examples" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCE "src/hps.cpp")

add_library(hps ${SOURCE})

if(HPS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()