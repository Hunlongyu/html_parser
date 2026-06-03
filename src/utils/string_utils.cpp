#include "hps/utils/string_utils.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace hps {

std::string decode_html_entities(const std::string& text) {
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 26> named_entities = {{
        {"amp", "&"},
        {"lt", "<"},
        {"gt", ">"},
        {"quot", "\""},
        {"apos", "'"},
        {"nbsp", " "},
        {"copy", "\xC2\xA9"},
        {"reg", "\xC2\xAE"},
        {"trade", "\xE2\x84\xA2"},
        {"mdash", "\xE2\x80\x94"},
        {"ndash", "\xE2\x80\x93"},
        {"hellip", "\xE2\x80\xA6"},
        {"middot", "\xC2\xB7"},
        {"bull", "\xE2\x80\xA2"},
        {"euro", "\xE2\x82\xAC"},
        {"laquo", "\xC2\xAB"},
        {"raquo", "\xC2\xBB"},
        {"iexcl", "\xC2\xA1"},
        {"cent", "\xC2\xA2"},
        {"pound", "\xC2\xA3"},
        {"yen", "\xC2\xA5"},
        {"sect", "\xC2\xA7"},
        {"para", "\xC2\xB6"},
        {"deg", "\xC2\xB0"},
        {"times", "\xC3\x97"},
        {"divide", "\xC3\xB7"},
    }};

    const auto append_code_point_utf8 = [](std::string& output, const uint32_t code_point) {
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
    };

    const auto parse_numeric_entity = [](const std::string_view entity_body) -> std::optional<uint32_t> {
        if (entity_body.size() < 2 || entity_body.front() != '#') {
            return std::nullopt;
        }

        uint32_t       value = 0;
        size_t         index = 1;
        const unsigned base  = (index < entity_body.size() && (entity_body[index] == 'x' || entity_body[index] == 'X')) ? 16U : 10U;
        if (base == 16U) {
            ++index;
        }
        if (index >= entity_body.size()) {
            return std::nullopt;
        }

        for (; index < entity_body.size(); ++index) {
            const char ch = entity_body[index];
            if (base == 16U) {
                if (!is_hex_digit(ch)) {
                    return std::nullopt;
                }
                value = static_cast<uint32_t>(value * 16 + (is_digit(ch) ? (ch - '0') : (to_lower(ch) - 'a' + 10)));
            } else {
                if (!is_digit(ch)) {
                    return std::nullopt;
                }
                value = static_cast<uint32_t>(value * 10 + (ch - '0'));
            }
            if (value > 0x10FFFF) {
                return std::nullopt;
            }
        }

        if (value == 0 || (value >= 0xD800 && value <= 0xDFFF)) {
            return std::nullopt;
        }
        return value;
    };

    const std::string_view input(text);
    std::string            out;
    out.reserve(text.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '&') {
            const size_t semicolon = input.find(';', i + 1);
            if (semicolon != std::string_view::npos) {
                const std::string_view entity_body = input.substr(i + 1, semicolon - i - 1);

                if (const auto code_point = parse_numeric_entity(entity_body); code_point.has_value()) {
                    append_code_point_utf8(out, *code_point);
                    i = semicolon + 1;
                    continue;
                }

                bool matched_named_entity = false;
                for (const auto& [name, replacement] : named_entities) {
                    if (entity_body == name) {
                        out.append(replacement);
                        i                    = semicolon + 1;
                        matched_named_entity = true;
                        break;
                    }
                }

                if (matched_named_entity) {
                    continue;
                }
            }
        }
        out.push_back(input[i]);
        ++i;
    }

    return out;
}

std::string normalize_whitespace(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    bool in_whitespace = false;
    for (char c : text) {
        if (is_whitespace(c)) {
            if (!in_whitespace) {
                result += ' ';  // 用单个空格替换所有连续空白
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }

    return result;
}

}  // namespace hps
