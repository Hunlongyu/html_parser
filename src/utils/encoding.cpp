#include "hps/utils/encoding.hpp"

#include "hps/utils/string_utils.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <iconv.h>
#endif

namespace hps {
namespace {

// 预扫描 <meta charset> 的字节窗口（覆盖绝大多数真实页面的头部声明）。
constexpr std::size_t meta_prescan_bytes = 4096;

struct CharsetAliasMapping {
    std::string_view alias;
    std::string_view canonical;
};

struct CharsetBackendMapping {
    std::string_view canonical;
    unsigned int     windows_code_page;
    std::string_view iconv_name;
};

constexpr std::array<CharsetAliasMapping, 28> supported_charset_aliases = {{
    {"utf-8", "utf-8"},
    {"utf8", "utf-8"},
    {"unicode-1-1-utf-8", "utf-8"},
    {"utf-16", "utf-16le"},
    {"utf-16le", "utf-16le"},
    {"utf-16be", "utf-16be"},
    {"gbk", "gbk"},
    {"gb2312", "gbk"},
    {"gb-2312", "gbk"},
    {"gb_2312", "gbk"},
    {"gb18030", "gbk"},
    {"gb-18030", "gbk"},
    {"gb_18030", "gbk"},
    {"cp936", "gbk"},
    {"ms936", "gbk"},
    {"x-gbk", "gbk"},
    {"windows-1252", "windows-1252"},
    {"cp1252", "windows-1252"},
    {"x-cp1252", "windows-1252"},
    {"iso-8859-1", "windows-1252"},
    {"iso8859-1", "windows-1252"},
    {"iso_8859-1", "windows-1252"},
    {"latin1", "windows-1252"},
    {"l1", "windows-1252"},
    {"shift_jis", "shift_jis"},
    {"shift-jis", "shift_jis"},
    {"sjis", "shift_jis"},
    {"cp932", "shift_jis"},
}};

constexpr std::array<CharsetAliasMapping, 8> supported_charset_aliases_tail = {{
    {"ms932", "shift_jis"},
    {"windows-31j", "shift_jis"},
    {"x-sjis", "shift_jis"},
    {"csshiftjis", "shift_jis"},
    {"big5", "big5"},
    {"big5-hkscs", "big5"},
    {"cn-big5", "big5"},
    {"csbig5", "big5"},
}};

constexpr std::array<CharsetBackendMapping, 6> supported_charset_backends = {{
    {"gbk", 936U, "GBK"},
    {"windows-1252", 1252U, "WINDOWS-1252"},
    {"shift_jis", 932U, "SHIFT_JIS"},
    {"big5", 950U, "BIG5"},
    {"utf-16le", 1200U, "UTF-16LE"},
    {"utf-16be", 1201U, "UTF-16BE"},
}};

[[nodiscard]] auto has_utf8_bom(const std::string_view input) noexcept -> bool {
    return input.size() >= 3 &&
           static_cast<unsigned char>(input[0]) == 0xEF &&
           static_cast<unsigned char>(input[1]) == 0xBB &&
           static_cast<unsigned char>(input[2]) == 0xBF;
}

[[nodiscard]] auto has_utf16le_bom(const std::string_view input) noexcept -> bool {
    return input.size() >= 2 &&
           static_cast<unsigned char>(input[0]) == 0xFF &&
           static_cast<unsigned char>(input[1]) == 0xFE;
}

[[nodiscard]] auto has_utf16be_bom(const std::string_view input) noexcept -> bool {
    return input.size() >= 2 &&
           static_cast<unsigned char>(input[0]) == 0xFE &&
           static_cast<unsigned char>(input[1]) == 0xFF;
}

void append_utf8(std::string& output, const char32_t code_point) {
    if (code_point <= 0x7F) {
        output.push_back(static_cast<char>(code_point));
        return;
    }
    if (code_point <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        return;
    }
    if (code_point <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
        return;
    }
    output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
}

[[nodiscard]] auto decode_utf16_to_utf8(
    const std::string_view bytes,
    const bool big_endian) -> std::string {
    std::string utf8;
    utf8.reserve(bytes.size());

    size_t index = 0;
    while (index + 1 < bytes.size()) {
        const auto read_unit = [&](const size_t offset) -> uint16_t {
            const auto first  = static_cast<unsigned char>(bytes[offset]);
            const auto second = static_cast<unsigned char>(bytes[offset + 1]);
            if (big_endian) {
                return static_cast<uint16_t>((first << 8) | second);
            }
            return static_cast<uint16_t>((second << 8) | first);
        };

        const uint16_t lead = read_unit(index);
        index += 2;

        if (lead >= 0xD800 && lead <= 0xDBFF) {
            if (index + 1 >= bytes.size()) {
                append_utf8(utf8, 0xFFFD);
                break;
            }

            const uint16_t trail = read_unit(index);
            if (trail >= 0xDC00 && trail <= 0xDFFF) {
                index += 2;
                const auto code_point =
                    static_cast<char32_t>(0x10000 + (((lead - 0xD800) << 10) | (trail - 0xDC00)));
                append_utf8(utf8, code_point);
                continue;
            }

            append_utf8(utf8, 0xFFFD);
            continue;
        }

        if (lead >= 0xDC00 && lead <= 0xDFFF) {
            append_utf8(utf8, 0xFFFD);
            continue;
        }

        append_utf8(utf8, lead);
    }

    if (index < bytes.size()) {
        append_utf8(utf8, 0xFFFD);
    }

    return utf8;
}

[[nodiscard]] auto is_valid_utf8(const std::string_view input) noexcept -> bool {
    size_t index = 0;
    while (index < input.size()) {
        const auto byte = static_cast<unsigned char>(input[index]);
        if (byte <= 0x7F) {
            ++index;
            continue;
        }

        if (byte >= 0xC2 && byte <= 0xDF) {
            if (index + 1 >= input.size()) {
                return false;
            }
            const auto b1 = static_cast<unsigned char>(input[index + 1]);
            if ((b1 & 0xC0) != 0x80) {
                return false;
            }
            index += 2;
            continue;
        }

        if (byte >= 0xE0 && byte <= 0xEF) {
            if (index + 2 >= input.size()) {
                return false;
            }
            const auto b1 = static_cast<unsigned char>(input[index + 1]);
            const auto b2 = static_cast<unsigned char>(input[index + 2]);
            if ((b2 & 0xC0) != 0x80) {
                return false;
            }
            if (byte == 0xE0) {
                if (b1 < 0xA0 || b1 > 0xBF) {
                    return false;
                }
            } else if (byte == 0xED) {
                if (b1 < 0x80 || b1 > 0x9F) {
                    return false;
                }
            } else if ((b1 & 0xC0) != 0x80) {
                return false;
            }
            index += 3;
            continue;
        }

        if (byte >= 0xF0 && byte <= 0xF4) {
            if (index + 3 >= input.size()) {
                return false;
            }
            const auto b1 = static_cast<unsigned char>(input[index + 1]);
            const auto b2 = static_cast<unsigned char>(input[index + 2]);
            const auto b3 = static_cast<unsigned char>(input[index + 3]);
            if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
                return false;
            }
            if (byte == 0xF0) {
                if (b1 < 0x90 || b1 > 0xBF) {
                    return false;
                }
            } else if (byte == 0xF4) {
                if (b1 < 0x80 || b1 > 0x8F) {
                    return false;
                }
            } else if ((b1 & 0xC0) != 0x80) {
                return false;
            }
            index += 4;
            continue;
        }

        return false;
    }

