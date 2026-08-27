<#
.SYNOPSIS
    Tự động kéo và biên dịch Mozilla NSS 2026 (TLS 1.3 & NSPR) cho Windows 10 Mobile (UWP ARM / x86).
#>

param(
    [string]$Platform = "ARM",
    [string]$Configuration = "Release",
    [string]$OutputDir = "$PSScriptRoot\..\src\FoxM.GoannaRuntime\libs\$Platform"
)

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [FoxM Core] Biên dịch Mozilla NSS 2026 TLS 1.3 cho UWP $Platform ($Configuration)" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$workDir = "$PSScriptRoot\..\build_nss_$Platform"
if (-not (Test-Path $workDir)) {
    New-Item -ItemType Directory -Path $workDir -Force | Out-Null
}

Push-Location $workDir

try {
    # 1. Kéo mã nguồn Mozilla NSS & NSPR từ mirror chính thức
    if (-not (Test-Path "nss")) {
        Write-Host "[1/4] Cloning Mozilla NSS repository..." -ForegroundColor Green
        & git clone --depth 1 https://github.com/nss-dev/nss.git nss
    }
    if (-not (Test-Path "nspr")) {
        Write-Host "[1/4] Cloning Mozilla NSPR repository..." -ForegroundColor Green
        & git clone --depth 1 https://github.com/nss-dev/nspr.git nspr
    }

    # 2. Tạo CMake build cho UWP
    $cmakeBuildDir = "$workDir\build"
    if (-not (Test-Path $cmakeBuildDir)) {
        New-Item -ItemType Directory -Path $cmakeBuildDir -Force | Out-Null
    }

    $arch = if ($Platform -eq "ARM") { "ARM" } elseif ($Platform -eq "x86") { "Win32" } else { "x64" }

    Write-Host "[2/4] Cấu hình CMake cho UWP $Platform..." -ForegroundColor Green
    & cmake -S nss -B $cmakeBuildDir `
        -G "Visual Studio 17 2022" `
        -A $arch `
        -DCMAKE_SYSTEM_NAME=WindowsStore `
        -DCMAKE_SYSTEM_VERSION=10.0 `
        -DNSS_DISABLE_GTESTS=ON `
        -DNSS_DISABLE_TESTS=ON `
        -DENABLE_WERROR=OFF `
        -DNSS_BUILD_WITHOUT_SOFTOKN=OFF

    # 3. Biên dịch bằng MSBuild
    Write-Host "[3/4] Đang biên dịch NSS C++ Core..." -ForegroundColor Green
    & cmake --build $cmakeBuildDir --config $Configuration --parallel 4

    # 4. Xuất các file .lib / .dll và headers vào dự án
    Write-Host "[4/4] Đang xuất thư viện vào $OutputDir..." -ForegroundColor Green
    Get-ChildItem -Path $cmakeBuildDir -Recurse -Include *.lib, *.dll | ForEach-Object {
        Copy-Item $_.FullName -Destination $OutputDir -Force
        Write-Host "   + Xuất: $($_.Name)" -ForegroundColor Cyan
    }

    Write-Host "`n[FoxM Core] Biên dịch Mozilla NSS 2026 TLS 1.3 THÀNH CÔNG!" -ForegroundColor Green
}
catch {
    Write-Host "[FoxM Core] Lỗi biên dịch: $_" -ForegroundColor Red
    throw $_
}
finally {
    Pop-Location
}
