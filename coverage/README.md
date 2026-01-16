# 覆盖率（Windows / OpenCppCoverage）

本目录提供 Windows 下的覆盖率生成脚本与报告打开脚本。覆盖率结果默认输出到 `coverage/report/`（已在 `.gitignore` 中忽略）。

> 说明：如果你之前用过旧脚本，可能会在 `coverage/` 下看到 `Modules/`、`third-party/`、`index.html` 等目录/文件。这些属于覆盖率报告产物，不建议提交，已在 `.gitignore` 中忽略。

## 前置条件

- 已安装 `OpenCppCoverage`，并确保 `OpenCppCoverage.exe` 可在终端直接运行（已加入 `PATH`）。
- 已安装 CMake，并可在终端运行 `cmake`。

## 生成覆盖率报告

在仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\coverage\run_coverage.ps1
```

默认会使用 `Debug` 配置构建并通过 `ctest` 运行测试，然后生成 HTML 报告：

- `coverage/report/index.html`
- `coverage/report/coverage.xml`（Cobertura 格式，方便做文件级汇总分析）

默认仅统计 `src/` 下实现文件的覆盖率（更贴近“代码是否被执行”的含义）。如需把 `include/hps/` 也纳入统计（可能会显著拉低覆盖率百分比），可使用：

```powershell
powershell -ExecutionPolicy Bypass -File .\coverage\run_coverage.ps1 -IncludeHeaders
```

如需指定构建配置：

```powershell
powershell -ExecutionPolicy Bypass -File .\coverage\run_coverage.ps1 -Config Release
```

如需清理旧脚本生成到 `coverage/` 根目录的遗留报告产物（可选）：

```powershell
powershell -ExecutionPolicy Bypass -File .\coverage\run_coverage.ps1 -CleanLegacy
```

## 打开报告

```powershell
powershell -ExecutionPolicy Bypass -File .\coverage\open_report.ps1
```
