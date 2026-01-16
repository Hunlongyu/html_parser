#include "hps/hps.hpp"
#include "hps/parsing/html_parser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>

using namespace hps;
namespace fs = std::filesystem;

static std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    try {
        // 1. Determine file path
        std::string file_path;
        std::vector<std::string> search_paths = {
            "../examples/html/panlong.html",
            "../../examples/html/panlong.html",
            "examples/html/panlong.html",
            "../examples/html/base.html",
            "../../examples/html/base.html"
        };

        if (argc > 1) {
            file_path = argv[1];
        } else {
            // Try to find a default file
            for (const auto& p : search_paths) {
                if (fs::exists(p)) {
                    file_path = p;
                    break;
                }
            }
        }

        if (file_path.empty() || !fs::exists(file_path)) {
            std::cerr << "Error: Could not find input file. Please provide a path to an HTML file." << std::endl;
            std::cerr << "Usage: " << argv[0] << " [path/to/file.html]" << std::endl;
            return 1;
        }

        std::cout << "Benchmarking with file: " << file_path << std::endl;

        // 2. Load data into memory (exclude I/O from benchmark)
        std::string source = read_file(file_path);
        size_t data_size = source.length();
        double data_size_kb = data_size / 1024.0;
        
        std::cout << "File size: " << data_size << " bytes (~" << data_size_kb << " KB)" << std::endl;

        Options options = Options::performance();
        std::shared_ptr<Document> doc;

        // 3. Warmup & Parse Benchmark
        {
            std::cout << "\n--- Parsing Benchmark ---" << std::endl;
            std::cout << "Warming up..." << std::endl;
            {
                HTMLParser parser;
                doc = parser.parse(source, options);
            }

            constexpr int ITERATIONS = 100;
            std::vector<double> durations;
            
            std::cout << "Running parse benchmark (" << ITERATIONS << " iterations)..." << std::endl;

            HTMLParser parser;
            for (int i = 0; i < ITERATIONS; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                
                // Parse
                auto temp_doc = parser.parse(source, options);

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> ms = end - start;
                durations.push_back(ms.count());
            }

            double total_time = std::accumulate(durations.begin(), durations.end(), 0.0);
            double avg_time = total_time / ITERATIONS;
            double min_time = *std::min_element(durations.begin(), durations.end());
            double max_time = *std::max_element(durations.begin(), durations.end());
            double throughput = (static_cast<double>(data_size) / (1024.0 * 1024.0)) / (avg_time / 1000.0);

            std::cout << "Average Parse Time: " << avg_time << " ms" << std::endl;
            std::cout << "Min Parse Time:     " << min_time << " ms" << std::endl;
            std::cout << "Max Parse Time:     " << max_time << " ms" << std::endl;
            std::cout << "Throughput:         " << throughput << " MB/s" << std::endl;
        }

        // 4. CSS Selector Benchmark
        {
            std::cout << "\n--- CSS Selector Benchmark ---" << std::endl;
            // Ensure we have a valid document for CSS benchmarking
            if (!doc) {
                HTMLParser parser;
                doc = parser.parse(source, options);
            }

            std::vector<std::string> selectors = {
                ".grid .odd a",                  // Used in basic.cpp (active)
                ".hsxa-host a[href]",            // Used in basic.cpp (commented)
                ".result .heading a.text-danger", // Used in basic.cpp (commented)
                "div",                           // Common tag
                ".container"                     // Common class
            };

            constexpr int CSS_ITERATIONS = 100;

            for (const auto& selector : selectors) {
                std::vector<double> durations;
                size_t match_count = 0;

                // Warmup for this selector
                doc->css(selector).elements();

                for (int i = 0; i < CSS_ITERATIONS; ++i) {
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    auto elements = doc->css(selector).elements();
                    
                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> ms = end - start;
                    durations.push_back(ms.count());

                    if (i == 0) match_count = elements.size();
                }

                double total_time = std::accumulate(durations.begin(), durations.end(), 0.0);
                double avg_time = total_time / CSS_ITERATIONS;
                
                std::cout << "Selector: '" << selector << "'" << std::endl;
                std::cout << "  Matches: " << match_count << std::endl;
                std::cout << "  Avg Time: " << avg_time << " ms" << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
