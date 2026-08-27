<#
.SYNOPSIS
    Tạo chứng chỉ ký số tự ký (Self-signed Certificate) thời hạn 5 năm để ký gói .appx sideload cho Windows 10 Mobile.
#>

param(
    [string]$CertPath = "$PSScriptRoot\FoxM_SigningCert.pfx",
    [string]$Password = "FoxM2026Password!"
)

Write-Host "[FoxM Sign] Đang tạo chứng chỉ tự ký thời hạn 5 năm cho Publisher CN=FoxMDev..." -ForegroundColor Cyan

$securePassword = ConvertTo-SecureString -String $Password -Force -AsPlainText
$notAfterDate = (Get-Date).AddYears(5)

$cert = New-SelfSignedCertificate `
    -Type Custom `
    -Subject "CN=FoxMDev" `
    -KeyUsage DigitalSignature `
    -FriendlyName "FoxM Development Certificate (5 Years)" `
    -CertStoreLocation "Cert:\LocalMachine\My" `
    -KeyExportPolicy Exportable `
    -NotAfter $notAfterDate `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")

# Xuất file PFX
Export-PfxCertificate -Cert $cert -FilePath $CertPath -Password $securePassword | Out-Null

# Cài đặt vào Trusted Root và Trusted People để MSBuild tin cậy
try {
    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store "Root", "LocalMachine"
    $rootStore.Open("ReadWrite")
    $rootStore.Add($cert)
    $rootStore.Close()

    $peopleStore = New-Object System.Security.Cryptography.X509Certificates.X509Store "TrustedPeople", "LocalMachine"
    $peopleStore.Open("ReadWrite")
    $peopleStore.Add($cert)
    $peopleStore.Close()
} catch {
    Write-Host "[FoxM Sign] Bỏ qua ghi LocalMachine Store nếu không chạy dưới quyền Admin." -ForegroundColor Yellow
}

Write-Host "[FoxM Sign] Thành công! Chứng chỉ đã được lưu tại: $CertPath (Hết hạn: $($notAfterDate.ToString('dd/MM/yyyy')))" -ForegroundColor Green
