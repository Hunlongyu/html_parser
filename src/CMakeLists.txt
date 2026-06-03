set(hps_sources
    core/comment_node.cpp
    core/document.cpp
    core/element.cpp
    core/node.cpp
    core/serializer.cpp
    core/text_node.cpp
    parsing/html_parser.cpp
    parsing/token.cpp
    parsing/tokenizer.cpp
    parsing/tree_builder.cpp
    query/css/css_lexer.cpp
    query/css/css_matcher.cpp
    query/css/css_parser.cpp
    query/css/css_selector.cpp
    query/element_query.cpp
    query/query.cpp
    utils/encoding.cpp
    utils/url.cpp
    hps.cpp
)

set(hps_public_headers
    "${PROJECT_SOURCE_DIR}/include/hps/core/attribute.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/core/comment_node.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/core/document.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/core/element.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/core/node.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/core/text_node.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/hps.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/hps_fwd.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/html_parser.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/options.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/token.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/token_attribute.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/token_builder.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/tokenizer.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/parsing/tree_builder.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/css/css_lexer.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/css/css_matcher.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/css/css_parser.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/css/css_selector.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/css/css_utils.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/element_query.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/query/query.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/encoding.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/exception.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/noncopyable.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/string_pool.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/string_utils.hpp"
    "${PROJECT_SOURCE_DIR}/include/hps/utils/url.hpp"
)

set(hps_generated_include_dir "${PROJECT_BINARY_DIR}/generated/include")
set(hps_generated_version_header "${hps_generated_include_dir}/hps/version.hpp")
set(hps_version_template "${PROJECT_SOURCE_DIR}/include/hps/version.hpp.in")

file(MAKE_DIRECTORY "${hps_generated_include_dir}/hps")
configure_file("${hps_version_template}" "${hps_generated_version_header}" @ONLY)

add_library(hps)
add_library(hps::hps ALIAS hps)

target_sources(
    hps
    PRIVATE ${hps_sources}
    PUBLIC
        FILE_SET public_headers TYPE HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
        FILES ${hps_public_headers}
        FILE_SET generated_headers TYPE HEADERS
        BASE_DIRS "${hps_generated_include_dir}"
        FILES "${hps_generated_version_header}"
)

target_include_directories(
    hps
    PUBLIC
        $<BUILD_INTERFACE:${hps_generated_include_dir}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(
    hps
    PROPERTIES
        OUTPUT_NAME hps
        EXPORT_NAME hps
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

if(WIN32 AND BUILD_SHARED_LIBS)
    set_target_properties(hps PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# 编码转码（src/utils/encoding.cpp）的系统原生依赖：
# - Windows：使用 Win32 的 MultiByteToWideChar/WideCharToMultiByte，无需额外链接。
# - POSIX：使用 iconv。glibc 将其内置于 libc（无需链接）；macOS/BSD/musl 需要单独链接 libiconv。
if(NOT WIN32)
    find_library(HPS_ICONV_LIBRARY NAMES iconv libiconv)
    if(HPS_ICONV_LIBRARY)
        target_link_libraries(hps PRIVATE "${HPS_ICONV_LIBRARY}")
    endif()
endif()

hps_apply_target_defaults(hps)

source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${hps_sources})
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${hps_public_headers})