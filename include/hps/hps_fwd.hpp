#pragma once
#include <cstdint>

namespace hps {

/** @brief DOM 节点类型 */
enum class NodeType : std::uint8_t {
    Undefined, /**< 未定义节点 */
    Element,   /**< 元素节点 <div> */
    Text,      /**< 文本节点 <p>Hello</p> 包括 <![CDATA[hello world]]> */
    Comment,   /**< <!-- 注释 --> */
    Document,  /**< 文档节点 整个文档的根节点 */
    Doctype,   /**< 文档类型声明 <!DOCTYPE html> */
};

/** @brief 元素命名空间 */
enum class NamespaceKind : std::uint8_t {
    Html,    /**< HTML 命名空间 */
    Svg,     /**< SVG 命名空间 */
    MathML,  /**< MathML 命名空间 */
};

/**
 * @brief 驻留后的标签标识（整数化标签名）。
 *
 * 已知标签映射到一个稳定的小整数 id（= 内部主表下标 + 1），未知/自定义标签由
 * Document 在解析期动态分配 id（>= 已知标签数）。`Tag::Unknown` 表示尚未解析。
 * 建树与 CSS 匹配在热路径上以整数比较取代字符串比较——这是 lexbor 同款加速核心。
 * 取回名字用 Element::tag_name()（已知查静态表，自定义查 Document 驻留表）。
 */
enum class Tag : std::uint16_t {
    Unknown = 0,
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
    Plaintext,   /// 进入 <plaintext> 标签后的纯文本状态
    CDataSection, /// CDATA section state
};

// 核心模块
class Attribute;
class Document;
class Element;
class Node;
class TextNode;
class CommentNode;
class DoctypeNode;

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
