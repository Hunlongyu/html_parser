#pragma once

#include "hps/core/node.hpp"

#include <string>
#include <string_view>

namespace hps {

/**
 * @brief 文档类型声明节点（`<!DOCTYPE ...>`）
 *
 * 表示文档开头的 DOCTYPE 声明，承载其名称（如 "html"）以及可选的
 * PUBLIC / SYSTEM 标识符。通常作为 Document 的首个子节点出现。
 */
class DoctypeNode : public Node {
  public:
    /**
     * @brief 构造函数
     * @param name DOCTYPE 名称（缺失时为空串）
     * @param public_id PUBLIC 标识符（未声明时为空串）
     * @param system_id SYSTEM 标识符（未声明时为空串）
     * @param has_identifiers 是否声明了 PUBLIC 或 SYSTEM（用于区分“缺失”与“空串”）
     */
    DoctypeNode(
        std::string_view name,
        std::string_view public_id,
        std::string_view system_id,
        bool has_identifiers);

    ~DoctypeNode() override = default;

    /**
     * @brief 节点类型
     * @return NodeType::Doctype
     */
    [[nodiscard]] NodeType type() const noexcept override;

    /**
     * @brief DOCTYPE 名称
     * @return 名称（如 "html"），缺失时为空串
     */
    [[nodiscard]] const std::string& name() const noexcept;

    /**
     * @brief PUBLIC 标识符
     * @return PUBLIC 标识符；未声明时为空串
     */
    [[nodiscard]] const std::string& public_id() const noexcept;

    /**
     * @brief SYSTEM 标识符
     * @return SYSTEM 标识符；未声明时为空串
     */
    [[nodiscard]] const std::string& system_id() const noexcept;

    /**
     * @brief 是否声明了 PUBLIC 或 SYSTEM 标识符
     * @return true 表示声明了标识符（即便其内容为空串）
     */
    [[nodiscard]] bool has_identifiers() const noexcept;

    /**
     * @brief 面向文本提取时的内容
     * @return 空字符串（DOCTYPE 不贡献文本）
     */
    [[nodiscard]] std::string text_content() const override;

  private:
    std::string m_name;
    std::string m_public_id;
    std::string m_system_id;
    bool        m_has_identifiers;
};

}  // namespace hps
