#include "hps/query/xpath/xpath_lexer.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace hps;

/**
 * @brief 将 XPathTokenType 转换为可读的字符串
 */
std::string token_type_to_string(XPathTokenType type) {
    switch (type) {
        // 基本符号
        case XPathTokenType::SLASH:
            return "SLASH";
        case XPathTokenType::DOUBLE_SLASH:
            return "DOUBLE_SLASH";
        case XPathTokenType::DOT:
            return "DOT";
        case XPathTokenType::DOUBLE_DOT:
            return "DOUBLE_DOT";
        case XPathTokenType::AT:
            return "AT";
        case XPathTokenType::STAR:
            return "STAR";
        case XPathTokenType::COLON:
            return "COLON";
        case XPathTokenType::DOUBLE_COLON:
            return "DOUBLE_COLON";

        // 括号和分隔符
        case XPathTokenType::LPAREN:
            return "LPAREN";
        case XPathTokenType::RPAREN:
            return "RPAREN";
        case XPathTokenType::LBRACKET:
            return "LBRACKET";
        case XPathTokenType::RBRACKET:
            return "RBRACKET";
        case XPathTokenType::COMMA:
            return "COMMA";
        case XPathTokenType::PIPE:
            return "PIPE";

        // 比较操作符
        case XPathTokenType::EQUAL:
            return "EQUAL";
        case XPathTokenType::NOT_EQUAL:
            return "NOT_EQUAL";
        case XPathTokenType::LESS:
            return "LESS";
        case XPathTokenType::LESS_EQUAL:
            return "LESS_EQUAL";
        case XPathTokenType::GREATER:
            return "GREATER";
        case XPathTokenType::GREATER_EQUAL:
            return "GREATER_EQUAL";

        // 算术操作符
        case XPathTokenType::PLUS:
            return "PLUS";
        case XPathTokenType::MINUS:
            return "MINUS";

        // 逻辑操作符
        case XPathTokenType::AND:
            return "AND";
        case XPathTokenType::OR:
            return "OR";

        // 数学操作符
        case XPathTokenType::DIV:
            return "DIV";
        case XPathTokenType::MOD:
            return "MOD";

        // 轴标识符
        case XPathTokenType::AXIS_ANCESTOR:
            return "AXIS_ANCESTOR";
        case XPathTokenType::AXIS_ANCESTOR_OR_SELF:
            return "AXIS_ANCESTOR_OR_SELF";
        case XPathTokenType::AXIS_ATTRIBUTE:
            return "AXIS_ATTRIBUTE";
        case XPathTokenType::AXIS_CHILD:
            return "AXIS_CHILD";
        case XPathTokenType::AXIS_DESCENDANT:
            return "AXIS_DESCENDANT";
        case XPathTokenType::AXIS_DESCENDANT_OR_SELF:
            return "AXIS_DESCENDANT_OR_SELF";
        case XPathTokenType::AXIS_FOLLOWING:
            return "AXIS_FOLLOWING";
        case XPathTokenType::AXIS_FOLLOWING_SIBLING:
            return "AXIS_FOLLOWING_SIBLING";
        case XPathTokenType::AXIS_NAMESPACE:
            return "AXIS_NAMESPACE";
        case XPathTokenType::AXIS_PARENT:
            return "AXIS_PARENT";
        case XPathTokenType::AXIS_PRECEDING:
            return "AXIS_PRECEDING";
        case XPathTokenType::AXIS_PRECEDING_SIBLING:
            return "AXIS_PRECEDING_SIBLING";
        case XPathTokenType::AXIS_SELF:
            return "AXIS_SELF";

        // 节点类型
        case XPathTokenType::NODE_TYPE_COMMENT:
            return "NODE_TYPE_COMMENT";
        case XPathTokenType::NODE_TYPE_TEXT:
            return "NODE_TYPE_TEXT";
        case XPathTokenType::NODE_TYPE_PROCESSING_INSTRUCTION:
            return "NODE_TYPE_PROCESSING_INSTRUCTION";
        case XPathTokenType::NODE_TYPE_NODE:
            return "NODE_TYPE_NODE";

        // 标识符和字面量
        case XPathTokenType::FUNCTION_NAME:
            return "FUNCTION_NAME";
        case XPathTokenType::IDENTIFIER:
            return "IDENTIFIER";
        case XPathTokenType::STRING_LITERAL:
            return "STRING_LITERAL";
        case XPathTokenType::NUMBER_LITERAL:
            return "NUMBER_LITERAL";

        // 特殊标记
        case XPathTokenType::END_OF_FILE:
            return "END_OF_FILE";
        case XPathTokenType::ERR:
            return "ERR";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief 打印词法标记
 */
void print_token(const XPathToken& token) {
    std::cout << std::left << std::setw(25) << token_type_to_string(token.type) << " | " << std::setw(15) << ("'" + token.value + "'") << " | pos: " << token.position << std::endl;
}

/**
 * @brief 分析并打印 XPath 表达式的所有词法标记
 */
void analyze_xpath_expression(const std::string& xpath) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "分析 XPath 表达式: " << xpath << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::left << std::setw(25) << "Token Type"
              << " | " << std::setw(15) << "Value"
              << " | Position" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    XPathLexer lexer(xpath);

    while (lexer.has_next()) {
        XPathToken token = lexer.next_token();
        print_token(token);

        if (token.type == XPathTokenType::END_OF_FILE || token.type == XPathTokenType::ERR) {
            break;
        }
    }
}

/**
 * @brief 演示前瞻功能
 */
void demonstrate_peek_functionality() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "演示前瞻功能 (peek)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    const std::string xpath = "//div[@class='test']";
    XPathLexer        lexer(xpath);

    std::cout << "原始表达式: " << xpath << std::endl;
    std::cout << "\n使用 peek_token() 预览而不消费标记:" << std::endl;

    // 预览前几个标记
    for (int i = 0; i < 3 && lexer.has_next(); ++i) {
        XPathToken peeked = lexer.peek_token();
        std::cout << "Peek " << (i + 1) << ": " << token_type_to_string(peeked.type) << " (" << peeked.value << ")" << std::endl;

        // 实际消费标记
        XPathToken consumed = lexer.next_token();
        std::cout << "Consumed: " << token_type_to_string(consumed.type) << " (" << consumed.value << ")" << std::endl;
        std::cout << std::endl;
    }
}