    return true;
}

template <typename Range>
[[nodiscard]] auto lookup_alias(
    const Range& aliases,
    const std::string_view normalized) -> std::optional<std::string_view> {
    const auto it = std::ranges::find_if(aliases, [&](const CharsetAliasMapping& mapping) {
        return mapping.alias == normalized;
    });
    if (it == aliases.end()) {
        return std::nullopt;
    }
    return it->canonical;
}

[[nodiscard]] auto normalize_encoding_label(std::string_view label) -> std::string {
    std::string normalized;
    normalized.reserve(label.size());
    for (const char ch : trim_whitespace(label)) {
        if (is_alnum(ch) || ch == '-' || ch == '_') {
            normalized.push_back(to_lower(ch));
        } else {
            break;
        }
    }

    if (const auto alias = lookup_alias(supported_charset_aliases, normalized); alias.has_value()) {
        return std::string(*alias);
    }
    if (const auto alias = lookup_alias(supported_charset_aliases_tail, normalized); alias.has_value()) {
        return std::string(*alias);
    }
    return normalized;
}

template <typename Range>
[[nodiscard]] auto lookup_backend(
    const Range& backends,
    const std::string_view canonical) -> std::optional<CharsetBackendMapping> {
    const auto it = std::ranges::find_if(backends, [&](const CharsetBackendMapping& mapping) {
        return mapping.canonical == canonical;
    });
    if (it == backends.end()) {
        return std::nullopt;
    }
    return *it;
}

[[nodiscard]] auto backend_mapping_for_charset(
    const std::string_view canonical) -> std::optional<CharsetBackendMapping> {
    return lookup_backend(supported_charset_backends, canonical);
}

[[nodiscard]] auto is_helper_supported_encoding(
    const std::string_view canonical) -> bool {
    return canonical == "utf-8" || backend_mapping_for_charset(canonical).has_value();
}

[[nodiscard]] auto extract_charset_from_meta_tag(const std::string_view lower_tag)
    -> std::string {
    size_t cursor = 0;
    while (cursor < lower_tag.size()) {
        const size_t charset_pos = lower_tag.find("charset", cursor);
        if (charset_pos == std::string_view::npos) {
            break;
        }

        size_t value_pos = charset_pos + 7;
        while (value_pos < lower_tag.size() && is_whitespace(lower_tag[value_pos])) {
            ++value_pos;
        }
        if (value_pos >= lower_tag.size() || lower_tag[value_pos] != '=') {
            cursor = charset_pos + 7;
            continue;
        }

        ++value_pos;
        while (value_pos < lower_tag.size() && is_whitespace(lower_tag[value_pos])) {
            ++value_pos;
        }
        if (value_pos >= lower_tag.size()) {
            break;
        }

        char quote = '\0';
        if (lower_tag[value_pos] == '"' || lower_tag[value_pos] == '\'') {
            quote = lower_tag[value_pos];
            ++value_pos;
        }

        const size_t start = value_pos;
        while (value_pos < lower_tag.size()) {
            const char ch = lower_tag[value_pos];
            if ((quote != '\0' && ch == quote) ||
                (quote == '\0' && (is_whitespace(ch) || ch == ';' || ch == '/' || ch == '>'))) {
                break;
            }
            ++value_pos;
        }

        if (value_pos > start) {
            return std::string(lower_tag.substr(start, value_pos - start));
        }
        cursor = charset_pos + 7;
    }

    return {};
}

// 预扫描文档头部的 <meta charset>，返回规范化编码标签（未找到则为空）。
[[nodiscard]] auto sniff_meta_charset(const std::string_view raw_html) -> std::string {
    const auto prefix = raw_html.substr(0, std::min<size_t>(raw_html.size(), meta_prescan_bytes));

    std::string lowered(prefix);
    std::ranges::transform(lowered, lowered.begin(), [](const unsigned char ch) {
        return static_cast<char>(ch < 0x80 ? to_lower(static_cast<char>(ch)) : ch);
    });

    size_t cursor = 0;
    while (cursor < lowered.size()) {
        const size_t meta_pos = lowered.find("<meta", cursor);
        if (meta_pos == std::string_view::npos) {
            break;
        }

        size_t end_pos = lowered.find('>', meta_pos);
        if (end_pos == std::string_view::npos) {
            end_pos = lowered.size();
        }

        const std::string detected_label = extract_charset_from_meta_tag(
            std::string_view(lowered).substr(meta_pos, end_pos - meta_pos));
        if (!detected_label.empty()) {
            return normalize_encoding_label(detected_label);
        }

        cursor = end_pos + 1;
    }

    return {};
}

[[nodiscard]] auto decode_code_page_to_utf8(
    const std::string_view bytes,
    const CharsetBackendMapping& mapping) -> std::string {
    if (mapping.canonical == "utf-16le") {
        return decode_utf16_to_utf8(bytes, false);
    }
    if (mapping.canonical == "utf-16be") {
        return decode_utf16_to_utf8(bytes, true);
    }

#ifdef _WIN32
    const int wide_size = MultiByteToWideChar(
        mapping.windows_code_page,
        0,
        bytes.data(),
        static_cast<int>(bytes.size()),
        nullptr,
        0);
    if (wide_size <= 0) {
        throw std::runtime_error("Failed to calculate UTF-16 size for charset " + std::string(mapping.canonical));
    }

    std::vector<wchar_t> wide(wide_size);
    if (MultiByteToWideChar(
            mapping.windows_code_page,
            0,
            bytes.data(),
            static_cast<int>(bytes.size()),
            wide.data(),
            wide_size) != wide_size) {
        throw std::runtime_error("Failed to convert bytes from charset " + std::string(mapping.canonical));
    }

    const int utf8_size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        wide_size,
        nullptr,
        0,
        nullptr,
        nullptr);
    if (utf8_size <= 0) {
        throw std::runtime_error("Failed to calculate UTF-8 size for charset " + std::string(mapping.canonical));
    }

