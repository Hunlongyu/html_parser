// CPU/allocation profiling probe for the parse pipeline.
// Splits time + heap allocations into tokenizer vs tree-builder, and buckets
// remaining allocations by size to see whether they are string-sized.
#include "benchmark_common.hpp"
#include "hps/core/document.hpp"
#include "hps/core/node.hpp"
#include "hps/parsing/html_parser.hpp"
#include "hps/parsing/options.hpp"
#include "hps/parsing/token.hpp"
#include "hps/parsing/tokenizer.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

// ----------------------------- allocation counters -----------------------------
namespace {
struct Counters {
    long long count = 0, bytes = 0;
    long long le16 = 0, le32 = 0, le64 = 0, le256 = 0, gt256 = 0;
};
Counters g_c;
bool     g_on = false;

inline void record(std::size_t n) {
    if (!g_on) return;
    g_c.count++;
    g_c.bytes += static_cast<long long>(n);
    if (n <= 16) g_c.le16++;
    else if (n <= 32) g_c.le32++;
    else if (n <= 64) g_c.le64++;
    else if (n <= 256) g_c.le256++;
    else g_c.gt256++;
}
void reset() { g_c = Counters{}; }
}  // namespace

void* operator new(std::size_t n) { record(n); void* p = std::malloc(n ? n : 1); if (!p) throw std::bad_alloc(); return p; }
void* operator new[](std::size_t n) { record(n); void* p = std::malloc(n ? n : 1); if (!p) throw std::bad_alloc(); return p; }
void  operator delete(void* p) noexcept { std::free(p); }
void  operator delete[](void* p) noexcept { std::free(p); }
void  operator delete(void* p, std::size_t) noexcept { std::free(p); }
void  operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using namespace hps;

std::size_t count_nodes(const Node& node) {
    std::size_t total = 1;
    for (auto child = node.first_child(); child; child = child->next_sibling()) total += count_nodes(*child);
    return total;
}

double min_ms(const std::vector<double>& v) {
    double m = v.front();
    for (double x : v) m = x < m ? x : m;
    return m;
}
}  // namespace

int main() {
    const auto files = bench::example_html_files();
    const Options opts = Options::performance();

    std::printf("%-14s %10s %8s | %10s %10s %10s | %10s %10s %10s %8s | %s\n",
                "file", "bytes", "nodes",
                "full_ms", "tok_ms", "build_ms",
                "full_allc", "tok_allc", "build_allc", "a/node", "size-buckets(full): <=16/<=32/<=64/<=256/>256");

    for (const auto& path : files) {
        const std::string src = bench::read_binary_file(path);
        const int iters = bench::recommended_iterations(src.size());

        // node count
        HTMLParser np;
        const auto doc0 = np.parse(src, opts);
        const std::size_t nodes = doc0 ? count_nodes(*doc0) : 0;

        // ---- timing: full parse ----
        std::vector<double> full_t;
        full_t.reserve(iters);
        HTMLParser fp;
        for (int i = 0; i < iters; ++i) {
            auto s = std::chrono::steady_clock::now();
            auto d = fp.parse(src, opts);
            auto e = std::chrono::steady_clock::now();
            if (!d) return 1;
            full_t.push_back(std::chrono::duration<double, std::milli>(e - s).count());
        }

        // ---- timing: tokenize only (next_token loop, mirrors parser feed) ----
        std::vector<double> tok_t;
        tok_t.reserve(iters);
        for (int i = 0; i < iters; ++i) {
            auto s = std::chrono::steady_clock::now();
            Tokenizer tk(src, opts);
            while (true) {
                auto t = tk.next_token();
                if (!t.has_value() || t->is_done()) break;
            }
            auto e = std::chrono::steady_clock::now();
            tok_t.push_back(std::chrono::duration<double, std::milli>(e - s).count());
        }

        // ---- allocations: full parse (count during parse only) ----
        reset();
        {
            HTMLParser p;
            g_on = true;
            auto d = p.parse(src, opts);
            g_on = false;
            (void)d;  // measured before destruction
        }
        const Counters full_a = g_c;

        // ---- allocations: tokenize only ----
        reset();
        {
            g_on = true;
            Tokenizer tk(src, opts);
            while (true) {
                auto t = tk.next_token();
                if (!t.has_value() || t->is_done()) break;
            }
            g_on = false;
        }
        const Counters tok_a = g_c;

        const double fm = min_ms(full_t), tm = min_ms(tok_t);
        const double anode = nodes ? static_cast<double>(full_a.count) / static_cast<double>(nodes) : 0.0;

        std::printf("%-14s %10zu %8zu | %10.4f %10.4f %10.4f | %10lld %10lld %10lld %8.2f | %lld/%lld/%lld/%lld/%lld\n",
                    path.filename().string().c_str(), src.size(), nodes,
                    fm, tm, fm - tm,
                    full_a.count, tok_a.count, full_a.count - tok_a.count, anode,
                    full_a.le16, full_a.le32, full_a.le64, full_a.le256, full_a.gt256);
    }
    return 0;
}
