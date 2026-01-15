# Check for OpenCppCoverage
if (-not (Get-Command OpenCppCoverage -ErrorAction SilentlyContinue)) {
    Write-Host "错误: 未找到 OpenCppCoverage 工具。" -ForegroundColor Red
    Write-Host "请先安装 OpenCppCoverage: https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
    Write-Host "安装后请确保将其添加到系统 PATH 环境变量中，然后重新运行此脚本。"
    exit 1
}

# Ensure build directory exists and is configured
if (-not (Test-Path "build")) {
    Write-Host "正在配置项目..."
    cmake -B build
}

# Build the project
Write-Host "正在编译项目..."
cmake --build build --config Debug

if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败。" -ForegroundColor Red
    exit 1
}

# Run Coverage
Write-Host "正在运行覆盖率测试..."
$projectRoot = (Get-Location).Path
$coverageDir = Join-Path $projectRoot "coverage"

if (Test-Path $coverageDir) {
    Remove-Item -Recurse -Force $coverageDir
}
New-Item -ItemType Directory -Path $coverageDir | Out-Null

Push-Location build

# --sources: Only show coverage for files in the project root
# --excluded_sources: Exclude tests, build, and external deps
# --cover_children: Track processes spawned by CTest
OpenCppCoverage --sources "$projectRoot" `
                --excluded_sources "$projectRoot\tests" `
                --excluded_sources "$projectRoot\build" `
                --excluded_sources "googletest" `
                --export_type "html:$coverageDir" `
                --cover_children `
                -- ctest -C Debug

Pop-Location

if ($LASTEXITCODE -eq 0) {
    Write-Host "覆盖率报告已生成: $coverageDir\index.html" -ForegroundColor Green
}
