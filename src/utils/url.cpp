#include "hps/utils/url.hpp"

#include "hps/utils/string_utils.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace hps {
namespace {

// RFC 3986 §3 的五个组成部分；scheme/authority/query/fragment 用 optional 表示“是否出现”，
// path 始终存在（可能为空）。视图均指向被解析的源字符串。
struct ParsedUrl {
    std::optional<std::string_view> scheme;
    std::optional<std::string_view> authority;
    std::string_view                path;
    std::optional<std::string_view> query;
    std::optional<std::string_view> fragment;
};

[[nodiscard]] bool is_scheme_start(const char ch) {
    return is_alpha(ch);
}

[[nodiscard]] bool is_scheme_char(const char ch) {
    return is_alnum(ch) || ch == '+' || ch == '-' || ch == '.';
}

// 按 RFC 3986 Appendix B 拆分 URL 各部分。
[[nodiscard]] ParsedUrl parse_url(std::string_view url) {
    ParsedUrl out;

    if (const auto hash = url.find('#'); hash != std::string_view::npos) {
        out.fragment = url.substr(hash + 1);
        url          = url.substr(0, hash);
    }
    if (const auto question = url.find('?'); question != std::string_view::npos) {
        out.query = url.substr(question + 1);
        url       = url.substr(0, question);
    }

    // scheme：首字符为字母，其后为 scheme 字符，直到 ':'，且冒号前不含 '/'。
    if (const auto colon = url.find(':'); colon != std::string_view::npos && colon > 0) {
        bool valid = is_scheme_start(url[0]);
        for (size_t i = 1; i < colon && valid; ++i) {
            valid = is_scheme_char(url[i]);
        }
        if (valid && url.substr(0, colon).find('/') == std::string_view::npos) {
            out.scheme = url.substr(0, colon);
            url        = url.substr(colon + 1);
        }
    }

    // authority：以 "//" 起始，直到下一个 '/'（query/fragment 已剥离）。
    if (url.size() >= 2 && url[0] == '/' && url[1] == '/') {
        url.remove_prefix(2);
        if (const auto slash = url.find('/'); slash == std::string_view::npos) {
            out.authority = url;
            url           = {};
        } else {
            out.authority = url.substr(0, slash);
            url           = url.substr(slash);
        }
    }

    out.path = url;
    return out;
}

// RFC 3986 §5.2.4 remove_dot_segments。
[[nodiscard]] std::string remove_dot_segments(std::string_view input_view) {
    std::string input(input_view);
    std::string output;
    output.reserve(input.size());

    const auto drop_last_segment = [&output]() {
        if (const auto pos = output.find_last_of('/'); pos == std::string::npos) {
            output.clear();
        } else {
            output.erase(pos);
        }
    };

    while (!input.empty()) {
        if (input.starts_with("../")) {
            input.erase(0, 3);
        } else if (input.starts_with("./")) {
            input.erase(0, 2);
        } else if (input.starts_with("/./")) {
            input.replace(0, 3, "/");
        } else if (input == "/.") {
            input = "/";
        } else if (input.starts_with("/../")) {
            input.replace(0, 4, "/");
            drop_last_segment();
        } else if (input == "/..") {
            input = "/";
            drop_last_segment();
        } else if (input == "." || input == "..") {
            input.clear();
        } else {
            const size_t start = (input.front() == '/') ? 1 : 0;
            const size_t next  = input.find('/', start);
            if (next == std::string::npos) {
                output += input;
                input.clear();
            } else {
                output += input.substr(0, next);
                input.erase(0, next);
            }
        }
    }
    return output;
}

// RFC 3986 §5.3 merge：将相对路径并入 base 路径。
[[nodiscard]] std::string merge_paths(const ParsedUrl& base, std::string_view ref_path) {
    if (base.authority.has_value() && base.path.empty()) {
        std::string merged("/");
        merged += ref_path;
        return merged;
    }
    if (const auto slash = base.path.rfind('/'); slash != std::string_view::npos) {
        std::string merged(base.path.substr(0, slash + 1));
        merged += ref_path;
        return merged;
    }
    return std::string(ref_path);
}

[[nodiscard]] std::string recompose(
    const std::optional<std::string_view>& scheme,
    const std::optional<std::string_view>& authority,
    std::string_view                       path,
    const std::optional<std::string_view>& query,
    const std::optional<std::string_view>& fragment) {
    std::string out;
    if (scheme.has_value()) {
        out += *scheme;
        out += ':';
    }
    if (authority.has_value()) {
        out += "//";
        out += *authority;
    }
    out += path;
    if (query.has_value()) {
        out += '?';
        out += *query;
    }
    if (fragment.has_value()) {
        out += '#';
        out += *fragment;
    }
    return out;
}

}  // namespace

std::string resolve_url(const std::string_view base, const std::string_view reference) {
    if (base.empty()) {
        return std::string(reference);  // 无 base 无法解析，原样返回
    }

    const ParsedUrl b = parse_url(base);
    const ParsedUrl r = parse_url(reference);

    std::optional<std::string_view> t_scheme;
    std::optional<std::string_view> t_authority;
    std::string                     t_path;
    std::optional<std::string_view> t_query;

    if (r.scheme.has_value()) {
        t_scheme    = r.scheme;
        t_authority = r.authority;
        t_path      = remove_dot_segments(r.path);
        t_query     = r.query;
    } else {
        if (r.authority.has_value()) {
            t_authority = r.authority;
            t_path      = remove_dot_segments(r.path);
            t_query     = r.query;
        } else {
            if (r.path.empty()) {
                t_path  = std::string(b.path);
                t_query = r.query.has_value() ? r.query : b.query;
            } else {
                if (r.path.front() == '/') {
                    t_path = remove_dot_segments(r.path);
                } else {
                    t_path = remove_dot_segments(merge_paths(b, r.path));
                }
                t_query = r.query;
            }
            t_authority = b.authority;
        }
        t_scheme = b.scheme;
    }

    return recompose(t_scheme, t_authority, t_path, t_query, r.fragment);
}

bool is_absolute_url(const std::string_view url) {
    return parse_url(url).scheme.has_value();
}

}  // namespace hps
