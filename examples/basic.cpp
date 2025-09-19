#include "hps/hps.hpp"

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
        // auto       links = doc->css(".hsxa-host a[href]").elements();
        auto links = doc->css(".result .heading a.text-danger").elements();
        std::cout << "links: " << links.size() << std::endl;
        for (const auto& link : links) {
            std::string ip = link->get_attribute("href");
            std::cout << "ip: " << ip << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    system("pause");
    return 0;
}