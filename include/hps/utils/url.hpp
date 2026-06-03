#pragma once

#include <string>
#include <string_view>

namespace hps {

/**
 * @file url.hpp
 * @brief URL 相对→绝对解析（RFC 3986 §5 参考解析）
 *
 * 爬虫从页面取到的 href/src 多为相对地址，需基于页面 URL 解析为绝对地址。
 * 本模块实现 RFC 3986 的引用解析算法（等价于 Python urljoin、Go ResolveReference、
 * JS `new URL(ref, base)`），不依赖第三方库。
 *
 * 通常无需直接调用：通过 Options::base_url 提供页面 URL 后，
 * Document::base_url() / Document::resolve_url() 以及 get_all_links()/get_all_images()
 * 会自动完成解析（同时会考虑文档中的 `<base href>`）。
 */

/**
 * @brief 将 reference 基于 base 解析为绝对 URL（RFC 3986 §5）。
 *
 * - reference 已是绝对 URL（含 scheme）时按原样规范化返回；
 * - 协议相对（`//host/path`）、绝对路径（`/a/b`）、相对路径（`a`、`../a`、`./a`）、
 *   仅查询（`?q`）、仅片段（`#f`）、空引用均按规范处理；
 * - base 为空时无法解析，原样返回 reference（best-effort，便于"未提供 base 即保持原值"）。
 *
 * @param base 基准 URL（通常为页面地址）
 * @param reference 待解析的引用（可为相对或绝对）
 * @return 解析后的 URL 字符串
 */
[[nodiscard]] std::string resolve_url(std::string_view base, std::string_view reference);

/**
 * @brief 判断 url 是否为绝对 URL（即是否带 scheme，如 `http:`、`mailto:`、`data:`）。
 * @param url 待判断的 URL
 * @return 含合法 scheme 返回 true
 */
[[nodiscard]] bool is_absolute_url(std::string_view url);

}  // namespace hps
