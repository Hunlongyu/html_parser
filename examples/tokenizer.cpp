#include "hps/parsing/tokenizer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

static std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    std::string html = read_file("./html/base.html");

    hps::Tokenizer tokenizer(html, hps::ErrorHandlingMode::Lenient);

    std::cout << "输入HTML: \n" << html << std::endl << std::endl;

    while (tokenizer.has_more()) {
        if (auto token = tokenizer.next_token()) {
            switch (token->type()) {
                case hps::TokenType::OPEN:
                    std::cout << "开始标签: <" << token->name();
                    if (!token->attrs().empty()) {
                        std::cout << " ";
                        for (const auto& attr : token->attrs()) {
                            std::cout << attr.m_name << "=\"" << attr.m_value << "\" ";
                        }
                    }
                    std::cout << ">" << std::endl;
                    break;

                case hps::TokenType::CLOSE:
                    std::cout << "结束标签: " << "</" << token->name() << ">" << std::endl;
                    break;

                case hps::TokenType::CLOSE_SELF:
                    std::cout << "自闭合标签: <" << token->name();
                    if (!token->attrs().empty()) {
                        std::cout << ", 属性: ";
                        for (const auto& attr : token->attrs()) {
                            std::cout << attr.m_name << "=\"" << attr.m_value << "\" ";
                        }
                    }
                    std::cout << "/>" << std::endl;
                    break;

                case hps::TokenType::DOCTYPE:
                    std::cout << "DOCTYPE 声明: " << token->name() << std::endl;
                    break;

                case hps::TokenType::COMMENT:
                    std::cout << "注释: " << token->value() << std::endl;
                    break;

                case hps::TokenType::TEXT:
                    if (!token->value().empty() && token->value() != " " && token->value() != "\n") {
                        std::cout << "文本内容: " << token->value() << std::endl;
                    }
                    break;

                case hps::TokenType::FORCE_QUIRKS:
                    std::cout << "强制怪异模式标记" << std::endl;
                    break;

                case hps::TokenType::DONE:
                    std::cout << "解析完成" << std::endl;
                    break;

                default:
                    std::cout << "未知 token 类型" << std::endl;
                    break;
            }
        }
    }

    const auto& errors = tokenizer.get_errors();
    if (!errors.empty()) {
        std::cout << "\n解析过程中发现 " << errors.size() << " 个错误:\n";
        for (const auto& error : errors) {
            std::cout << "位置 " << error.location.position << ": " << error.message << std::endl;
        }
    }

    system("pause");
    return 0;
}