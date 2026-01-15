#pragma once

#include "hps/parsing/options.hpp"
#include "hps/parsing/tokenizer.hpp"
#include "hps/parsing/tree_builder.hpp"

namespace hps {

/**
 * @brief HTML解析器类
 *
 * HTMLParser是HTML解析的协调器，负责整合词法分析器(Tokenizer)和语法分析器(TreeBuilder)，
 * 将HTML字符串解析为DOM文档树。支持完整HTML文档和文件解析，提供灵活的错误处理选项。
 */
class HTMLParser : public NonCopyable {
  public:
    /**
     * @brief 默认构造函数
     */
    HTMLParser() = default;

    /**
     * @brief 析构函数
     */
    ~HTMLParser() = default;

    // 核心解析功能（最重要的基础功能）
    /**
     * @brief 解析HTML字符串
     * @param html HTML字符串视图
     * @param options 解析选项（可选，默认为宽松模式）
     * @return 解析后的文档对象智能指针
     */
    [[nodiscard]] std::shared_ptr<Document> parse(std::string_view html, const Options& options = {});

    /**
     * @brief 解析HTML字符串（右值引用优化）
     * @param html HTML字符串（右值）
     * @param options 解析选项（可选，默认为宽松模式）
     * @return 解析后的文档对象智能指针
     */
    [[nodiscard]] std::shared_ptr<Document> parse(std::string&& html, const Options& options = {});

    // 文件解析功能（扩展功能）
    /**
     * @brief 解析HTML文件
     * @param filePath HTML文件路径
     * @param options 解析选项（可选，默认为宽松模式）
     * @return 解析后的文档对象智能指针
     */
    [[nodiscard]] std::shared_ptr<Document> parse_file(std::string_view filePath, const Options& options = {});

    // 错误信息访问（诊断功能）
    /**
     * @brief 获取解析过程中的错误列表
     * @return 错误列表的常量引用
     */
    [[nodiscard]] const std::vector<HPSError>& get_errors() const noexcept;

  private:
    std::vector<HPSError> m_errors;  ///< 解析错误列表

    [[nodiscard]] std::shared_ptr<Document> parse_owned(std::string html, const Options& options);
};

}  // namespace hps
