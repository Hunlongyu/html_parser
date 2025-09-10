#pragma once

// 核心模块
#include "hps/core/attribute.hpp"
#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/core/node.hpp"
#include "hps/core/text_node.hpp"

// 解析模块
#include "hps/parsing/html_parser.hpp"
#include "hps/parsing/token.hpp"
#include "hps/parsing/tokenizer.hpp"
#include "hps/parsing/tree_builder.hpp"

// 查询模块
#include "hps/query/element_query.hpp"
#include "hps/query/query.hpp"

// 错误处理模块
#include "hps/utils/exception.hpp"

#include "hps/version.hpp"

namespace hps {
std::unique_ptr<Document> parse(std::string_view html);

std::unique_ptr<Document> parse(std::string_view html, ErrorHandlingMode mode);

std::unique_ptr<Document> parse_file(std::string_view path);

std::unique_ptr<Document> parse_file(std::string_view path, ErrorHandlingMode mode);

std::string version();
}  // namespace hps