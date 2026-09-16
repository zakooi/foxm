<#
.SYNOPSIS
    Tự động tải và đồng bộ hóa kho chứng chỉ gốc Mozilla NSS Root CA mới nhất cho FoxM.
.DESCRIPTION
    Script này tải file certdata.txt chính thức từ Mozilla NSS repository và chuyển đổi
    thành định dạng nssckbi tương thích với Goanna Engine trên Windows 10 Mobile.

    BƯỚC MỚI (Agent Network): sau khi tải, script trích các CA đáng tin (serverAuth)
    ra bundle PEM tại $PemOutputPath để HttpStack (src/FoxM.GoannaRuntime/HttpStack.*)
    dùng cho bước pin/validate thủ công tiếp theo.
    Đường dẫn bundle mặc định:
        src/FoxM.UwpHost/Assets/mozilla-roots.pem
    Tương thích ngược: gọi không tham số vẫn chạy đúng như script cũ.
#>

[CmdletBinding()]
param(
    [string]$OutputPath = "$PSScriptRoot\certdata.txt",
    [string]$PemOutputPath = "$PSScriptRoot\..\FoxM.UwpHost\Assets\mozilla-roots.pem",
    [switch]$SkipDownload = $false
)

$MozillaCertUrl = "https://hg.mozilla.org/mozilla-central/raw-file/default/security/nss/lib/ckfw/builtins/certdata.txt"

Write-Host "[FoxM NSS] Đang tải kho chứng chỉ Mozilla NSS mới nhất..." -ForegroundColor Cyan

try {
    if (-not $SkipDownload) {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 -bor [Net.SecurityProtocolType]::Tls13
        $webClient = New-Object System.Net.WebClient
        $webClient.DownloadFile($MozillaCertUrl, $OutputPath)

        $lines = (Get-Content $OutputPath | Measure-Object -Line).Lines
        Write-Host "[FoxM NSS] Thành công! Đã nạp $lines dòng dữ liệu chứng chỉ vào $OutputPath" -ForegroundColor Green
        Write-Host "[FoxM NSS] Windows 10 Mobile giờ đây đã có thể xác thực tất cả các website TLS 1.3 và HTTPS mới nhất." -ForegroundColor Yellow
    }
    elseif (-not (Test-Path $OutputPath)) {
        throw "SkipDownload được bật nhưng không tìm thấy $OutputPath"
    }
}
catch {
    Write-Error "[FoxM NSS] Thất bại khi tải chứng chỉ: $_"
    exit 1
}

# --- Bước 2 (mới): certdata.txt -> bundle PEM (chỉ CA serverAuth đáng tin) ---
Write-Host "[FoxM NSS] Đang chuyển đổi certdata.txt sang PEM bundle..." -ForegroundColor Cyan

try {
    $pemDir = Split-Path -Parent $PemOutputPath
    if (-not (Test-Path $pemDir)) {
        New-Item -ItemType Directory -Force -Path $pemDir | Out-Null
    }

    # Pass 1: thu thập label của trust object cho phép serverAuth
    # (CKO_NSS_TRUST + CKA_TRUST_SERVER_AUTH = CKT_NSS_TRUSTED_DELEGATOR).
    $trustedLabels = @{}
    $curLabel = $null
    $inTrustObj = $false
    foreach ($line in (Get-Content $OutputPath)) {
        $t = $line.Trim()
        if ($t -eq "CKA_CLASS CK_OBJECT_CLASS CKO_NSS_TRUST") { $inTrustObj = $true; $curLabel = $null; continue }
        if ($inTrustObj -and $t.StartsWith("CKA_LABEL ")) {
            if ($t -match 'CKA_LABEL UTF8 "(.*)"') { $curLabel = $Matches[1] }
            continue
        }
        if ($inTrustObj -and $t -eq "CKA_TRUST_SERVER_AUTH CK_TRUST CKT_NSS_TRUSTED_DELEGATOR") {
            if ($curLabel) { $trustedLabels[$curLabel] = $true }
            continue
        }
        if ($t -eq "" -and $inTrustObj) { $inTrustObj = $false; $curLabel = $null }
    }

    # Pass 2: trích DER (octal) của CKO_CERTIFICATE có label nằm trong trustedLabels.
    $sb = New-Object System.Text.StringBuilder
    $count = 0
    $inCert = $false; $inValue = $false; $label = $null
    $der = New-Object System.Collections.Generic.List[byte]
    foreach ($line in (Get-Content $OutputPath)) {
        $t = $line.Trim()
        if ($t -eq "CKA_CLASS CK_OBJECT_CLASS CKO_CERTIFICATE") { $inCert = $true; $inValue = $false; $label = $null; $der.Clear(); continue }
        if (-not $inCert) { continue }
        if ($t.StartsWith("CKA_LABEL ")) {
            if ($t -match 'CKA_LABEL UTF8 "(.*)"') { $label = $Matches[1] }
            continue
        }
        if ($t -eq "CKA_VALUE MULTILINE OCTAL") { $inValue = $true; continue }
        if ($t -eq "END CKA_VALUE") {
            $inValue = $false
            if ($label -and $trustedLabels.ContainsKey($label) -and $der.Count -gt 0) {
                $b64 = [Convert]::ToBase64String($der.ToArray(), [Base64FormattingOptions]::InsertLineBreaks)
                # Chuẩn hoá ngắt dòng đúng 64 ký tự/dòng cho PEM.
                $b64 = ($b64 -split "`r?`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }) -join "`n"
                [void]$sb.Append("# label=`"$label`"`n")
                [void]$sb.Append("-----BEGIN CERTIFICATE-----`n")
                [void]$sb.Append($b64 + "`n")
                [void]$sb.Append("-----END CERTIFICATE-----`n")
                $count++
            }
            $der.Clear()
            continue
        }
        if ($inValue) {
            foreach ($part in ($t -split '\\')) {
                if ($part -eq "") { continue }
                [void]$der.Add([Convert]::ToByte($part, 8))
            }
            continue
        }
        if ($t -eq "" -and -not $inValue) { $inCert = $false; $label = $null }
    }

    if ($count -eq 0) { throw "Không trích được CA nào (certdata.txt sai định dạng?)" }

    $header = "# FoxM Mozilla root bundle (PEM) — tạo tự động từ certdata.txt.`n" + `
              "# Chỉ gồm CA serverAuth TRUSTED_DELEGATOR. Dùng cho HttpStack pin/validate thủ công.`n"
    Set-Content -Path $PemOutputPath -Value ($header + $sb.ToString()) -Encoding Ascii -NoNewline
    Write-Host "[FoxM NSS] Đã xuất $count CA đáng tin ra $PemOutputPath" -ForegroundColor Green
}
catch {
    Write-Error "[FoxM NSS] Thất bại khi chuyển đổi PEM: $_"
    exit 1
}
