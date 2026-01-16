#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"
#include "hps/hps.hpp"
#include "hps/parsing/options.hpp"
#include "hps/utils/string_utils.hpp"

#include <chrono>
#include <iostream>
using namespace hps;

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        Options options;
        options.comment_mode = CommentMode::ProcessOnly;
        const auto doc       = parse_file("./html/panlong.html", options);
        auto       start     = std::chrono::high_resolution_clock::now();
        std::cout << "charset: " << doc->charset() << std::endl;
        // auto       links = doc->css(".hsxa-host a[href]").elements();
        // auto links = doc->css(".result .heading a.text-danger").elements();
        auto titles = doc->css(".grid .odd a").elements();
        std::cout << "titles: " << titles.size() << std::endl;
        for (const auto& title : titles) {
            const auto tl  = title->as_text()->text_content();
            const auto res = gbk_to_utf8(tl);
            std::cout << "title: " << res << std::endl;
        }
        /* doc->css(".heading a.text-danger").each([](const Element& ele) { std::cout << "ip: " << ele.get_attribute("href") << std::endl; });*/
        auto end      = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Total time: " << duration.count() << " microseconds" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    system("pause");
    return 0;
}