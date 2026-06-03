set(EXAMPLES
    01_basic_parsing
    02_dom_manipulation
    03_css_selectors
    04_advanced_features
)

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.cpp)
    target_link_libraries(${example} PRIVATE hps::hps)
    hps_apply_target_defaults(${example})
endforeach()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/html")
    add_custom_target(
        copy_html_files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/html"
                "${CMAKE_CURRENT_BINARY_DIR}/html"
        COMMENT "Copying example HTML files to the build directory"
    )

    foreach(example ${EXAMPLES})
        add_dependencies(${example} copy_html_files)
    endforeach()
endif()
