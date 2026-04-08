if(POLICY CMP0168)
  cmake_policy(SET CMP0168 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://gitee.com/mirrors/googletest.git
  GIT_TAG v1.17.0)

set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_GOOGLETEST OR
   FETCHCONTENT_SOURCE_DIR_GOOGLETEST STREQUAL "")
  foreach(_hps_googletest_seed_dir
      "${PROJECT_SOURCE_DIR}/build_review/_deps/googletest-src"
      "${PROJECT_SOURCE_DIR}/build_tidy/_deps/googletest-src")
    if(EXISTS "${_hps_googletest_seed_dir}/CMakeLists.txt")
      set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST
          "${_hps_googletest_seed_dir}"
          CACHE PATH "Pre-seeded googletest source directory" FORCE)
      break()
    endif()
  endforeach()
endif()

set(_hps_googletest_source_dir "${FETCHCONTENT_BASE_DIR}/googletest-src")
set(_hps_googletest_build_dir "${FETCHCONTENT_BASE_DIR}/googletest-build")
set(_hps_googletest_tmp_dir "${FETCHCONTENT_BASE_DIR}/googletest-tmp")
if(EXISTS "${_hps_googletest_source_dir}" AND
   NOT EXISTS "${_hps_googletest_source_dir}/CMakeLists.txt")
  file(REMOVE_RECURSE
    "${_hps_googletest_source_dir}"
    "${_hps_googletest_build_dir}"
    "${_hps_googletest_tmp_dir}")
endif()
FetchContent_MakeAvailable(googletest)

enable_testing()

if(TARGET GTest::gtest_main)
  set(HPS_GTEST_MAIN_TARGET GTest::gtest_main)
elseif(TARGET gtest_main)
  set(HPS_GTEST_MAIN_TARGET gtest_main)
else()
  message(FATAL_ERROR "googletest was populated, but neither GTest::gtest_main nor gtest_main is available")
endif()

function(add_hps_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE $<IF:$<TARGET_EXISTS:hps_static>,hps_static,hps> ${HPS_GTEST_MAIN_TARGET})
  target_compile_definitions(${name} PRIVATE HPS_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
  hps_enable_clang_tidy(${name})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

# Core tests
add_hps_test(core_attribute_tests core/attribute_test.cpp)
add_hps_test(core_node_tests core/node_test.cpp)
add_hps_test(core_element_tests core/element_test.cpp)
add_hps_test(core_document_tests core/document_test.cpp)
add_hps_test(core_document_resource_tests core/document_resource_test.cpp)
add_hps_test(core_text_node_tests core/text_node_test.cpp)
add_hps_test(core_comment_node_tests core/comment_node_test.cpp)

# Parsing tests
add_hps_test(parsing_entity_tests parsing/entity_test.cpp)
add_hps_test(parsing_html_parser_tests parsing/html_parser_test.cpp)
add_hps_test(parsing_html5lib_tokenizer_baseline_tests parsing/html5lib_tokenizer_baseline_test.cpp)
add_hps_test(parsing_html5lib_tree_construction_baseline_tests parsing/html5lib_tree_construction_baseline_test.cpp)
add_hps_test(parsing_tokenizer_tests parsing/tokenizer_test.cpp)
add_hps_test(parsing_tokenizer_states_tests parsing/tokenizer_states_test.cpp)
add_hps_test(parsing_options_tests parsing/options_test.cpp)
add_hps_test(parsing_token_tests parsing/token_test.cpp)
add_hps_test(parsing_token_attribute_tests parsing/token_attribute_test.cpp)
add_hps_test(parsing_token_builder_tests parsing/token_builder_test.cpp)
add_hps_test(parsing_tree_builder_tests parsing/tree_builder_test.cpp)

# Query tests
add_hps_test(query_element_query_tests query/element_query_test.cpp)
add_hps_test(query_element_query_advanced_tests query/element_query_advanced_test.cpp)
add_hps_test(query_query_tests query/query_test.cpp)
add_hps_test(query_nth_of_type_tests query/nth_of_type_test.cpp)
add_hps_test(query_advanced_pseudo_tests query/advanced_pseudo_test.cpp)

# Query CSS tests
add_hps_test(query_css_lexer_tests query/css/css_lexer_test.cpp)
add_hps_test(query_css_parser_tests query/css/css_parser_test.cpp)
add_hps_test(query_css_matcher_tests query/css/css_matcher_test.cpp)
add_hps_test(query_css_selector_tests query/css/css_selector_test.cpp)
add_hps_test(query_css_utils_tests query/css/css_utils_test.cpp)

# Utils tests
add_hps_test(utils_encoding_tests utils/encoding_test.cpp)
add_hps_test(utils_exception_tests utils/exception_test.cpp)
add_hps_test(utils_string_pool_tests utils/string_pool_test.cpp)
add_hps_test(utils_string_utils_tests utils/string_utils_test.cpp)

# Public API tests
add_hps_test(hps_api_tests hps/hps_api_test.cpp)