    std::string utf8(utf8_size, '\0');
    if (WideCharToMultiByte(
            CP_UTF8,
            0,
            wide.data(),
            wide_size,
            utf8.data(),
            utf8_size,
            nullptr,
            nullptr) != utf8_size) {
        throw std::runtime_error("Failed to convert UTF-16 to UTF-8 for charset " + std::string(mapping.canonical));
    }
    return utf8;
#else
    std::string iconv_name(mapping.iconv_name);
    iconv_t     cd = iconv_open("UTF-8", iconv_name.c_str());
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw std::runtime_error("Failed to open iconv for charset " + std::string(mapping.canonical));
    }

    size_t            in_bytes_left  = bytes.size();
    size_t            out_bytes_left = std::max<size_t>(bytes.size() * 4, 32);
    std::vector<char> out_buffer(out_bytes_left);
    char*             in_ptr  = const_cast<char*>(bytes.data());
    char*             out_ptr = out_buffer.data();
    const size_t      result  = iconv(cd, &in_ptr, &in_bytes_left, &out_ptr, &out_bytes_left);
    iconv_close(cd);
    if (result == static_cast<size_t>(-1)) {
        throw std::runtime_error("Failed to convert bytes from charset " + std::string(mapping.canonical));
    }

    return std::string(out_buffer.data(), out_buffer.size() - out_bytes_left);
