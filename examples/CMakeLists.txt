cmake_minimum_required(VERSION 3.28)

project(hps_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED HPS_BUILD_SHARED)
    set(HPS_BUILD_SHARED ON)
endif()
if(NOT DEFINED HPS_BUILD_STATIC)
    set(HPS_BUILD_STATIC ON)
endif()

set(EXAMPLES
    01_basic_parsing
    02_dom_manipulation
    03_css_selectors
    04_advanced_features
)

set(HPS_LIB_TO_LINK "")
if(HPS_BUILD_STATIC)
    set(HPS_LIB_TO_LINK hps_static)
elseif(HPS_BUILD_SHARED)
    set(HPS_LIB_TO_LINK hps)
endif()

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.cpp)
    if(HPS_LIB_TO_LINK)
        target_link_libraries(${example} PRIVATE ${HPS_LIB_TO_LINK})
    endif()
    target_include_directories(${example} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    # add_dependencies(${example} copy_html_files) # Uncomment if html files are needed
endforeach()

# Optional: keep copy_html_files if you still want to copy the html folder
add_custom_target(copy_html_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/html
        ${CMAKE_CURRENT_BINARY_DIR}/html
    COMMENT "Copying HTML files to build directory"
)
