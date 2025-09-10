#pragma once

#include "hps/core/document.hpp"
#include "hps/core/element.hpp"
#include "hps/parsing/token.hpp"
#include "hps/utils/exception.hpp"
#include "hps/utils/noncopyable.hpp"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace hps {

// 解析错误信息
struct BuilderError {
    BuilderException::ErrorCode code;
    std::string                 message;
    size_t                      line;
    size_t                      column;

    BuilderError(const BuilderException::ErrorCode c, std::string msg, const size_t line, const size_t col) : code(c), message(std::move(msg)), line(line), column(col) {}
};

class TreeBuilder : public NonCopyable {
  public:
    /**
     * @brief 构造函数
     * @param document 目标文档对象
     */
    explicit TreeBuilder(Document* document);

    /**
     * @brief 析构函数
     */
    ~TreeBuilder() = default;

    /**
     * @brief 处理Token
     * @param token 要处理的Token
     * @return 是否成功处理
     */
    bool process_token(const Token& token);

    /**
     * @brief 完成树构建
     * @return 是否成功完成
     */
    bool finish();

    /**
     * @brief 获取构建的文档
     * @return 文档对象指针
     */
    Document* document() const noexcept;

    /**
     * @brief 获取错误列表
     * @return 解析错误列表
     */
    const std::vector<BuilderError>& errors() const noexcept;

  private:
    // Token处理方法
    void        process_start_tag(const Token& token);
    void        process_end_tag(const Token& token);
    void        process_text(const Token& token) const;
    void        process_done(const Token& token);

    // 元素操作方法
    static std::unique_ptr<Element> create_element(const Token& token);
    void                            insert_element(std::unique_ptr<Element> element) const;
    void                            insert_text(std::string_view text) const;

    // 栈操作方法
    void     push_element(Element* element);
    Element* pop_element();
    Element* current_element() const;

    // 简单的作用域检查
    bool should_close_element(std::string_view tag_name) const;
    void close_elements_until(std::string_view tag_name);

    // 错误处理
    void parse_error(BuilderException::ErrorCode code, const std::string& message);

    // 工具方法
    static bool is_void_element(std::string_view tag_name);
    static bool is_raw_text_element(std::string_view tag_name);

  private:
    Document*                 m_document;       // 目标文档
    std::vector<Element*>     m_element_stack;  // 元素栈
    std::vector<BuilderError> m_errors;         // 解析错误列表

    // 预定义的元素集合（简化版）
    static const std::unordered_set<std::string_view> s_void_elements;
    static const std::unordered_set<std::string_view> s_raw_text_elements;
};

}  // namespace hps