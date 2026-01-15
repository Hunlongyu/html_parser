include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
  GIT_TAG v1.17.0)

set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

function(add_hps_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE $<IF:$<TARGET_EXISTS:hps_static>,hps_static,hps> GTest::gtest_main)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_hps_test(core_attribute_tests core/attribute_test.cpp)
add_hps_test(core_node_tests core/node_test.cpp)
add_hps_test(core_element_tests core/element_test.cpp)
add_hps_test(core_document_tests core/document_test.cpp)
add_hps_test(core_text_node_tests core/text_node_test.cpp)
add_hps_test(core_comment_node_tests core/comment_node_test.cpp)
add_hps_test(parsing_entity_tests parsing/entity_test.cpp)
add_hps_test(parsing_html_parser_tests parsing/html_parser_test.cpp)