#endif
}

// 按给定的“已规范化”编码标签将字节转码为 UTF-8。
[[nodiscard]] auto decode_as_canonical(
    const std::string_view raw_bytes,
    const std::string_view canonical,
    const EncodingSource source) -> DecodeResult {
    DecodeResult result;
    result.encoding = std::string(canonical);
    result.source   = source;

    if (canonical == "utf-8") {
        std::string_view bytes = raw_bytes;
        if (has_utf8_bom(bytes)) {
            bytes.remove_prefix(3);
        }
        if (!is_valid_utf8(bytes)) {
            result.status = DecodeStatus::InvalidBytes;
            return result;
        }
        result.text   = std::string(bytes);
        result.status = DecodeStatus::Ok;
        return result;
    }

    const auto mapping = backend_mapping_for_charset(canonical);
    if (!mapping.has_value()) {
        result.status = DecodeStatus::UnsupportedEncoding;
        return result;
    }

    std::string_view bytes = raw_bytes;
    const bool has_matching_utf16_bom =
        (canonical == "utf-16le" && has_utf16le_bom(bytes)) ||
        (canonical == "utf-16be" && has_utf16be_bom(bytes));
    if (has_matching_utf16_bom) {
        bytes.remove_prefix(2);
    }

    try {
        result.text   = decode_code_page_to_utf8(bytes, *mapping);
        result.status = DecodeStatus::Ok;
    } catch (const std::exception&) {
        result.status = DecodeStatus::InvalidBytes;
    }
    return result;
}

}  // namespace

