cmake_minimum_required(VERSION 3.28)

project(hps_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EXAMPLES
    basic
    tokenizer
    tree_builder
    css_selector
    css_parser
    css_matcher
)

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE hps_static)
    target_include_directories(${example} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    add_dependencies(${example} copy_html_files)
endforeach()

add_custom_target(copy_html_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/html
        ${CMAKE_CURRENT_BINARY_DIR}/html
    COMMENT "Copying HTML files to build directory"
)