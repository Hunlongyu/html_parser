#include "hps/parsing/tokenizer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifndef HPS_SOURCE_DIR
#error "HPS_SOURCE_DIR must be defined for baseline tests"
#endif

namespace {

class JsonValue {
  public:
    using array_type  = std::vector<JsonValue>;
    using object_type = std::map<std::string, JsonValue, std::less<>>;
    using value_type  = std::variant<std::nullptr_t, bool, std::int64_t, std::string, array_type, object_type>;

    JsonValue() noexcept : m_value(nullptr) {}

    explicit JsonValue(std::nullptr_t) noexcept : m_value(nullptr) {}
    explicit JsonValue(const bool value) noexcept : m_value(value) {}
    explicit JsonValue(const std::int64_t value) noexcept : m_value(value) {}
    explicit JsonValue(std::string value) : m_value(std::move(value)) {}
    explicit JsonValue(array_type value) : m_value(std::move(value)) {}
    explicit JsonValue(object_type value) : m_value(std::move(value)) {}

    [[nodiscard]] auto is_null() const noexcept -> bool {
        return std::holds_alternative<std::nullptr_t>(m_value);
    }

    [[nodiscard]] auto as_bool() const -> bool {
        return std::get<bool>(m_value);
    }

    [[nodiscard]] auto as_int() const -> std::int64_t {
        return std::get<std::int64_t>(m_value);
    }

    [[nodiscard]] auto as_string() const -> const std::string& {
        return std::get<std::string>(m_value);
    }

    [[nodiscard]] auto as_array() const -> const array_type& {
        return std::get<array_type>(m_value);
    }

    [[nodiscard]] auto as_object() const -> const object_type& {
        return std::get<object_type>(m_value);
    }

  private:
    value_type m_value;
};

class JsonParser {
  public:
    explicit JsonParser(std::string_view input) noexcept : m_input(input) {}

    [[nodiscard]] auto parse_document() -> JsonValue {
        auto value = parse_value();
        skip_whitespace();
        if (m_pos != m_input.size()) {
            fail("Unexpected trailing JSON content");
        }
        return value;
    }

  private:
    [[nodiscard]] auto parse_value() -> JsonValue {
        skip_whitespace();
        if (m_pos >= m_input.size()) {
            fail("Unexpected end of JSON input");
        }

        switch (m_input[m_pos]) {
            case '{':
                return parse_object();
            case '[':
                return parse_array();
            case '"':
                return JsonValue(parse_string());
            case 't':
                consume_literal("true");
                return JsonValue(true);
            case 'f':
                consume_literal("false");
                return JsonValue(false);
            case 'n':
                consume_literal("null");
                return JsonValue(nullptr);
            default:
                if (m_input[m_pos] == '-' || std::isdigit(static_cast<unsigned char>(m_input[m_pos])) != 0) {
                    return JsonValue(parse_integer());
                }
                fail("Unexpected character while parsing JSON value");
        }
    }

    [[nodiscard]] auto parse_object() -> JsonValue {
        expect('{');
        JsonValue::object_type object;
        skip_whitespace();
        if (consume_if('}')) {
            return JsonValue(std::move(object));
        }

        while (true) {
            skip_whitespace();
            const auto key = parse_string();
            skip_whitespace();
            expect(':');
            object.insert_or_assign(key, parse_value());
            skip_whitespace();
            if (consume_if('}')) {
                break;
            }
            expect(',');
        }

        return JsonValue(std::move(object));
    }

    [[nodiscard]] auto parse_array() -> JsonValue {
        expect('[');
        JsonValue::array_type array;
        skip_whitespace();
        if (consume_if(']')) {
            return JsonValue(std::move(array));
        }

        while (true) {
            array.push_back(parse_value());
            skip_whitespace();
            if (consume_if(']')) {
                break;
            }
            expect(',');
        }

        return JsonValue(std::move(array));
    }

