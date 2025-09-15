#pragma once

#include "css_selector.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace hps {

// CSS解析器异常
class CSSParseException : public std::exception {
  public:
    explicit CSSParseException(const std::string& message, size_t position = 0) : m_message(message), m_position(position) {}

    const char* what() const noexcept override {
        return m_message.c_str();
    }
    size_t position() const {
        return m_position;
    }

  private:
    std::string m_message;
    size_t      m_position;
};

// CSS词法分析器
class CSSLexer {
  public:
    enum class TokenType {
        Identifier,    // div, class-name
        Hash,          // #id
        Dot,           // .
        Star,          // *
        LeftBracket,   // [
        RightBracket,  // ]
        Equals,        // =
        Contains,      // *=
        StartsWith,    // ^=
        EndsWith,      // $=
        WordMatch,     // ~=
        LangMatch,     // |=
        String,        // "value" or 'value'
        Greater,       // >
        Plus,          // +
        Tilde,         // ~
        Comma,         // ,
        Whitespace,    // space, tab, newline
        EndOfFile
    };

    struct Token {
        TokenType   type;
        std::string value;
        size_t      position;

        Token(TokenType t, std::string v, size_t pos) : type(t), value(std::move(v)), position(pos) {}
    };

    explicit CSSLexer(std::string_view input);

    Token  next_token();
    Token  peek_token();
    bool   has_more_tokens() const;
    size_t current_position() const {
        return m_position;
    }

  private:
    std::string_view m_input;
    size_t           m_position;
    Token            m_current_token;
    bool             m_has_current;

    void        skip_whitespace();
    char        current_char() const;
    char        peek_char(size_t offset = 1) const;
    void        advance();
    std::string read_identifier();
    std::string read_string(char quote);
};

// CSS解析器
class CSSParser {
  public:
    explicit CSSParser(std::string_view selector);

    // 解析选择器列表 (逗号分隔的选择器)
    std::unique_ptr<SelectorList> parse_selector_list();

    // 解析单个选择器
    std::unique_ptr<CSSSelector> parse_selector();

  private:
    CSSLexer m_lexer;

    // 解析复合选择器 (如 div.class#id)
    std::unique_ptr<CSSSelector> parse_compound_selector();

    // 解析简单选择器
    std::unique_ptr<CSSSelector> parse_simple_selector();

    // 解析类型选择器
    std::unique_ptr<TypeSelector> parse_type_selector();

    // 解析类选择器
    std::unique_ptr<ClassSelector> parse_class_selector();

    // 解析ID选择器
    std::unique_ptr<IdSelector> parse_id_selector();

    // 解析属性选择器
    std::unique_ptr<AttributeSelector> parse_attribute_selector();

    // 解析组合器
    SelectorType parse_combinator();

    // 解析属性操作符
    AttributeOperator parse_attribute_operator();

    // 工具方法
    void consume_token(CSSLexer::TokenType expected);
    bool match_token(CSSLexer::TokenType type);
    void skip_whitespace();

    [[noreturn]] void throw_parse_error(const std::string& message);
};

// 便利函数
std::unique_ptr<SelectorList> parse_css_selector(std::string_view selector);

}  // namespace hps