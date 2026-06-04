#pragma once

// 共享的 html5lib tree-construction 测试支撑：.dat 解析 + 树序列化（html5lib 转储格式）。
// 由 curated 基线测试与完整一致性测试共用。

#include "hps/hps.hpp"
#include "hps/core/comment_node.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/text_node.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace hps::test_support {

struct TreeConstructionCase {
    std::string description;
    std::string input;
    std::string expected_document;
    std::string fragment_context;
    bool        scripting_enabled{true};
};

enum class DatSection : std::uint8_t {
    None,
    Data,
    Errors,
    Document,
    DocumentFragment,
};

inline void append_section_line(std::string& section, const std::string& line) {
    if (!section.empty()) {
        section.push_back('\n');
    }
    section += line;
}

[[nodiscard]] inline std::string trim_trailing_blank_lines(std::string text) {
    while (!text.empty()) {
        const size_t line_start = text.rfind('\n');
        const size_t start      = line_start == std::string::npos ? 0 : line_start + 1;
        if (text.find_first_not_of(" \t", start) != std::string::npos) {
            break;
        }
        if (line_start == std::string::npos) {
            text.clear();
            break;
        }
        text.erase(line_start);
    }
    return text;
}

[[nodiscard]] inline std::string first_non_empty_line(std::string_view text) {
    std::istringstream stream{std::string(text)};
    std::string        line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            return line;
        }
    }
    return {};
}

[[nodiscard]] inline std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("Cannot open baseline file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

[[nodiscard]] inline std::vector<TreeConstructionCase> parse_tree_construction_cases(
    const std::filesystem::path& path) {
    const std::string  contents = read_text_file(path);
    std::istringstream stream(contents);

    std::vector<TreeConstructionCase> cases;
    TreeConstructionCase              current_case;
    DatSection                        current_section = DatSection::None;
    size_t                            case_index      = 0;

    auto flush_case = [&]() {
        if (current_case.input.empty() && current_case.expected_document.empty() &&
            current_case.fragment_context.empty()) {
            return;
        }
        current_case.expected_document =
            trim_trailing_blank_lines(std::move(current_case.expected_document));
        if (current_case.description.empty()) {
            current_case.description = path.filename().string() + "#" + std::to_string(case_index);
        }
        cases.push_back(std::move(current_case));
        current_case    = {};
        current_section = DatSection::None;
    };

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "#data") {
            flush_case();
            ++case_index;
            current_case.description = path.filename().string() + "#" + std::to_string(case_index);
            current_section          = DatSection::Data;
            continue;
        }
        if (line == "#errors") {
            current_section = DatSection::Errors;
            continue;
        }
        if (line == "#document") {
            current_section = DatSection::Document;
            continue;
        }
        if (line == "#document-fragment") {
            current_section = DatSection::DocumentFragment;
            continue;
        }
        if (line == "#script-on") {
            current_case.scripting_enabled = true;
            current_section                = DatSection::None;
            continue;
        }
        if (line == "#script-off") {
            current_case.scripting_enabled = false;
            current_section                = DatSection::None;
            continue;
        }

        switch (current_section) {
            case DatSection::Data:
                append_section_line(current_case.input, line);
                break;
            case DatSection::Document:
                append_section_line(current_case.expected_document, line);
                break;
            case DatSection::DocumentFragment:
                if (current_case.fragment_context.empty()) {
                    current_case.fragment_context = line;
                } else {
                    append_section_line(current_case.expected_document, line);
                }
                break;
            case DatSection::Errors:
            case DatSection::None:
                break;
        }
    }

    flush_case();
    return cases;
}

[[nodiscard]] inline std::vector<TreeConstructionCase> collect_cases_from_dir(
    const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            files.push_back(entry.path());
        }
    }
    std::ranges::sort(files);

    std::vector<TreeConstructionCase> cases;
    for (const auto& path : files) {
        auto file_cases = parse_tree_construction_cases(path);
        cases.insert(cases.end(),
                     std::make_move_iterator(file_cases.begin()),
                     std::make_move_iterator(file_cases.end()));
    }
    return cases;
}

[[nodiscard]] inline std::string escape_tree_text(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"':  escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:   escaped.push_back(ch); break;
        }
    }
    return escaped;
}

inline void serialize_node(const hps::Node& node, std::vector<std::string>& lines, const size_t depth) {
    const std::string indent(depth * 2, ' ');
    switch (node.type()) {
        case hps::NodeType::Document:
            for (auto child = node.first_child(); child; child = child->next_sibling()) {
                serialize_node(*child, lines, depth);
            }
            return;
        case hps::NodeType::Element: {
            const auto* element = node.as_element();
            lines.push_back("| " + indent + "<" + element->tag_name() + ">");
            for (const auto& attribute : element->attributes()) {
                lines.push_back("| " + std::string((depth + 1) * 2, ' ') + attribute.name() +
                                "=\"" + escape_tree_text(attribute.value()) + "\"");
            }
            for (auto child = element->first_child(); child; child = child->next_sibling()) {
                serialize_node(*child, lines, depth + 1);
            }
            return;
        }
        case hps::NodeType::Text:
            lines.push_back("| " + indent + "\"" + escape_tree_text(node.as_text()->value()) + "\"");
            return;
        case hps::NodeType::Comment:
            lines.push_back("| " + indent + "<!-- " + escape_tree_text(node.as_comment()->value()) + " -->");
            return;
        case hps::NodeType::Undefined:
            return;
    }
}

[[nodiscard]] inline std::string serialize_document_tree(const hps::Document& document) {
    std::vector<std::string> lines;
    serialize_node(document, lines, 0);

    std::ostringstream out;
    for (size_t index = 0; index < lines.size(); ++index) {
        if (index != 0) {
            out << '\n';
        }
        out << lines[index];
    }
    return out.str();
}

}  // namespace hps::test_support