    [[nodiscard]] auto parse_string() -> std::string {
        expect('"');
        std::string result;

        while (m_pos < m_input.size()) {
            const char ch = m_input[m_pos++];
            if (ch == '"') {
                return result;
            }
            if (ch != '\\') {
                result.push_back(ch);
                continue;
            }

            if (m_pos >= m_input.size()) {
                fail("Unexpected end of JSON string escape");
            }

            const char escape = m_input[m_pos++];
            switch (escape) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escape);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u': {
                    char32_t code_point = parse_hex_escape();
                    if (code_point >= 0xD800 && code_point <= 0xDBFF) {
                        const auto saved_pos = m_pos;
                        if (consume_if('\\') && consume_if('u')) {
                            const char32_t low_surrogate = parse_hex_escape();
                            if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
                                code_point = 0x10000U + ((code_point - 0xD800U) << 10U) + (low_surrogate - 0xDC00U);
                            } else {
                                fail("Invalid low surrogate in JSON string escape");
                            }
                        } else {
                            m_pos = saved_pos;
                            fail("Incomplete surrogate pair in JSON string escape");
                        }
                    }
                    append_utf8(result, code_point);
                    break;
                }
                default:
                    fail("Unsupported JSON string escape");
            }
        }

        fail("Unterminated JSON string");
    }

    [[nodiscard]] auto parse_integer() -> std::int64_t {
        const size_t start = m_pos;
        if (m_input[m_pos] == '-') {
            ++m_pos;
        }
        if (m_pos >= m_input.size() || std::isdigit(static_cast<unsigned char>(m_input[m_pos])) == 0) {
            fail("Invalid JSON number");
        }
        while (m_pos < m_input.size() && std::isdigit(static_cast<unsigned char>(m_input[m_pos])) != 0) {
            ++m_pos;
        }
        return std::stoll(std::string(m_input.substr(start, m_pos - start)));
    }

    void consume_literal(std::string_view literal) {
        if (m_input.substr(m_pos, literal.size()) != literal) {
            fail("Unexpected JSON literal");
        }
        m_pos += literal.size();
    }

    void skip_whitespace() noexcept {
        while (m_pos < m_input.size() && std::isspace(static_cast<unsigned char>(m_input[m_pos])) != 0) {
            ++m_pos;
        }
    }

    void expect(const char expected) {
        if (m_pos >= m_input.size() || m_input[m_pos] != expected) {
            fail("Unexpected JSON token");
        }
        ++m_pos;
    }

    [[nodiscard]] auto consume_if(const char expected) noexcept -> bool {
        if (m_pos < m_input.size() && m_input[m_pos] == expected) {
            ++m_pos;
            return true;
        }
        return false;
    }

    [[nodiscard]] auto parse_hex_escape() -> char32_t {
        if (m_pos + 4 > m_input.size()) {
            fail("Incomplete JSON unicode escape");
        }

        char32_t code_point = 0;
        for (int i = 0; i < 4; ++i) {
            code_point = (code_point << 4U) | static_cast<char32_t>(hex_digit(m_input[m_pos++]));
        }
        return code_point;
    }

    [[nodiscard]] static auto hex_digit(const char ch) -> int {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F') {
            return 10 + (ch - 'A');
        }
        throw std::runtime_error("Invalid JSON unicode escape");
    }

    static void append_utf8(std::string& output, const char32_t code_point) {
        if (code_point <= 0x7F) {
            output.push_back(static_cast<char>(code_point));
            return;
        }
        if (code_point <= 0x7FF) {
            output.push_back(static_cast<char>(0xC0 | ((code_point >> 6U) & 0x1F)));
            output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            return;
        }
        if (code_point <= 0xFFFF) {
            output.push_back(static_cast<char>(0xE0 | ((code_point >> 12U) & 0x0F)));
            output.push_back(static_cast<char>(0x80 | ((code_point >> 6U) & 0x3F)));
            output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            return;
        }
        output.push_back(static_cast<char>(0xF0 | ((code_point >> 18U) & 0x07)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 12U) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6U) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }

    [[noreturn]] void fail(std::string_view message) const {
        std::ostringstream stream;
        stream << message << " at offset " << m_pos;
        throw std::runtime_error(stream.str());
    }

    std::string_view m_input;
    size_t           m_pos{0};
};

enum class BaselineTokenKind : std::uint8_t {
    StartTag,
    EndTag,
    Comment,
    Character,
    Doctype
};

struct BaselineToken {
    BaselineTokenKind                        kind;
    std::string                              data;
    std::map<std::string, std::string, std::less<>> attributes;
    std::string                              public_id;
    std::string                              system_id;
    bool                                     self_closing{false};
    bool                                     quirks_mode{false};
};

struct BaselineParseError {
    hps::ErrorCode code;
    size_t         line{0};
    size_t         column{0};
};

struct Html5libTokenizerCase {
    std::string                description;
    std::string                input;
    std::vector<BaselineToken> expected_tokens;
    std::vector<BaselineParseError> expected_errors;
    hps::TokenizerState        initial_state{hps::TokenizerState::Data};
    std::string                last_start_tag;
};

struct TokenizerBaselineResult {
    std::vector<BaselineToken> tokens;
    std::vector<hps::HPSError> errors;
};

