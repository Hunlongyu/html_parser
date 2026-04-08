#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace hps {

enum class EncodingHintSource : std::uint8_t {
    None,
    Utf8Bom,
    Utf16LeBom,
    Utf16BeBom,
    MetaCharset,
    Utf8Heuristic,
};

struct EncodingHint {
    std::string        detected_label;
    std::string        canonical_label;
    EncodingHintSource source = EncodingHintSource::None;
    bool               helper_supported = false;

    [[nodiscard]] bool has_encoding() const noexcept {
        return source != EncodingHintSource::None;
    }
};

[[nodiscard]] EncodingHint sniff_html_encoding(std::string_view raw_bytes);

[[nodiscard]] std::optional<std::string> decode_html_bytes_to_utf8(
    std::string_view raw_bytes,
    std::string_view encoding_label);

}  // namespace hps