EncodingDetection detect_encoding(const std::string_view raw_bytes) {
    EncodingDetection detection;
    if (raw_bytes.empty()) {
        return detection;  // Unknown
    }

    if (has_utf8_bom(raw_bytes)) {
        detection.label      = "utf-8";
        detection.source     = EncodingSource::ByteOrderMark;
        detection.supported  = true;
        detection.bom_length = 3;
        return detection;
    }
    if (has_utf16le_bom(raw_bytes)) {
        detection.label      = "utf-16le";
        detection.source     = EncodingSource::ByteOrderMark;
        detection.supported  = true;
        detection.bom_length = 2;
        return detection;
    }
    if (has_utf16be_bom(raw_bytes)) {
        detection.label      = "utf-16be";
        detection.source     = EncodingSource::ByteOrderMark;
        detection.supported  = true;
        detection.bom_length = 2;
        return detection;
    }

    if (auto meta_label = sniff_meta_charset(raw_bytes); !meta_label.empty()) {
        detection.label     = meta_label;
        detection.source    = EncodingSource::MetaCharset;
        detection.supported = is_helper_supported_encoding(meta_label);
        return detection;
    }

    if (is_valid_utf8(raw_bytes)) {
        detection.label    = "utf-8";
        detection.source   = EncodingSource::Utf8Heuristic;
        detection.supported = true;
        return detection;
    }

    return detection;  // Unknown
}

DecodeResult decode_to_utf8(
    const std::string_view raw_bytes,
    const std::string_view override_label) {
    // 空输入：视为成功的空 UTF-8。
    if (raw_bytes.empty()) {
        DecodeResult result;
        result.status = DecodeStatus::Ok;
        if (override_label.empty()) {
            result.encoding = "utf-8";
            result.source   = EncodingSource::Utf8Heuristic;
        } else {
            result.encoding = normalize_encoding_label(override_label);
            result.source   = EncodingSource::UserOverride;
        }
        return result;
    }

    // 显式编码：视为权威，直接据此转码（不再嗅探）。
    if (!override_label.empty()) {
        const std::string canonical = normalize_encoding_label(override_label);
        if (canonical.empty()) {
            DecodeResult result;
            result.status = DecodeStatus::UnsupportedEncoding;
            return result;
        }
        return decode_as_canonical(raw_bytes, canonical, EncodingSource::UserOverride);
    }

    // 自动检测后转码。
    const auto detection = detect_encoding(raw_bytes);
    if (!detection.found()) {
        DecodeResult result;
        result.status = DecodeStatus::UnknownEncoding;
        return result;
    }
    return decode_as_canonical(raw_bytes, detection.label, detection.source);
}

bool is_encoding_supported(const std::string_view label) {
    return is_helper_supported_encoding(normalize_encoding_label(label));
}

std::span<const std::string_view> supported_encodings() {
    static constexpr std::array<std::string_view, 7> supported_encoding_labels = {
        "utf-8", "utf-16le", "utf-16be", "gbk", "big5", "shift_jis", "windows-1252",
    };
    return supported_encoding_labels;
}

}  // namespace hps
