#include "hps/query/xpath/xpath_parser.hpp"

#include "hps/query/xpath/xpath_ast.hpp"
#include "hps/utils/exception.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace hps;

// 辅助函数：打印XPath表达式类型
std::string get_expression_type(const XPathExpression* expr) {
    if (dynamic_cast<const XPathLocationPath*>(expr)) {
        return "LocationPath";
    } else if (dynamic_cast<const XPathFilterExpression*>(expr)) {
        return "FilterExpression";
    } else if (dynamic_cast<const XPathBinaryExpression*>(expr)) {
        return "BinaryExpression";
    } else if (dynamic_cast<const XPathUnaryExpression*>(expr)) {
        return "UnaryExpression";
    } else if (dynamic_cast<const XPathFunctionCall*>(expr)) {
        return "FunctionCall";
    } else if (dynamic_cast<const XPathLiteral*>(expr)) {
        return "Literal";
    } else if (dynamic_cast<const XPathNumber*>(expr)) {
        return "Number";
    } else {
        return "Unknown";
    }
}

// 辅助函数：打印轴类型
std::string get_axis_name(XPathAxis axis) {
    switch (axis) {
        case XPathAxis::CHILD:
            return "child";
        case XPathAxis::DESCENDANT:
            return "descendant";
        case XPathAxis::PARENT:
            return "parent";
        case XPathAxis::ANCESTOR:
            return "ancestor";
        case XPathAxis::FOLLOWING_SIBLING:
            return "following-sibling";
        case XPathAxis::PRECEDING_SIBLING:
            return "preceding-sibling";
        case XPathAxis::FOLLOWING:
            return "following";
        case XPathAxis::PRECEDING:
            return "preceding";
        case XPathAxis::ATTRIBUTE:
            return "attribute";
        case XPathAxis::NAMESPACE:
            return "namespace";
        case XPathAxis::SELF:
            return "self";
        case XPathAxis::DESCENDANT_OR_SELF:
            return "descendant-or-self";
        case XPathAxis::ANCESTOR_OR_SELF:
            return "ancestor-or-self";
        default:
            return "unknown";
    }
}

// 辅助函数：打印节点测试信息
void print_node_test(const XPathNodeTest* test, int indent = 0) {
    std::string prefix(indent, ' ');

    if (auto name_test = dynamic_cast<const XPathNameTest*>(test)) {
        std::cout << prefix << "NameTest: ";
        if (!name_test->prefix().empty()) {
            std::cout << name_test->prefix() << ":";
        }
        std::cout << name_test->local_name() << std::endl;
    } else if (dynamic_cast<const XPathWildcardTest*>(test)) {
        std::cout << prefix << "WildcardTest: *" << std::endl;
    } else if (auto node_type_test = dynamic_cast<const XPathNodeTypeTest*>(test)) {
        std::cout << prefix << "NodeTypeTest: ";
        switch (node_type_test->node_type()) {
            case XPathNodeTypeTest::NodeType::TEXT:
                std::cout << "text()";
                break;
            case XPathNodeTypeTest::NodeType::COMMENT:
                std::cout << "comment()";
                break;
            case XPathNodeTypeTest::NodeType::NODE:
                std::cout << "node()";
                break;
        }
        std::cout << std::endl;
    } else {
        std::cout << prefix << "Unknown NodeTest" << std::endl;
    }
}

// 辅助函数：打印步骤信息
void print_step(const XPathStep* step, int indent = 0) {
    std::string prefix(indent, ' ');
    std::cout << prefix << "Step:" << std::endl;
    std::cout << prefix << "  Axis: " << get_axis_name(step->axis()) << std::endl;
    std::cout << prefix << "  NodeTest:" << std::endl;
    print_node_test(&step->node_test(), indent + 4);

    if (!step->predicates().empty()) {
        std::cout << prefix << "  Predicates: " << step->predicates().size() << std::endl;
    }
}