[[nodiscard]] auto read_text_file(const std::filesystem::path& path) -> std::string {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open baseline file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

[[nodiscard]] auto parse_attributes(const JsonValue& json_value)
    -> std::map<std::string, std::string, std::less<>> {
    std::map<std::string, std::string, std::less<>> attributes;
    for (const auto& [name, value] : json_value.as_object()) {
        attributes.emplace(name, value.as_string());
    }
    return attributes;
}

[[nodiscard]] auto parse_expected_token(const JsonValue& json_value) -> BaselineToken {
    const auto& token = json_value.as_array();
    if (token.empty()) {
        throw std::runtime_error("html5lib token array must not be empty");
    }

    const auto& token_kind = token.front().as_string();
    if (token_kind == "StartTag") {
        if (token.size() < 3 || token.size() > 4) {
            throw std::runtime_error("Invalid html5lib StartTag token length");
        }
        BaselineToken result{BaselineTokenKind::StartTag, token[1].as_string()};
        result.attributes   = parse_attributes(token[2]);
        result.self_closing = token.size() == 4 ? token[3].as_bool() : false;
        return result;
    }
    if (token_kind == "EndTag") {
        if (token.size() != 2) {
            throw std::runtime_error("Invalid html5lib EndTag token length");
        }
        return {BaselineTokenKind::EndTag, token[1].as_string()};
    }
    if (token_kind == "Comment") {
        if (token.size() != 2) {
            throw std::runtime_error("Invalid html5lib Comment token length");
        }
        return {BaselineTokenKind::Comment, token[1].as_string()};
    }
    if (token_kind == "Character") {
        if (token.size() != 2) {
            throw std::runtime_error("Invalid html5lib Character token length");
        }
        return {BaselineTokenKind::Character, token[1].as_string()};
    }
    if (token_kind == "DOCTYPE") {
        if (token.size() != 5) {
            throw std::runtime_error("Invalid html5lib DOCTYPE token length");
        }
        BaselineToken result{BaselineTokenKind::Doctype, token[1].is_null() ? "" : token[1].as_string()};
        result.public_id   = token[2].is_null() ? "" : token[2].as_string();
        result.system_id   = token[3].is_null() ? "" : token[3].as_string();
        result.quirks_mode = !token[4].as_bool();
        return result;
    }

    throw std::runtime_error("Unsupported html5lib token kind: " + token_kind);
}

[[nodiscard]] auto map_html5lib_error_code(const std::string_view code) -> hps::ErrorCode {
    if (code == "eof-in-comment" || code == "eof-in-doctype" || code == "eof-in-tag") {
        return hps::ErrorCode::UnexpectedEOF;
    }
    if (code == "missing-whitespace-between-attributes") {
        return hps::ErrorCode::InvalidToken;
    }
    throw std::runtime_error("Unsupported html5lib error code in curated baseline: " + std::string(code));
}

[[nodiscard]] auto parse_expected_error(const JsonValue& json_value) -> BaselineParseError {
    const auto& error = json_value.as_object();
    return {
        map_html5lib_error_code(error.at("code").as_string()),
        static_cast<size_t>(error.at("line").as_int()),
        static_cast<size_t>(error.at("col").as_int()),
    };
}

[[nodiscard]] auto parse_html5lib_state(const std::string_view state_name) -> hps::TokenizerState {
    if (state_name == "Data state") {
        return hps::TokenizerState::Data;
    }
    if (state_name == "RCDATA state") {
        return hps::TokenizerState::RCDATA;
    }
    if (state_name == "RAWTEXT state") {
        return hps::TokenizerState::RAWTEXT;
    }
    if (state_name == "Script data state") {
        return hps::TokenizerState::ScriptData;
    }
    if (state_name == "PLAINTEXT state") {
        return hps::TokenizerState::Plaintext;
    }
    if (state_name == "CDATA section state") {
        return hps::TokenizerState::CDataSection;
    }
    throw std::runtime_error("Unsupported html5lib tokenizer state: " + std::string(state_name));
}

[[nodiscard]] auto parse_case(const JsonValue& json_value) -> std::vector<Html5libTokenizerCase> {
    const auto& test_case = json_value.as_object();

    if (const auto it = test_case.find("doubleEscaped");
        it != test_case.end() && it->second.as_bool()) {
        throw std::runtime_error("doubleEscaped html5lib fixtures are not supported yet");
    }

    std::vector<hps::TokenizerState> initial_states{hps::TokenizerState::Data};
    if (const auto it = test_case.find("initialStates"); it != test_case.end()) {
        initial_states.clear();
        for (const auto& state : it->second.as_array()) {
            initial_states.push_back(parse_html5lib_state(state.as_string()));
        }
    }

    const std::string last_start_tag =
        test_case.contains("lastStartTag") ? test_case.at("lastStartTag").as_string() : std::string{};

    std::vector<Html5libTokenizerCase> cases;
    cases.reserve(initial_states.size());
    for (const auto initial_state : initial_states) {
        Html5libTokenizerCase result;
        result.description    = test_case.at("description").as_string();
        result.input          = test_case.at("input").as_string();
        result.initial_state  = initial_state;
        result.last_start_tag = last_start_tag;
        if (initial_states.size() > 1) {
            result.description += " [" + std::to_string(static_cast<int>(initial_state)) + "]";
        }
        for (const auto& token : test_case.at("output").as_array()) {
            result.expected_tokens.push_back(parse_expected_token(token));
        }
        if (const auto errors_it = test_case.find("errors"); errors_it != test_case.end()) {
            for (const auto& error : errors_it->second.as_array()) {
                result.expected_errors.push_back(parse_expected_error(error));
            }
        }
        cases.push_back(std::move(result));
    }
    return cases;
}

[[nodiscard]] auto load_html5lib_cases(const std::filesystem::path& file_path)
    -> std::vector<Html5libTokenizerCase> {
    const auto root  = JsonParser(read_text_file(file_path)).parse_document();
    const auto& file = root.as_object();

    const auto tests_it = file.find("tests");
    if (tests_it == file.end()) {
        throw std::runtime_error("html5lib baseline file is missing a top-level tests array");
    }

    std::vector<Html5libTokenizerCase> test_cases;
    for (const auto& test_case : tests_it->second.as_array()) {
        auto expanded_cases = parse_case(test_case);
        test_cases.insert(
            test_cases.end(),
            std::make_move_iterator(expanded_cases.begin()),
            std::make_move_iterator(expanded_cases.end()));
    }
    return test_cases;
}

[[nodiscard]] auto baseline_root() -> std::filesystem::path {
    return std::filesystem::path(HPS_SOURCE_DIR) / "tests" / "baselines" / "html5lib" / "tokenizer";
}

void append_character_token(std::vector<BaselineToken>& tokens, std::string_view data) {
    if (data.empty()) {
        return;
    }
    if (!tokens.empty() && tokens.back().kind == BaselineTokenKind::Character) {
        tokens.back().data += data;
        return;
    }
    tokens.push_back(BaselineToken{BaselineTokenKind::Character, std::string(data)});
}

[[nodiscard]] auto normalize_tokens(const std::vector<hps::Token>& tokens) -> std::vector<BaselineToken> {
    std::vector<BaselineToken> normalized;
    normalized.reserve(tokens.size());

    for (const auto& token : tokens) {
        switch (token.type()) {
            case hps::TokenType::OPEN:
            case hps::TokenType::CLOSE_SELF: {
                BaselineToken normalized_token{
                    BaselineTokenKind::StartTag,
                    std::string(token.name()),
                };
                normalized_token.self_closing = token.type() == hps::TokenType::CLOSE_SELF;
                for (const auto& attribute : token.attrs()) {
                    normalized_token.attributes.emplace(attribute.name, attribute.value);
                }
                normalized.push_back(std::move(normalized_token));
                break;
            }
            case hps::TokenType::CLOSE:
                normalized.push_back(BaselineToken{BaselineTokenKind::EndTag, std::string(token.name())});
                break;
            case hps::TokenType::TEXT:
                append_character_token(normalized, token.value());
                break;
            case hps::TokenType::COMMENT:
                normalized.push_back(BaselineToken{BaselineTokenKind::Comment, std::string(token.value())});
                break;
            case hps::TokenType::DOCTYPE: {
                BaselineToken normalized_token{BaselineTokenKind::Doctype, std::string(token.name())};
                normalized_token.public_id   = std::string(token.doctype_public_id());
                normalized_token.system_id   = std::string(token.doctype_system_id());
                normalized_token.quirks_mode = token.doctype_force_quirks();
                normalized.push_back(std::move(normalized_token));
                break;
            }
            case hps::TokenType::FORCE_QUIRKS:
            case hps::TokenType::DONE:
                break;
        }
    }

    return normalized;
}

[[nodiscard]] auto tokenize_to_baseline(
    std::string_view       input,
    const hps::TokenizerState initial_state,
    std::string_view       last_start_tag) -> TokenizerBaselineResult {
    const hps::Options options;
    hps::Tokenizer     tokenizer(input, options, initial_state, last_start_tag);
    auto tokens = tokenizer.tokenize_all();
    return {
        normalize_tokens(tokens),
        tokenizer.consume_errors(),
    };
}

[[nodiscard]] auto kind_name(const BaselineTokenKind kind) -> std::string_view {
    switch (kind) {
        case BaselineTokenKind::StartTag:
            return "StartTag";
        case BaselineTokenKind::EndTag:
            return "EndTag";
        case BaselineTokenKind::Comment:
            return "Comment";
        case BaselineTokenKind::Character:
            return "Character";
        case BaselineTokenKind::Doctype:
            return "DOCTYPE";
    }
    return "Unknown";
}

[[nodiscard]] auto describe_token(const BaselineToken& token) -> std::string {
    std::ostringstream stream;
    stream << kind_name(token.kind) << '(' << token.data;
    if (!token.attributes.empty()) {
        stream << ", attrs={";
        bool first = true;
        for (const auto& [name, value] : token.attributes) {
            if (!first) {
                stream << ", ";
            }
            first = false;
            stream << name << '=' << value;
        }
        stream << '}';
    }
    if (token.self_closing) {
        stream << ", selfClosing";
    }
    if (token.kind == BaselineTokenKind::Doctype) {
        stream << ", publicId=" << token.public_id << ", systemId=" << token.system_id
               << ", quirks=" << (token.quirks_mode ? "true" : "false");
    }
    stream << ')';
    return stream.str();
}

void expect_tokens_match(
    const std::vector<BaselineToken>& expected,
    const std::vector<BaselineToken>& actual,
    const std::filesystem::path&      suite_path,
    std::string_view                  description) {
    SCOPED_TRACE("Suite: " + suite_path.string());
    SCOPED_TRACE("Case: " + std::string(description));

    ASSERT_EQ(actual.size(), expected.size()) << "Tokenizer baseline token count mismatch";
    for (size_t index = 0; index < expected.size(); ++index) {
        SCOPED_TRACE("Token[" + std::to_string(index) + "]");
        EXPECT_EQ(actual[index].kind, expected[index].kind)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].data, expected[index].data)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].attributes, expected[index].attributes)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].self_closing, expected[index].self_closing)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].public_id, expected[index].public_id)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].system_id, expected[index].system_id)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
        EXPECT_EQ(actual[index].quirks_mode, expected[index].quirks_mode)
            << "actual=" << describe_token(actual[index]) << ", expected=" << describe_token(expected[index]);
    }
}

