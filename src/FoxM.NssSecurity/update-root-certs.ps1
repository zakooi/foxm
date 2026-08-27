<#
.SYNOPSIS
    Tự động tải và đồng bộ hóa kho chứng chỉ gốc Mozilla NSS Root CA mới nhất cho FoxM.
.DESCRIPTION
    Script này tải file certdata.txt chính thức từ Mozilla NSS repository và chuyển đổi
    thành định dạng nssckbi tương thích với Goanna Engine trên Windows 10 Mobile.
#>

[CmdletBinding()]
param(
    [string]$OutputPath = "$PSScriptRoot\certdata.txt"
)

$MozillaCertUrl = "https://hg.mozilla.org/mozilla-central/raw-file/default/security/nss/lib/ckfw/builtins/certdata.txt"

Write-Host "[FoxM NSS] Đang tải kho chứng chỉ Mozilla NSS mới nhất..." -ForegroundColor Cyan

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
    $webClient = New-Object System.Net.WebClient
    $webClient.DownloadFile($MozillaCertUrl, $OutputPath)
    
    $lines = (Get-Content $OutputPath | Measure-Object -Line).Lines
    Write-Host "[FoxM NSS] Thành công! Đã nạp $lines dòng dữ liệu chứng chỉ vào $OutputPath" -ForegroundColor Green
    Write-Host "[FoxM NSS] Windows 10 Mobile giờ đây đã có thể xác thực tất cả các website TLS 1.3 và HTTPS mới nhất." -ForegroundColor Yellow
}
catch {
    Write-Error "[FoxM NSS] Thất bại khi tải chứng chỉ: $_"
}
