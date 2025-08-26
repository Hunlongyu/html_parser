cmake_minimum_required(VERSION 3.28)

project(hps_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXAMPLES
    basic
)

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE hps)
    target_link_libraries(${example} PRIVATE hps_static)
    target_include_directories(${example} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
endforeach()