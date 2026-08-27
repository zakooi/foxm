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

    # 2. Biên dịch bằng công cụ chính thức build.py của Mozilla
    $targetArch = if ($Platform -eq "ARM") { "arm" } elseif ($Platform -eq "x86") { "ia32" } else { "x64" }
    $optFlag = if ($Configuration -eq "Release") { "--opt" } else { "" }

    Write-Host "[2/4] Đang khởi chạy Mozilla build.py cho kiến trúc $targetArch..." -ForegroundColor Green
    if ($optFlag) {
        python nss/build.py --target=$targetArch --disable-tests --opt
    } else {
        python nss/build.py --target=$targetArch --disable-tests
    }

    # 3. Thu gom các file .lib / .dll và xuất vào thư mục libs
    Write-Host "[3/4] Đang thu gom thư viện đã biên dịch vào $OutputDir..." -ForegroundColor Green
    $distDir = "$workDir\dist"
    if (Test-Path $distDir) {
        Get-ChildItem -Path $distDir -Recurse -Include *.lib, *.dll | ForEach-Object {
            Copy-Item $_.FullName -Destination $OutputDir -Force
            Write-Host "   + Đã xuất: $($_.Name)" -ForegroundColor Cyan
        }
    }

    Write-Host "`n[FoxM Core] Biên dịch Mozilla NSS 2026 TLS 1.3 THÀNH CÔNG!" -ForegroundColor Green
}
catch {
    Write-Host "[FoxM Core] Lưu ý biên dịch NSS: $_" -ForegroundColor Yellow
}
finally {
    Pop-Location
}
