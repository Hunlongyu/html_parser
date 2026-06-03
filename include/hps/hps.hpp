#pragma once

#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/hps_fwd.hpp"
#include "hps/parsing/options.hpp"
#include "hps/query/query.hpp"
#include "hps/utils/encoding.hpp"
#include "hps/utils/exception.hpp"
#include "hps/utils/url.hpp"
#include "hps/version.hpp"

namespace hps {

/**
 * @brief 解析结果：文档 + 解析期间收集的错误/警告。
 *
 * 由 `*_with_error` 系列入口返回，用于需要诊断信息的场景。普通入口（如 `parse`）
 * 只返回文档；宽松模式下即使有错误 `document` 也通常非空（已尽力恢复）。
 */
struct ParseResult {
    std::shared_ptr<Document> document;  ///< 解析得到的文档（失败时可能为空文档）
    std::vector<HPSError>     errors;    ///< 解析期间产生的错误/警告，按出现顺序排列

    /// @return 是否存在任何错误
    [[nodiscard]] bool has_errors() const noexcept {
        return !errors.empty();
    }

    /// @return 错误数量
    [[nodiscard]] int error_count() const noexcept {
        return static_cast<int>(errors.size());
    }
};

// 说明：每个入口都用 `const Options& = {}` 提供默认选项，"带/不带选项"为同一函数；
// 每类输入各有「返回文档」与「返回文档+诊断(ParseResult)」两个变体。

// ==================== 解析 UTF-8 文本 ====================

/**
 * @brief 解析一段 UTF-8 HTML 文本为文档。
 * @param html 已是 UTF-8 的 HTML 文本（未知编码的原始字节请用 parse_bytes）
 * @param options 解析选项（默认即可）
 * @return 文档（始终非空）
 */
[[nodiscard]] std::shared_ptr<Document> parse(std::string_view html, const Options& options = {});

/**
 * @brief 解析并返回文档与诊断信息（错误列表）。
 * @param html 已是 UTF-8 的 HTML 文本
 * @param options 解析选项（严格模式下首个错误会抛 HPSException）
 * @return 文档与错误列表
 */
[[nodiscard]] ParseResult parse_with_error(std::string_view html, const Options& options = {});

// ==================== 解析 HTML 片段 ====================

/**
 * @brief 解析 HTML 片段（无需完整文档结构）。
 * @param html 片段文本（UTF-8）
 * @param context_tag 片段所处的上下文标签名（如 "div"、"table"、"tr"），影响解析规则
 * @param options 解析选项
 * @return 仅含片段节点的文档（无 html/head/body 外壳）
 */
[[nodiscard]] std::shared_ptr<Document> parse_fragment(
    std::string_view html,
    std::string_view context_tag,
    const Options& options = {});

/// @brief 解析 HTML 片段并返回诊断信息。 @see parse_fragment
[[nodiscard]] ParseResult parse_fragment_with_error(
    std::string_view html,
    std::string_view context_tag,
    const Options& options = {});

// ==================== 解析未知编码的原始字节 ====================

/**
 * @brief 解析未知编码的原始字节（如 HTTP 响应体）。
 *
 * 与 parse 的区别在于**输入语义**：parse 接收已解码的 UTF-8 文本、不做编码处理；
 * parse_bytes 接收未知编码的原始字节并自动检测/转码——`Options::encoding` 非空则
 * 以其为准，否则按 BOM → `<meta charset>` → UTF-8 启发式 嗅探，再用系统原生 codec
 * 转为 UTF-8 后解析。（二者签名相同，故为两个具名入口而非重载。）
 * @param bytes 原始字节
 * @param options 解析选项（可用 encoding 指定编码）
 * @return 文档（编码无法识别/不支持时为空文档，诊断见 parse_bytes_with_error）
 */
[[nodiscard]] std::shared_ptr<Document> parse_bytes(std::string_view bytes, const Options& options = {});

/// @brief 解析原始字节并返回诊断信息（含 UnsupportedEncoding）。 @see parse_bytes
[[nodiscard]] ParseResult parse_bytes_with_error(std::string_view bytes, const Options& options = {});

// ==================== 解析文件 ====================

/**
 * @brief 读取并解析 HTML 文件。
 *
 * 以二进制读取后，与 parse_bytes 一致地嗅探/转码为 UTF-8 再解析。
 * @param path 文件路径
 * @param options 解析选项
 * @return 文档（读取失败或编码不支持时为空文档）
 */
[[nodiscard]] std::shared_ptr<Document> parse_file(std::string_view path, const Options& options = {});

/// @brief 读取并解析 HTML 文件并返回诊断信息。 @see parse_file
[[nodiscard]] ParseResult parse_file_with_error(std::string_view path, const Options& options = {});

// ==================== 其它 ====================

/// @return 库版本字符串（如 "1.2.3"）
[[nodiscard]] std::string version();

}  // namespace hps