/**
 * @brief 测试各种 XPath 表达式类型
 */
void test_various_xpath_expressions() {
    std::vector<std::string> test_cases = {// 基本路径表达式
                                           "/html/body/div",
                                           "//div",
                                           "./child",
                                           "../parent",

                                           // 属性选择
                                           "//div[@class]",
                                           "//input[@type='text']",
                                           "//*[@id=\"main\"]",

                                           // 谓词表达式
                                           "//div[1]",
                                           "//div[position()=1]",
                                           "//div[last()]",
                                           "//div[@class='test' and @id='main']",

                                           // 轴表达式
                                           "child::div",
                                           "descendant::p",
                                           "ancestor-or-self::*",
                                           "following-sibling::span",

                                           // 函数调用
                                           "//div[contains(@class, 'test')]",
                                           "//text()[normalize-space()]",
                                           "count(//div)",
                                           "string-length(@title)",

                                           // 复杂表达式
                                           "//div[@class='container']//p[position() > 1 and text()]",
                                           "//a[@href and starts-with(@href, 'http')]",

                                           // 数学表达式
                                           "//div[count(p) > 2]",
                                           "//item[price div 2 > 10]",
                                           "//product[@id mod 2 = 0]",

                                           // 节点类型测试
                                           "//comment()",
                                           "//text()",
                                           "//processing-instruction()",
                                           "//node()",

                                           // 联合表达式
                                           "//div | //span",
                                           "//h1 | //h2 | //h3"};

    for (const auto& xpath : test_cases) {
        analyze_xpath_expression(xpath);
    }
}

