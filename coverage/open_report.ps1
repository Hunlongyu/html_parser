param(
    [string]$ReportDir = ""
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($ReportDir)) {
    $ReportDir = Join-Path $scriptDir "report"
}

$indexPath = Join-Path $ReportDir "index.html"
if (-not (Test-Path $indexPath)) {
    Write-Host "错误: 未找到覆盖率报告: $indexPath" -ForegroundColor Red
    exit 1
}

Start-Process $indexPath
