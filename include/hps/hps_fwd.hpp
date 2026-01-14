#pragma once
#include <cstdint>

namespace hps {

enum class NodeType : std::uint8_t {
    Undefined, /**< 未定义节点 */
    Element,   /**< 元素节点 <div> */
    Text,      /**< 文本节点 <p>Hello</p> 包括 <![CDATA[hello world]]> */
    Comment,   /**< <!-- 注释 --> */
    Document,  /**< 文档节点 整个文档的根节点 */
};

enum class TokenType : std::uint8_t {
    OPEN,          /// <tag ...>
    CLOSE,         /// </tag>
    CLOSE_SELF,    /// <tag ... />
    FORCE_QUIRKS,  /// 浏览器怪异模式触发
    TEXT,          /// Text
    COMMENT,       /// <!-- ... -->
    DOCTYPE,       /// <!DOCTYPE ...>
    DONE,          /// 输入结束 EOF
};

/**
 * @brief Tokenizer状态枚举
 *
 * 定义HTML词法分析器在解析过程中的各种状态，用于状态机驱动的解析流程。
 */
enum class TokenizerState : std::uint8_t {
    Data,        /// 普通文本
    TagOpen,     /// <          标签开始
    TagName,     /// <tag       标签名
    EndTagOpen,  /// </tag      结束标签开始
    EndTagName,  /// </tag      结束标签名称

    BeforeAttributeName,         /// <tag                   标签属性前的空格
    AttributeName,               /// <tag attr              属性名
    AfterAttributeName,          /// <tag attr              属性名后的空格
    BeforeAttributeValue,        /// <tag attr=             属性值前的空格
    AttributeValueDoubleQuoted,  /// <tag attr="value"      双引号属性值
    AttributeValueSingleQuoted,  /// <tag attr='value'      单引号属性值
    AttributeValueUnquoted,      /// <tag attr=value        无引号属性值
    SelfClosingStartTag,         /// <tag attr="value" />   自闭合标签

    DOCTYPE,     /// <!DOCTYPE html>    DOCTYPE
    Comment,     /// <!--comment-->     注释
    ScriptData,  /// 进入 <script> 标签后的原始文本状态
    RAWTEXT,     /// 进入 <style>、<noscript> 等标签后的原始文本状态
    RCDATA,      /// 进入 <textarea>、<title> 标签后的状态（可解析字符实体但不解析标签）
};

// 核心模块
class Attribute;
class Document;
class Element;
class Node;
class TextNode;
class CommentNode;

// 解析模块
class HTMLParser;
class Options;
class Token;
class Tokenizer;
class TreeBuilder;

// 查询模块
class Query;
class ElementQuery;

// 异常模块
class HPSException;
}  // namespace hps