void expect_errors_match(
    const std::vector<BaselineParseError>& expected,
    const std::vector<hps::HPSError>&      actual,
    const std::filesystem::path&           suite_path,
    std::string_view                       description) {
    SCOPED_TRACE("Suite: " + suite_path.string());
    SCOPED_TRACE("Case: " + std::string(description));

    ASSERT_EQ(actual.size(), expected.size()) << "Tokenizer baseline error count mismatch";
    for (size_t index = 0; index < expected.size(); ++index) {
        SCOPED_TRACE("Error[" + std::to_string(index) + "]");
        EXPECT_EQ(actual[index].code, expected[index].code);
        EXPECT_EQ(actual[index].location.line, expected[index].line);
        EXPECT_EQ(actual[index].location.column, expected[index].column);
    }
}

}  // namespace

TEST(Html5libTokenizerBaselineTest, CuratedOfficialCasesMatchTokenizerOutput) {
    const auto root = baseline_root();
    ASSERT_TRUE(std::filesystem::exists(root)) << "Missing tokenizer baseline directory: " << root.string();

    std::vector<std::filesystem::path> suite_paths;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".test") {
            continue;
        }
        suite_paths.push_back(entry.path());
    }

    std::sort(suite_paths.begin(), suite_paths.end());
    ASSERT_FALSE(suite_paths.empty()) << "No html5lib tokenizer baselines found under " << root.string();

    for (const auto& suite_path : suite_paths) {
        const auto test_cases = load_html5lib_cases(suite_path);
        ASSERT_FALSE(test_cases.empty()) << "Baseline file contained no test cases: " << suite_path.string();

        for (const auto& test_case : test_cases) {
            const auto result = tokenize_to_baseline(test_case.input, test_case.initial_state, test_case.last_start_tag);
            expect_tokens_match(test_case.expected_tokens, result.tokens, suite_path, test_case.description);
            expect_errors_match(test_case.expected_errors, result.errors, suite_path, test_case.description);
        }
    }
}
