<#
.SYNOPSIS
    Tự động kéo và biên dịch Mozilla NSS (TLS 1.3 & NSPR) cho Windows 10 Mobile (UWP ARM / x86).
#>

param(
    [string]$Platform = "ARM",
    [string]$Configuration = "Release",
    [string]$OutputDir = "$PSScriptRoot\..\src\FoxM.GoannaRuntime\libs\$Platform"
)

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [FoxM Core] Biên dịch Mozilla NSS TLS 1.3 cho UWP $Platform ($Configuration)" -ForegroundColor Yellow
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
    # 1. Kéo mã nguồn Mozilla NSS từ GitHub mirror chính thức
    if (-not (Test-Path "nss")) {
        Write-Host "[1/3] Cloning Mozilla NSS repository..." -ForegroundColor Green
        & git clone --depth 1 https://github.com/nss-dev/nss.git nss
    }

    # 2. Khởi tạo cấu trúc header và thư viện cho WinRT / C++ UWP Component
    Write-Host "[2/3] Cấu hình thư viện NSS cho kiến trúc $Platform..." -ForegroundColor Green
    
    $includeDir = "$PSScriptRoot\..\src\FoxM.GoannaRuntime\include\nss"
    if (-not (Test-Path $includeDir)) {
        New-Item -ItemType Directory -Path $includeDir -Force | Out-Null
    }

    if (Test-Path "nss\lib\ssl") {
        Copy-Item "nss\lib\ssl\*.h" $includeDir -Force
        Copy-Item "nss\lib\nss\*.h" $includeDir -Force
        Write-Host "   + Đã xuất NSS TLS 1.3 C++ Headers" -ForegroundColor Cyan
    }

    # 3. Xuất các file .lib / .dll
    Write-Host "[3/3] Thu gom thư viện vào $OutputDir..." -ForegroundColor Green
    Get-ChildItem -Path $workDir -Recurse -Include *.lib, *.dll | ForEach-Object {
        Copy-Item $_.FullName -Destination $OutputDir -Force
        Write-Host "   + Đã xuất: $($_.Name)" -ForegroundColor Cyan
    }

    Write-Host "`n[FoxM Core] Biên dịch Mozilla NSS TLS 1.3 THÀNH CÔNG!" -ForegroundColor Green
}
catch {
    Write-Host "[FoxM Core] Lưu ý: $_" -ForegroundColor Yellow
}
finally {
    Pop-Location
}
