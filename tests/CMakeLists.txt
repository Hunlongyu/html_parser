if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
endif()

# 优先使用系统/包管理器提供的 GoogleTest；找不到再用 FetchContent 拉取。
# 拉取源默认走 gitee 镜像，可用 -DHPS_GTEST_GIT_REPOSITORY=... 覆盖（如 CI 用官方源）。
set(HPS_GTEST_GIT_REPOSITORY "https://gitee.com/mirrors/googletest.git"
    CACHE STRING "Git repository used to fetch GoogleTest when not found via find_package")
set(HPS_GTEST_GIT_TAG "v1.17.0"
    CACHE STRING "Git tag used to fetch GoogleTest")

find_package(GTest QUIET)

if(NOT TARGET GTest::gtest_main)
    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "${HPS_GTEST_GIT_REPOSITORY}"
        GIT_TAG "${HPS_GTEST_GIT_TAG}"
    )
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

if(TARGET GTest::gtest_main)
    set(HPS_GTEST_MAIN_TARGET GTest::gtest_main)
elseif(TARGET gtest_main)
    set(HPS_GTEST_MAIN_TARGET gtest_main)
else()
    message(FATAL_ERROR "GoogleTest not found via find_package and could not be fetched")
endif()

function(add_hps_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE hps::hps ${HPS_GTEST_MAIN_TARGET})
    target_compile_definitions(${name} PRIVATE HPS_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    hps_apply_target_defaults(${name})
    gtest_discover_tests(${name})
endfunction()

# Core tests
add_hps_test(core_attribute_tests core/attribute_test.cpp)
add_hps_test(core_node_tests core/node_test.cpp)
add_hps_test(core_element_tests core/element_test.cpp)
add_hps_test(core_document_tests core/document_test.cpp)
add_hps_test(core_document_resource_tests core/document_resource_test.cpp)
add_hps_test(core_text_node_tests core/text_node_test.cpp)
add_hps_test(core_comment_node_tests core/comment_node_test.cpp)
add_hps_test(core_serialization_tests core/serialization_test.cpp)
add_hps_test(core_url_resolution_tests core/url_resolution_test.cpp)

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
add_hps_test(query_nth_child_tests query/nth_child_test.cpp)
add_hps_test(query_text_attr_tests query/text_attr_test.cpp)
add_hps_test(query_advanced_pseudo_tests query/advanced_pseudo_test.cpp)

# Query CSS tests
add_hps_test(query_css_lexer_tests query/css/css_lexer_test.cpp)
add_hps_test(query_css_parser_tests query/css/css_parser_test.cpp)
add_hps_test(query_css_matcher_tests query/css/css_matcher_test.cpp)
add_hps_test(query_css_selector_tests query/css/css_selector_test.cpp)
add_hps_test(query_css_attribute_tests query/css/css_attribute_test.cpp)
add_hps_test(query_css_utils_tests query/css/css_utils_test.cpp)

# Utils tests
add_hps_test(utils_encoding_tests utils/encoding_test.cpp)
add_hps_test(utils_url_tests utils/url_test.cpp)
add_hps_test(utils_exception_tests utils/exception_test.cpp)
add_hps_test(utils_string_pool_tests utils/string_pool_test.cpp)
add_hps_test(utils_string_utils_tests utils/string_utils_test.cpp)

# Public API tests
add_hps_test(hps_api_tests hps/hps_api_test.cpp)

# ==================== 完整 html5lib tree-construction 一致性套件 ====================
# 拉取上游 html5lib-tests，跑完整 tree-construction 套件并“报告”通过率（与 parse5/lexbor 同口径）。
# 默认开启；置 OFF 可跳过网络拉取。源默认 github，可用 -DHPS_HTML5LIB_TESTS_GIT_REPOSITORY=<mirror> 覆盖。
option(HPS_HTML5LIB_CONFORMANCE "Fetch full html5lib-tests and run the tree-construction conformance suite" ON)

if(HPS_HTML5LIB_CONFORMANCE)
    set(HPS_HTML5LIB_TESTS_GIT_REPOSITORY "https://github.com/html5lib/html5lib-tests.git"
        CACHE STRING "Git repository for html5lib-tests data (override for a mirror)")
    # 该提交是上游把 tree-construction 数据迁往 WPT 前的最后一个版本。
    # 固定提交保证 conformance 基线可复现，不随上游 master 布局变化而失效。
    set(_HPS_HTML5LIB_TESTS_PIN "9329e64694e7835d0dcff9811e22856ef6ad16f9")
    if(DEFINED HPS_HTML5LIB_TESTS_GIT_TAG AND HPS_HTML5LIB_TESTS_GIT_TAG STREQUAL "master")
        # 迁移旧版本写入 CMakeCache.txt 的默认值；否则已有构建目录会一直追踪已删除该目录的 master。
        set(HPS_HTML5LIB_TESTS_GIT_TAG "${_HPS_HTML5LIB_TESTS_PIN}"
            CACHE STRING "Git tag/branch for html5lib-tests data" FORCE)
    else()
        set(HPS_HTML5LIB_TESTS_GIT_TAG "${_HPS_HTML5LIB_TESTS_PIN}"
            CACHE STRING "Git tag/branch for html5lib-tests data")
    endif()
    unset(_HPS_HTML5LIB_TESTS_PIN)

    include(FetchContent)
    FetchContent_Declare(
        html5lib_tests
        GIT_REPOSITORY "${HPS_HTML5LIB_TESTS_GIT_REPOSITORY}"
        GIT_TAG "${HPS_HTML5LIB_TESTS_GIT_TAG}"
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(html5lib_tests)

    add_executable(parsing_html5lib_tree_construction_conformance_tests
        parsing/html5lib_tree_construction_conformance_test.cpp)
    target_link_libraries(parsing_html5lib_tree_construction_conformance_tests
        PRIVATE hps::hps ${HPS_GTEST_MAIN_TARGET})
    target_compile_definitions(parsing_html5lib_tree_construction_conformance_tests
        PRIVATE HPS_HTML5LIB_TREE_DIR="${html5lib_tests_SOURCE_DIR}/tree-construction")
    hps_apply_target_defaults(parsing_html5lib_tree_construction_conformance_tests)
    gtest_discover_tests(parsing_html5lib_tree_construction_conformance_tests)
endif()
