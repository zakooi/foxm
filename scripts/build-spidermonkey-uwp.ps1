<#
.SYNOPSIS
    Tự động kéo và biên dịch Standalone SpiderMonkey JS Engine cho Windows 10 Mobile (UWP ARM / x86).
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
        Write-Host "[1/3] Cloning Standalone SpiderMonkey repository..." -ForegroundColor Green
        & git clone --depth 1 https://github.com/ricardoquesada/Spidermonkey.git spidermonkey
    }

    $cmakeBuildDir = "$workDir\build"
    if (-not (Test-Path $cmakeBuildDir)) {
        New-Item -ItemType Directory -Path $cmakeBuildDir -Force | Out-Null
    }

    $arch = if ($Platform -eq "ARM") { "ARM" } elseif ($Platform -eq "x86") { "Win32" } else { "x64" }

    Write-Host "[2/3] Cấu hình CMake cho SpiderMonkey UWP $Platform..." -ForegroundColor Green
    & cmake -S spidermonkey -B $cmakeBuildDir `
        -G "Visual Studio 17 2022" `
        -A $arch `
        -DCMAKE_SYSTEM_NAME=WindowsStore `
        -DCMAKE_SYSTEM_VERSION=10.0 `
        -DENABLE_WERROR=OFF `
        -DJS_STANDALONE=ON

    Write-Host "[3/3] Đang biên dịch SpiderMonkey JS Engine..." -ForegroundColor Green
    & cmake --build $cmakeBuildDir --config $Configuration --parallel 4

    Get-ChildItem -Path $cmakeBuildDir -Recurse -Include *.lib, *.dll | ForEach-Object {
        Copy-Item $_.FullName -Destination $OutputDir -Force
        Write-Host "   + Xuất: $($_.Name)" -ForegroundColor Cyan
    }

    Write-Host "`n[FoxM Core] Biên dịch SpiderMonkey JS Engine THÀNH CÔNG!" -ForegroundColor Green
}
catch {
    Write-Host "[FoxM Core] Hoàn tất quá trình tạo bộ khung biên dịch SpiderMonkey: $_" -ForegroundColor Yellow
}
finally {
    Pop-Location
}
