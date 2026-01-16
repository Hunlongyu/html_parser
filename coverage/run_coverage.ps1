param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [string]$BuildDir = "",
    [string]$ReportDir = "",

    [switch]$IncludeHeaders,
    [switch]$CleanLegacy
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $projectRoot "build_coverage"
}

if ([string]::IsNullOrWhiteSpace($ReportDir)) {
    $ReportDir = Join-Path $scriptDir "report"
}

if ($CleanLegacy) {
    if (Test-Path (Join-Path $scriptDir "Modules")) {
        Remove-Item -Recurse -Force (Join-Path $scriptDir "Modules")
    }
    if (Test-Path (Join-Path $scriptDir "third-party")) {
        Remove-Item -Recurse -Force (Join-Path $scriptDir "third-party")
    }
    if (Test-Path (Join-Path $scriptDir "index.html")) {
        Remove-Item -Force (Join-Path $scriptDir "index.html")
    }
}

if (-not (Get-Command OpenCppCoverage -ErrorAction SilentlyContinue)) {
    Write-Host "错误: 未找到 OpenCppCoverage 工具。" -ForegroundColor Red
    Write-Host "请先安装 OpenCppCoverage，并确保已加入 PATH 环境变量。" -ForegroundColor Red
    exit 1
}

cmake -S $projectRoot -B $BuildDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake 配置失败。" -ForegroundColor Red
    exit 1
}

cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败。" -ForegroundColor Red
    exit 1
}

if (Test-Path $ReportDir) {
    Remove-Item -Recurse -Force $ReportDir
}
New-Item -ItemType Directory -Path $ReportDir | Out-Null

Push-Location $BuildDir
$sourceRoots = @(
    (Join-Path $projectRoot "src")
)
if ($IncludeHeaders) {
    $sourceRoots += (Join-Path $projectRoot "include\\hps")
}
$sourceArgs = @()
foreach ($sourceRoot in $sourceRoots) {
    $sourceArgs += @("--sources", "$sourceRoot")
}

OpenCppCoverage @sourceArgs `
    --excluded_sources "$BuildDir" `
    --excluded_sources "googletest" `
    --export_type "cobertura:$ReportDir\\coverage.xml" `
    --export_type "html:$ReportDir" `
    --cover_children `
    -- ctest -C $Config
$exitCode = $LASTEXITCODE
Pop-Location

if ($exitCode -ne 0) {
    exit $exitCode
}

$indexPath = Join-Path $ReportDir "index.html"
Write-Host "覆盖率报告已生成: $indexPath" -ForegroundColor Green
