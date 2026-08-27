<#
.SYNOPSIS
    Tạo chứng chỉ ký số tự ký (Self-signed Certificate) để ký gói .appx trên GitHub Actions runner.
#>

param(
    [string]$CertPath = "$PSScriptRoot\FoxM_SigningCert.pfx",
    [string]$Password = "FoxM2026Password!"
)

Write-Host "[FoxM Sign] Đang tạo chứng chỉ tự ký cho Publisher CN=FoxMDev..." -ForegroundColor Cyan

$securePassword = ConvertTo-SecureString -String $Password -Force -AsPlainText

$cert = New-SelfSignedCertificate `
    -Type Custom `
    -Subject "CN=FoxMDev" `
    -KeyUsage DigitalSignature `
    -FriendlyName "FoxM Development Certificate" `
    -CertStoreLocation "Cert:\LocalMachine\My" `
    -KeyExportPolicy Exportable `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")

# Xuất file PFX
Export-PfxCertificate -Cert $cert -FilePath $CertPath -Password $securePassword | Out-Null

# Cài đặt vào Trusted Root và Trusted People để MSBuild tin cậy
$rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store "Root", "LocalMachine"
$rootStore.Open("ReadWrite")
$rootStore.Add($cert)
$rootStore.Close()

$peopleStore = New-Object System.Security.Cryptography.X509Certificates.X509Store "TrustedPeople", "LocalMachine"
$peopleStore.Open("ReadWrite")
$peopleStore.Add($cert)
$peopleStore.Close()

Write-Host "[FoxM Sign] Thành công! Chứng chỉ đã được lưu tại: $CertPath" -ForegroundColor Green
