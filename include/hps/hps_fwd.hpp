#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

enum class NodeType {
    Undefined, /**< 未定义节点 */
    Element,   /**< 元素节点 <div> */
    Text,      /**< 文本节点 <p>Hello</p> 包括 <![CDATA[hello world]]> */
    Comment,   /**< <!-- 注释 --> */
    Document,  /**< 文档节点 整个文档的根节点 */
};

enum class TokenType {
    OPEN,          /// <tag ...>
    CLOSE,         /// </tag>
    CLOSE_SELF,    /// <tag ... />
    FORCE_QUIRKS,  /// 浏览器怪异模式触发
    TEXT,          /// Text
    COMMENT,       /// <!-- ... -->
    DOCTYPE,       /// <!DOCTYPE ...>
    DONE,          /// 输入结束 EOF
};

// 核心模块
class Attribute;
class Document;
class Element;
class Node;
class TextNode;

// 解析模块
class HTMLParser;
class ParserOptions;
class Token;
class Tokenizer;
class TreeBuilder;

// 查询模块
class Query;
class ElementQuery;

// 异常模块
class HPSException;
}  // namespace hps