<#
.SYNOPSIS
    Tạo các file hình ảnh biểu tượng placeholder (PNG) chuẩn cho FoxM UWP trên Windows 10 Mobile.
#>

param(
    [switch]$Force = $false
)

$assetsDir = "$PSScriptRoot\..\src\FoxM.UwpHost\Assets"
if (-not (Test-Path $assetsDir)) {
    New-Item -ItemType Directory -Force -Path $assetsDir | Out-Null
}

Add-Type -AssemblyName System.Drawing

function Create-Png($path, $width, $height, $bgHex, $text) {
    if ((Test-Path $path) -and -not $Force) {
        Write-Host "[FoxM Assets] File đã tồn tại, giữ nguyên: $path" -ForegroundColor Gray
        return
    }

    $bmp = New-Object System.Drawing.Bitmap($width, $height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    
    $color = [System.Drawing.ColorTranslator]::FromHtml($bgHex)
    $brush = New-Object System.Drawing.SolidBrush($color)
    $g.FillRectangle($brush, 0, 0, $width, $height)
    
    $textColor = [System.Drawing.Color]::White
    $textBrush = New-Object System.Drawing.SolidBrush($textColor)
    $font = New-Object System.Drawing.Font("Segoe UI", [Math]::Max(8, [int]($height / 6)), [System.Drawing.FontStyle]::Bold)
    
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    
    $rect = New-Object System.Drawing.RectangleF(0, 0, $width, $height)
    $g.DrawString($text, $font, $textBrush, $rect, $sf)
    
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
    Write-Host "[FoxM Assets] Đã tạo mới: $path ($width x $height)" -ForegroundColor Green
}

Create-Png "$assetsDir\Square150x150Logo.png" 150 150 "#FFD85012" "🦊 FoxM"
Create-Png "$assetsDir\Square44x44Logo.png" 44 44 "#FFD85012" "🦊"
Create-Png "$assetsDir\StoreLogo.png" 50 50 "#FFD85012" "🦊"
Create-Png "$assetsDir\SplashScreen.png" 620 300 "#FF18181C" "🦊 FoxM Browser for Windows 10 Mobile"

Write-Host "[FoxM Assets] Hoàn tất kiểm tra assets!" -ForegroundColor Cyan
