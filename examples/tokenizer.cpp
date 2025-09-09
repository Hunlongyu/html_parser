#include "hps/parsing/tokenizer.hpp"

#include <iostream>

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    std::string_view html = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>HTML 示例</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f9f9f9;
        }
        .container {
            margin: 20px;
            padding: 15px;
            border: 1px solid #ccc;
        }
        .title {
            color: #333;
        }
    </style>
    <script src="/static/js/main.js" async></script>
</head>
<body class="main" id="page-top" style="background-color: #fff;">
    <div class="container" id="content">
        <h1 class="title">欢迎来到我的网页</h1>
        <p>
            这是一个 <strong>标准的 HTML 页面</strong>，用于演示无错误 HTML 的解析过程。
        </p>
        <p>
            支持中文字符：<br>
            &lt;div&gt; 标签，<b>粗体</b>，<i>斜体</i>，以及一些 <u>带下划线的文本</u>。
        </p>
        <ul class="menu">
            <li><a href="/">首页</a></li>
            <li><a href="/about" target="_blank" rel="noopener">关于我们</a></li>
            <li><a href="/contact">联系方式</a></li>
        </ul>
        <div id="section">
            <h2>脚本部分</h2>
            <script type="application/javascript">
                console.log("这是嵌入的 JavaScript");
                for (let i = 0; i < 10; i++) {
                    // Loop 简单脚本
                }
            </script>
        </div>
        <footer class="page-footer" style="text-align: center; padding: 10px;">
            &copy; 2025 MyWebsite. All rights reserved.
        </footer>
    </div>
</body>
</html>
)";

    std::string_view html2 = R"(
<!DOCTYPE html>
<html lang="zh-CN">
 <head> 
 </head>
<body>
 <div>
  <a href="https://example.com">Example</a>
 </div>
</body>
</html>
)";

    hps::Tokenizer   tokenizer(html, hps::ErrorHandlingMode::Lenient);

    std::cout << "输入HTML: " << html << std::endl << std::endl;

    while (tokenizer.has_more()) {
        if (auto token = tokenizer.next_token()) {
            switch (token->type()) {
                case hps::TokenType::OPEN:
                    std::cout << "开始标签: " << token->name();
                    if (!token->attrs().empty()) {
                        std::cout << ", 属性: ";
                        for (const auto& attr : token->attrs()) {
                            std::cout << attr.m_name << "=\"" << attr.m_value << "\" ";
                        }
                    }
                    std::cout << std::endl;
                    break;

                case hps::TokenType::CLOSE:
                    std::cout << "结束标签: " << token->name() << std::endl;
                    break;

                case hps::TokenType::CLOSE_SELF:
                    std::cout << "自闭合标签: " << token->name();
                    if (!token->attrs().empty()) {
                        std::cout << ", 属性: ";
                        for (const auto& attr : token->attrs()) {
                            std::cout << attr.m_name << "=\"" << attr.m_value << "\" ";
                        }
                    }
                    std::cout << std::endl;
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
            std::cout << "位置 " << error.position << ": " << error.message << std::endl;
        }
    }
    return 0;
}