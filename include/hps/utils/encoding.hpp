#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace hps {

/**
 * @file encoding.hpp
 * @brief 面向爬虫的字符编码检测与转码（仅依赖系统原生 codec，零第三方依赖）
 *
 * 设计要点：
 * - 解析入口约定：`parse` / `parse_fragment` 假定输入已是 UTF-8；当输入是
 *   未知编码的原始字节（HTTP 响应体、磁盘文件）时使用 `parse_bytes` /
 *   `parse_file`，二者内部会调用本模块完成检测与转码。
 * - 转码后端：Windows 使用 `MultiByteToWideChar`，POSIX 使用 `iconv`，
 *   不引入 ICU 等第三方库；无法用系统 codec 覆盖的编码会如实报告为不支持，
 *   由调用方自行转成 UTF-8 后再交给解析器。
 * - 显式流程（推荐，掌控力最强）：
 *   @code
 *   const auto r = hps::decode_to_utf8(bytes);          // 自动检测 + 转码
 *   if (r.ok()) {
 *       auto doc = hps::parse(r.text);                  // r.encoding 记录了实际编码
 *   } else if (r.status == hps::DecodeStatus::UnknownEncoding) {
 *       // 无声明且非合法 UTF-8：可自行回退或借助第三方库
 *   }
 *   @endcode
 */

/**
 * @brief 编码判定的来源（同时反映优先级：BOM > meta > UTF-8 启发式）
 */
enum class EncodingSource : std::uint8_t {
    Unknown,       ///< 无法判定
    UserOverride,  ///< 调用方显式指定（如 HTTP Content-Type / Options::encoding）
    ByteOrderMark, ///< 由 BOM 判定（UTF-8 / UTF-16LE / UTF-16BE）
    MetaCharset,   ///< 由文档头部 `<meta charset>` 预扫描判定
    Utf8Heuristic, ///< 无显式声明，但字节序列是合法 UTF-8
};

/**
 * @brief 编码检测结果（仅检测，不执行转码）
 */
struct EncodingDetection {
    std::string    label;             ///< 规范化编码标签（如 "utf-8"、"gbk"）；Unknown 时为空
    EncodingSource source = EncodingSource::Unknown;
    bool           supported = false; ///< 本库能否将该编码转码为 UTF-8
    std::size_t    bom_length = 0;    ///< 头部 BOM 字节数（无 BOM 为 0）

    /// @return 是否成功判定出编码
    [[nodiscard]] bool found() const noexcept {
        return source != EncodingSource::Unknown;
    }
};

/**
 * @brief 转码到 UTF-8 的结果状态
 */
enum class DecodeStatus : std::uint8_t {
    Ok,                  ///< 成功产出 UTF-8（可能与输入相同）
    UnknownEncoding,     ///< 无法判定输入编码（无 BOM/声明且非合法 UTF-8）
    UnsupportedEncoding, ///< 判定出了编码，但本库不支持其转码
    InvalidBytes,        ///< 字节序列对于该编码非法，无法转码
};

/**
 * @brief 转码到 UTF-8 的完整结果
 */
struct DecodeResult {
    std::string    text;              ///< UTF-8 文本（仅当 status==Ok 时有效）
    std::string    encoding;          ///< 实际使用的规范化编码标签
    EncodingSource source = EncodingSource::Unknown;
    DecodeStatus   status = DecodeStatus::UnknownEncoding;

    /// @return 是否成功转码
    [[nodiscard]] bool ok() const noexcept {
        return status == DecodeStatus::Ok;
    }
};

/**
 * @brief 检测原始字节的字符编码（不转码）
 *
 * 判定优先级：BOM → 头部 `<meta charset>` 预扫描 → UTF-8 启发式校验。
 * 三者均未命中时返回 `EncodingSource::Unknown`。
 *
 * @param raw_bytes 原始字节
 * @return 检测结果；`label` 为规范化标签，`supported` 表示本库能否转码
 */
[[nodiscard]] EncodingDetection detect_encoding(std::string_view raw_bytes);

/**
 * @brief 将原始字节转码为 UTF-8（检测 + 转码，一步到位）
 *
 * - 当 `override_label` 非空时视为权威编码（如来自 HTTP 头），直接据此转码，
 *   不再进行嗅探；
 * - 当 `override_label` 为空时按 `detect_encoding` 的优先级自动判定后转码；
 * - 输入已是 UTF-8 时仅校验并去除可能存在的 BOM；
 * - 空输入返回成功且文本为空。
 *
 * @param raw_bytes 原始字节
 * @param override_label 可选的强制编码标签（支持别名与大小写，如 "GB2312"）
 * @return 转码结果；失败时 `status` 指明具体原因，`encoding`/`source` 仍尽量填充
 */
[[nodiscard]] DecodeResult decode_to_utf8(
    std::string_view raw_bytes,
    std::string_view override_label = {});

/**
 * @brief 判断某编码标签是否被本库支持转码（别名/大小写不敏感）
 * @param label 编码标签（如 "gbk"、"GB2312"、"iso-8859-1"）
 * @return 支持返回 true
 */
[[nodiscard]] bool is_encoding_supported(std::string_view label);

/**
 * @brief 列出本库支持转码的全部规范化编码标签
 * @return 规范化标签的只读视图（如 utf-8/utf-16le/utf-16be/gbk/big5/shift_jis/windows-1252）
 */
[[nodiscard]] std::span<const std::string_view> supported_encodings();

}  // namespace hps