// 辅助函数：打印位置路径信息
void print_location_path(const XPathLocationPath* location_path, int indent = 0) {
    std::string prefix(indent, ' ');
    std::cout << prefix << "LocationPath:" << std::endl;
    std::cout << prefix << "  Absolute: " << (location_path->is_absolute() ? "true" : "false") << std::endl;
    std::cout << prefix << "  Steps: " << location_path->steps().size() << std::endl;

    for (size_t i = 0; i < location_path->steps().size(); ++i) {
        std::cout << prefix << "  Step " << i << ":" << std::endl;
        print_step(location_path->steps()[i].get(), indent + 4);
    }
}

// 辅助函数：打印表达式信息
void print_expression(const XPathExpression* expr, int indent = 0) {
    std::string prefix(indent, ' ');
    std::cout << prefix << "Expression Type: " << get_expression_type(expr) << std::endl;

    if (auto location_path = dynamic_cast<const XPathLocationPath*>(expr)) {
        print_location_path(location_path, indent);
    } else if (auto filter_expr = dynamic_cast<const XPathFilterExpression*>(expr)) {
        std::cout << prefix << "FilterExpression:" << std::endl;
        std::cout << prefix << "  PrimaryExpression:" << std::endl;
        print_expression(filter_expr->primary_expr(), indent + 4);
        if (filter_expr->has_predicates()) {
            std::cout << prefix << "  Predicates: " << filter_expr->predicates().size() << std::endl;
            for (size_t i = 0; i < filter_expr->predicates().size(); ++i) {
                std::cout << prefix << "  Predicate " << i << ": " << filter_expr->predicates()[i]->to_string() << std::endl;
            }
        }
    } else if (auto literal = dynamic_cast<const XPathLiteral*>(expr)) {
        std::cout << prefix << "Literal: \"" << literal->value() << "\"" << std::endl;
    } else if (auto number = dynamic_cast<const XPathNumber*>(expr)) {
        std::cout << prefix << "Number: " << number->value() << std::endl;
    } else if (auto func_call = dynamic_cast<const XPathFunctionCall*>(expr)) {
        std::cout << prefix << "FunctionCall: " << func_call->name() << "()" << std::endl;
        std::cout << prefix << "  Arguments: " << func_call->arguments().size() << std::endl;
    }
}

// 测试XPath解析器的函数
void test_xpath_parser(const std::string& xpath_expr) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Testing XPath: " << xpath_expr << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    try {
        XPathParser parser(xpath_expr);
        auto        result = parser.parse();

        if (result) {
            std::cout << "✓ Parsing successful!" << std::endl;
            print_expression(result.get());
        } else {
            std::cout << "✗ Parsing failed: null result" << std::endl;
        }
    } catch (const HPSException& e) {
        std::cout << "✗ Parsing failed with exception: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ Parsing failed with standard exception: " << e.what() << std::endl;
    }
}

int main() {
#ifdef _WIN32
    // 设置控制台支持 UTF-8
    system("chcp 65001 > nul");
#endif
    std::cout << "XPath Parser Demo" << std::endl;
    std::cout << "=================" << std::endl;

    // 测试各种XPath表达式
    std::vector<std::string> test_expressions = {// 简单路径
                                                 "/",
                                                 "//",
                                                 "/html",
                                                 "//div",
                                                 "/html/body",
                                                 "//div/p",

                                                 // 带属性的路径
                                                 "//div[@class]",
                                                 "//a[@href='http://example.com']",

                                                 // 轴表达式
                                                 "child::div",
                                                 "descendant::p",
                                                 "attribute::class",

                                                 // 节点类型测试
                                                 "//text()",
                                                 "//comment()",
                                                 "//node()",

                                                 // 通配符
                                                 "//*",
                                                 "//div/*",

                                                 // 数字和字符串字面量
                                                 "123",
                                                 "'hello world'",
                                                 "\"test string\"",

                                                 // 函数调用（如果实现了的话）
                                                 "count(//div)",
                                                 "text()",

                                                 // 复合表达式
                                                 "/html/body//div[@class='content']",
                                                 "//div[position()=1]",
                                                 "//p[text()='Hello']"};

    for (const auto& expr : test_expressions) {
        test_xpath_parser(expr);
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Demo completed!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    return 0;
}