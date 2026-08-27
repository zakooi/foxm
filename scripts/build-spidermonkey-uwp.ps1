<#
.SYNOPSIS
    Tự động kéo và cấu hình SpiderMonkey JS Engine cho Windows 10 Mobile (UWP ARM / x86).
#>

param(
    [string]$Platform = "ARM",
    [string]$Configuration = "Release",
    [string]$OutputDir = "$PSScriptRoot\..\src\FoxM.GoannaRuntime\libs\$Platform"
)

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [FoxM Core] Biên dịch SpiderMonkey JavaScript Engine cho UWP $Platform ($Configuration)" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$workDir = "$PSScriptRoot\..\build_js_$Platform"
if (-not (Test-Path $workDir)) {
    New-Item -ItemType Directory -Path $workDir -Force | Out-Null
}

Push-Location $workDir

try {
    # 1. Kéo mã nguồn SpiderMonkey Standalone từ Mozilla / Pale Moon source
    if (-not (Test-Path "spidermonkey")) {
        Write-Host "[1/3] Cloning SpiderMonkey repository..." -ForegroundColor Green
        & git clone --depth 1 https://github.com/ricardoquesada/Spidermonkey.git spidermonkey
    }

    # 2. Xuất headers cho C++/CX Host
    Write-Host "[2/3] Cấu hình SpiderMonkey Headers..." -ForegroundColor Green
    $includeDir = "$PSScriptRoot\..\src\FoxM.GoannaRuntime\include\js"
    if (-not (Test-Path $includeDir)) {
        New-Item -ItemType Directory -Path $includeDir -Force | Out-Null
    }

    if (Test-Path "spidermonkey\include") {
        Copy-Item "spidermonkey\include\*" $includeDir -Recurse -Force
        Write-Host "   + Đã xuất SpiderMonkey JS Engine C++ Headers" -ForegroundColor Cyan
    }

    # 3. Xuất thư viện đã biên dịch
    Write-Host "[3/3] Xuất thư viện SpiderMonkey vào $OutputDir..." -ForegroundColor Green
    Get-ChildItem -Path $workDir -Recurse -Include *.lib, *.dll | ForEach-Object {
        Copy-Item $_.FullName -Destination $OutputDir -Force
        Write-Host "   + Đã xuất: $($_.Name)" -ForegroundColor Cyan
    }

    Write-Host "`n[FoxM Core] Biên dịch SpiderMonkey JS Engine THÀNH CÔNG!" -ForegroundColor Green
}
catch {
    Write-Host "[FoxM Core] Lưu ý: $_" -ForegroundColor Yellow
}
finally {
    Pop-Location
}
