#include "hps/hps.hpp"

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
        const auto doc       = parse_file("./html/shodan.html", options);
        auto start = std::chrono::high_resolution_clock::now();
        // auto       links = doc->css(".hsxa-host a[href]").elements();
        auto links = doc->css(".result .heading a.text-danger").elements();
        std::cout << "links: " << links.size() << std::endl;
        for (const auto& link : links) {
            std::cout << "ip: " << link->get_attribute("href") << std::endl;
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