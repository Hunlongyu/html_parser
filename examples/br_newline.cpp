#include "hps/hps.hpp"

#include <iostream>

int main() {
    using namespace hps;

    const std::string html = R"HTML(
        <html>
          <body>
            <p>line1<br>line2<br/>line3</p>
          </body>
        </html>
    )HTML";

    Options opts;
    opts.br_handling = BRHandling::InsertCustom;  // 将 <br> 插入为 "\n"
    opts.br_text     = "$&$&";

    auto doc = parse(html, opts);

    // 输出整个文档文本内容，期望看到换行
    std::cout << "Document text_content:\n";
    std::cout << doc->text_content() << std::endl;

    // 也可验证 p 的文本内容
    auto ps = doc->get_elements_by_tag_name("p");
    if (!ps.empty()) {
        std::cout << "\n<p> text_content:\n";
        std::cout << ps[0]->text_content() << std::endl;
    }

#ifdef _WIN32
    system("pause");
#endif

    return 0;
}