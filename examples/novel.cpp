#include "hps/hps.hpp"
#include "hps/utils/string_utils.hpp"

#include <chrono>
#include <iostream>
using namespace hps;

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    try {
        const auto res   = parse_file_with_error("./html/crxs.htm");
        const auto start = std::chrono::high_resolution_clock::now();

        const auto contents = res.document->css(".fiction-body p").elements();

        for (const auto& content : contents) {
            std::cout << "    " << content->text_content() << std::endl;
        }

        const auto end      = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Total time: " << duration.count() << " microseconds" << std::endl;

        if (res.has_errors()) {
            std::cout << "Errors count:" << res.error_count() << std::endl;
            for (const auto& error : res.errors) {
                std::cout << error.message << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    system("pause");
    return 0;
}