/**
 * @brief 演示错误处理
 */
void demonstrate_error_handling() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "演示词法分析错误处理" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    std::vector<std::string> lexical_error_cases = {
        "//div[@class='unclosed string",  // 未闭合的字符串
        "//div[#invalid]",                // 无效字符 #
        "//div[%invalid]",                // 无效字符 %
        "//div[@class=\"unclosed quote",  // 未闭合的双引号字符串
        "//div[&invalid]"                 // 无效字符 &
    };

    std::cout << "以下是真正的词法错误（会产生 ERR 标记）：\n" << std::endl;

    for (const auto& xpath : lexical_error_cases) {
        std::cout << "测试词法错误表达式: " << xpath << std::endl;
        XPathLexer lexer(xpath);

        bool found_error = false;
        while (lexer.has_next()) {
            XPathToken token = lexer.next_token();
            print_token(token);

            if (token.type == XPathTokenType::ERR) {
                std::cout << ">>> 发现词法错误标记!" << std::endl;
                found_error = true;
                break;
            }

            if (token.type == XPathTokenType::END_OF_FILE) {
                break;
            }
        }

        if (!found_error) {
            std::cout << ">>> 词法分析正常完成（无词法错误）" << std::endl;
        }
        std::cout << std::endl;
    }
}
/**
 * @brief 解释词法分析与语法分析的区别
 */
void explain_lexical_vs_syntax_analysis() {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "词法分析 vs 语法分析" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    std::cout << "\n词法分析器（Lexer）的职责：" << std::endl;
    std::cout << "- 将输入字符串分解为词法标记（tokens）" << std::endl;
    std::cout << "- 识别关键字、标识符、操作符、字面量等" << std::endl;
    std::cout << "- 只关心单个标记是否有效，不关心标记之间的关系" << std::endl;

    std::cout << "\n语法分析器（Parser）的职责：" << std::endl;
    std::cout << "- 检查标记序列是否符合语法规则" << std::endl;
    std::cout << "- 构建抽象语法树（AST）" << std::endl;
    std::cout << "- 检测语法错误和语义错误" << std::endl;

    std::cout << "\n示例对比：" << std::endl;

    struct Example {
        std::string expression;
        std::string lexical_result;
        std::string syntax_result;
    };

    std::vector<Example> examples = {{"//div[@class='test']", "✓ 词法正确", "✓ 语法正确"},
                                     {"//div[@class=]", "✓ 词法正确（所有标记都能识别）", "✗ 语法错误（= 后缺少值）"},
                                     {"//div[@@class]", "✓ 词法正确（@ 是有效标记）", "✗ 语法错误（连续的 @ 不符合语法）"},
                                     {"//div[#invalid]", "✗ 词法错误（# 不是有效标记）", "- 无法进行语法分析"}};

    for (const auto& example : examples) {
        std::cout << "\n表达式: " << example.expression << std::endl;
        std::cout << "  词法分析: " << example.lexical_result << std::endl;
        std::cout << "  语法分析: " << example.syntax_result << std::endl;
    }
}

int main() {
#ifdef _WIN32
    // 设置控制台支持 UTF-8
    system("chcp 65001 > nul");
#endif

    std::cout << "XPath Lexer 示例程序" << std::endl;
    std::cout << "===================" << std::endl;

    try {
        // 测试各种 XPath 表达式
        test_various_xpath_expressions();

        // 演示前瞻功能
        demonstrate_peek_functionality();

        // 解释词法分析与语法分析的区别
        explain_lexical_vs_syntax_analysis();

        // 演示错误处理
        demonstrate_error_handling();

        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "示例程序执行完成!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("pause");
#endif

    return 0;
}