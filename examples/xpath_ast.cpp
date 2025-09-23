#include "hps/query/xpath/xpath_ast.hpp"

#include <iostream>
#include <memory>
#include <vector>

using namespace hps;

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    std::cout << "=== XPath AST 示例 ===" << std::endl;

    // 1. 创建字面量表达式
    std::cout << "\n1. 字面量表达式:" << std::endl;
    auto literal = std::make_unique<XPathLiteral>("Hello World");
    std::cout << "  字符串字面量: " << literal->to_string() << std::endl;

    auto number = std::make_unique<XPathNumber>(42.5);
    std::cout << "  数字字面量: " << number->to_string() << std::endl;

    // 2. 创建变量引用
    std::cout << "\n2. 变量引用:" << std::endl;
    auto variable = std::make_unique<XPathVariableReference>("myVar");
    std::cout << "  变量引用: " << variable->to_string() << std::endl;

    // 3. 创建二元表达式
    std::cout << "\n3. 二元表达式:" << std::endl;
    auto left   = std::make_unique<XPathNumber>(10);
    auto right  = std::make_unique<XPathNumber>(5);
    auto binary = std::make_unique<XPathBinaryExpression>(std::move(left), XPathBinaryExpression::Operator::PLUS, std::move(right));
    std::cout << "  加法表达式: " << binary->to_string() << std::endl;

    // 4. 创建一元表达式
    std::cout << "\n4. 一元表达式:" << std::endl;
    auto operand = std::make_unique<XPathNumber>(123);
    auto unary   = std::make_unique<XPathUnaryExpression>(XPathUnaryExpression::Operator::NEGATE, std::move(operand));
    std::cout << "  负号表达式: " << unary->to_string() << std::endl;

    // 5. 创建函数调用
    std::cout << "\n5. 函数调用:" << std::endl;
    std::vector<std::unique_ptr<XPathExpression>> args;
    args.push_back(std::make_unique<XPathLiteral>("test"));
    args.push_back(std::make_unique<XPathNumber>(1));
    auto function = std::make_unique<XPathFunctionCall>("substring", std::move(args));
    std::cout << "  函数调用: " << function->to_string() << std::endl;

    // 6. 创建节点测试
    std::cout << "\n6. 节点测试:" << std::endl;

    // 名称测试
    auto nameTest = std::make_unique<XPathNameTest>("", "div");
    std::cout << "  名称测试: " << nameTest->to_string() << std::endl;

    // 带命名空间的名称测试
    auto nsNameTest = std::make_unique<XPathNameTest>("html", "div");
    std::cout << "  命名空间名称测试: " << nsNameTest->to_string() << std::endl;

    // 通配符测试
    auto wildcardTest = std::make_unique<XPathWildcardTest>();
    std::cout << "  通配符测试: " << wildcardTest->to_string() << std::endl;

    // 节点类型测试
    auto textTest = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::TEXT);
    std::cout << "  文本节点测试: " << textTest->to_string() << std::endl;

    auto commentTest = std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::COMMENT);
    std::cout << "  注释节点测试: " << commentTest->to_string() << std::endl;

    // 7. 创建谓词
    std::cout << "\n7. 谓词:" << std::endl;
    auto predicateExpr = std::make_unique<XPathNumber>(1);
    auto predicate     = std::make_unique<XPathPredicate>(std::move(predicateExpr));
    std::cout << "  谓词: [" << predicate->to_string() << "]" << std::endl;

    // 8. 创建步骤
    std::cout << "\n8. XPath 步骤:" << std::endl;

    // 简单步骤
    auto                                         stepNodeTest = std::make_unique<XPathNameTest>("", "p");
    std::vector<std::unique_ptr<XPathPredicate>> predicates;
    auto                                         step = std::make_unique<XPathStep>(XPathAxis::CHILD, std::move(stepNodeTest), std::move(predicates));
    std::cout << "  子轴步骤: " << step->to_string() << std::endl;

    // 带谓词的步骤 - 修复类型错误
    auto                                         stepNodeTest2 = std::make_unique<XPathNameTest>("", "div");
    std::vector<std::unique_ptr<XPathPredicate>> predicates2;

    // 创建一个简单的数字谓词而不是复杂的属性比较
    auto predicateExpr2 = std::make_unique<XPathNumber>(1);
    predicates2.push_back(std::make_unique<XPathPredicate>(std::move(predicateExpr2)));

    auto stepWithPredicate = std::make_unique<XPathStep>(XPathAxis::CHILD, std::move(stepNodeTest2), std::move(predicates2));
    std::cout << "  带谓词的步骤: " << stepWithPredicate->to_string() << std::endl;

    // 属性轴步骤
    auto                                         attrNodeTest = std::make_unique<XPathNameTest>("", "id");
    std::vector<std::unique_ptr<XPathPredicate>> attrPredicates;
    auto                                         attrStep = std::make_unique<XPathStep>(XPathAxis::ATTRIBUTE, std::move(attrNodeTest), std::move(attrPredicates));
    std::cout << "  属性轴步骤: " << attrStep->to_string() << std::endl;

    // 9. 创建位置路径
    std::cout << "\n9. 位置路径:" << std::endl;

    // 相对路径
    std::vector<std::unique_ptr<XPathStep>> relativeSteps;
    auto                                    relStep1 = std::make_unique<XPathStep>(XPathAxis::CHILD, std::make_unique<XPathNameTest>("", "div"), std::vector<std::unique_ptr<XPathPredicate>>());
    auto                                    relStep2 = std::make_unique<XPathStep>(XPathAxis::CHILD, std::make_unique<XPathNameTest>("", "p"), std::vector<std::unique_ptr<XPathPredicate>>());
    relativeSteps.push_back(std::move(relStep1));
    relativeSteps.push_back(std::move(relStep2));
    auto relativePath = std::make_unique<XPathLocationPath>(std::move(relativeSteps), false);
    std::cout << "  相对路径: " << relativePath->to_string() << std::endl;

    // 绝对路径
    std::vector<std::unique_ptr<XPathStep>> absoluteSteps;
    auto                                    absStep1 = std::make_unique<XPathStep>(XPathAxis::CHILD, std::make_unique<XPathNameTest>("", "html"), std::vector<std::unique_ptr<XPathPredicate>>());
    auto                                    absStep2 = std::make_unique<XPathStep>(XPathAxis::CHILD, std::make_unique<XPathNameTest>("", "body"), std::vector<std::unique_ptr<XPathPredicate>>());
    absoluteSteps.push_back(std::move(absStep1));
    absoluteSteps.push_back(std::move(absStep2));
    auto absolutePath = std::make_unique<XPathLocationPath>(std::move(absoluteSteps), true);
    std::cout << "  绝对路径: " << absolutePath->to_string() << std::endl;

    // 10. 复杂表达式组合
    std::cout << "\n10. 复杂表达式组合:" << std::endl;

    // 创建联合表达式
    auto leftSide  = std::make_unique<XPathNumber>(1);
    auto rightSide = std::make_unique<XPathNumber>(2);
    auto unionExpr = std::make_unique<XPathBinaryExpression>(std::move(leftSide), XPathBinaryExpression::Operator::UNION, std::move(rightSide));
    std::cout << "  联合表达式: " << unionExpr->to_string() << std::endl;

    // 11. 演示各种轴类型
    std::cout << "\n11. 各种轴类型:" << std::endl;

    // 后代轴
    auto descendantStep = std::make_unique<XPathStep>(XPathAxis::DESCENDANT, std::make_unique<XPathNameTest>("", "span"), std::vector<std::unique_ptr<XPathPredicate>>());
    std::cout << "  后代轴: " << descendantStep->to_string() << std::endl;

    // 父轴
    auto parentStep = std::make_unique<XPathStep>(XPathAxis::PARENT, std::make_unique<XPathNodeTypeTest>(XPathNodeTypeTest::NodeType::NODE), std::vector<std::unique_ptr<XPathPredicate>>());
    std::cout << "  父轴: " << parentStep->to_string() << std::endl;

    // 自身轴
    auto selfStep = std::make_unique<XPathStep>(XPathAxis::SELF, std::make_unique<XPathWildcardTest>(), std::vector<std::unique_ptr<XPathPredicate>>());
    std::cout << "  自身轴: " << selfStep->to_string() << std::endl;

    std::cout << "\n=== XPath AST 示例完成 ===" << std::endl;
    return 0;
}