# 贡献指南 / Contributing

感谢你对 HPS 的关注！欢迎提交 Issue 和 Pull Request。

## 提交 Issue

- 🐛 **Bug**：请附上能复现的最小 HTML 输入、期望输出、实际输出，以及平台/编译器版本。
- 💡 **功能请求**：到 [Discussions](https://github.com/Hunlongyu/html_parser/discussions) 讨论。

## 本地开发

```bash
git clone https://github.com/Hunlongyu/html_parser.git
cd html_parser
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
```

- 系统要求：C++20 编译器、CMake 3.28+。
- 库本体零运行时第三方依赖；测试使用 GoogleTest（CMake 自动获取）。
- 一致性套件（html5lib-tests）默认开启，首次配置会联网拉取；可用 `-DHPS_HTML5LIB_CONFORMANCE=OFF` 关闭。

## 代码规范

- 遵循仓库内的 `.clang-format` 与 `.clang-tidy`（correctness / modernize / performance）。
- 公共 API 一律 `snake_case`（如 `query_selector`，非 `querySelector`）。
- 公共头文件不得包含内部头或平台头。
- 提交信息遵循 [Conventional Commits](https://www.conventionalcommits.org/)（`feat:` / `fix:` / `refactor:` / `test:` / `docs:`）。

## Pull Request 要求

1. **测试必须全绿**：`ctest --preset dev-debug` 全部通过。
2. **新行为要带测试**：解析相关改动尽量补一条 `tests/baselines/html5lib/tree-construction/*.dat` 用例或 gtest。
3. **不要回退一致性**：改动 tree builder / tokenizer 前后对比 html5lib 一致性通过数，不应下降。
4. 保持每个 PR 聚焦单一关注点，附简要说明与影响范围。

## 许可证

提交即表示你同意你的贡献以本项目的 [MIT 许可证](LICENSE) 发布。
