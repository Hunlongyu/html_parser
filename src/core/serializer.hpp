#pragma once

#include <string>

namespace hps {

class Node;
class Element;

namespace detail {

/**
 * @brief 将单个节点及其子树序列化为 HTML，追加到 out。
 *
 * - Element：`<tag attr="...">children</tag>`，void 元素无闭合标签；
 * - Text：转义后的文本（实体感知，避免对已有字符引用二次转义）；
 * - Comment：`<!--...-->`；
 * - Document：等价于序列化其全部子节点。
 */
void serialize_node(const Node& node, std::string& out);

/**
 * @brief 序列化某节点的全部子节点（不含该节点自身标签）。
 */
void serialize_children(const Node& node, std::string& out);

/**
 * @brief 序列化元素的内部内容：raw text 元素（script/style 等）原样输出文本，
 *        其余元素按子节点序列化。
 */
void serialize_inner(const Element& element, std::string& out);

}  // namespace detail
}  // namespace hps
