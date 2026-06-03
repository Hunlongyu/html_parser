#pragma once

#include "hps/parsing/options.hpp"
#include "hps/utils/exception.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hps {

class Document;

/**
 * @brief HTML解析器类
 *
 * HTMLParser是HTML解析的协调器，负责整合词法分析器(Tokenizer)和语法分析器(TreeBuilder)，
 * 将HTML字符串解析为DOM文档树。支持完整HTML文档和文件解析，提供灵活的错误处理选项。
 */
class HTMLParser {
  public:
    /**
     * @brief 默认构造函数
     */
    HTMLParser() = default;

    /**
     * @brief 析构函数
     */
    ~HTMLParser() = default;

    HTMLParser(const HTMLParser&)            = delete;
    HTMLParser& operator=(const HTMLParser&) = delete;

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

    /**
     * @brief 解析 HTML 片段
     * @param html HTML 片段内容
     * @param context_tag 片段上下文标签名，例如 "div"、"table"、"textarea"
     * @param options 解析选项
     * @return 只包含片段节点的文档对象
     */
    [[nodiscard]] std::shared_ptr<Document> parse_fragment(
        std::string_view html,
        std::string_view context_tag,
        const Options& options = {});

    /**
     * @brief 解析 HTML 片段（右值引用优化）
     * @param html HTML 片段内容
     * @param context_tag 片段上下文标签名
     * @param options 解析选项
     * @return 只包含片段节点的文档对象
     */
    [[nodiscard]] std::shared_ptr<Document> parse_fragment(
        std::string&& html,
        std::string_view context_tag,
        const Options& options = {});

    // 原始字节解析功能（爬虫场景：输入编码未知，自动嗅探并转码为 UTF-8）
    /**
     * @brief 解析未知编码的原始字节（如 HTTP 响应体）
     *
     * 先确定输入编码——若 options.encoding 非空则以其为准，否则按
     * BOM → <meta charset> → UTF-8 启发式 的顺序嗅探——再用系统原生 codec
     * （Windows: MultiByteToWideChar；POSIX: iconv）转码为 UTF-8 后解析。
     * 当编码无法识别或不受支持时记录 UnsupportedEncoding 错误（严格模式下抛出）。
     *
     * @param bytes 原始字节
     * @param options 解析选项（可选）
     * @return 解析后的文档对象智能指针
     */
    [[nodiscard]] std::shared_ptr<Document> parse_bytes(std::string_view bytes, const Options& options = {});

    // 文件解析功能（扩展功能）
    /**
     * @brief 解析HTML文件
     *
     * 以二进制读取文件后，与 parse_bytes 一致地嗅探/转码为 UTF-8 再解析。
     *
     * @param file_path HTML文件路径
     * @param options 解析选项（可选，默认为宽松模式）
     * @return 解析后的文档对象智能指针
     */
    [[nodiscard]] std::shared_ptr<Document> parse_file(std::string_view file_path, const Options& options = {});

    // 错误信息访问（诊断功能）
    /**
     * @brief 获取解析过程中的错误列表
     * @return 错误列表的常量引用
     */
    [[nodiscard]] const std::vector<HPSError>& get_errors() const noexcept;

  private:
    std::vector<HPSError> m_errors;  ///< 解析错误列表

    [[nodiscard]] std::shared_ptr<Document> parse_owned(std::string html, const Options& options);
    [[nodiscard]] std::shared_ptr<Document> parse_fragment_owned(
        std::string html,
        std::string_view context_tag,
        const Options& options);
};

}  // namespace hps
