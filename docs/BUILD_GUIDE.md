# 🛠️ FoxM Build Guide

Hướng dẫn chi tiết cách thiết lập môi trường biên dịch mã nguồn Goanna C++ cho nền tảng Windows 10 Mobile (UWP ARM32/ARM64).

---

## 1. Yêu cầu Môi trường (Prerequisites)

1. **Hệ điều hành Host:** Windows 10 hoặc Windows 11 (64-bit).
2. **Visual Studio:** Visual Studio 2017 (v15.9) hoặc Visual Studio 2019 (v16.11).
   * Workload: **Universal Windows Platform development** (UWP).
   * C++ Universal Windows Platform tools (v141 hoặc v142).
   * **Windows 10 SDK 10.0.19041** hoặc **10.0.15063 / 10.0.14393** (cho Windows 10 Mobile).
   * MSVC v141 - VS 2017 C++ ARM build tools (hoặc v142 ARM).
3. **MozillaBuild Package:**
   * Tải và cài đặt [MozillaBuild 3.4 / 4.0](https://ftp.mozilla.org/pub/mozilla/libraries/win32/MozillaBuildSetup-Latest.exe).
   * Cài đặt vào thư mục mặc định `C:\mozilla-build\`.

---

## 2. Các Bước Biên Dịch Engine

### Bước 1: Mở MozillaBuild Shell
Khởi chạy file `C:\mozilla-build\start-shell.bat`.

### Bước 2: Clone Mã nguồn Goanna Core
```bash
git clone https://github.com/MoonchildProductions/UXP.git goanna-src
cd goanna-src
```

### Bước 3: Áp dụng Cấu hình mozconfig
Sao chép file cấu hình từ thư mục `config/` của dự án FoxM:
```bash
cp ../config/mozconfig-uwp-arm32 .mozconfig
```

### Bước 4: Khởi chạy Biên dịch
```bash
./mach build
```
Sau khi hoàn tất, thư viện `xul.dll`, `nss3.dll`, `mozglue.dll` sẽ được tạo ra tại thư mục `obj-uwp-arm/dist/bin/`.

---

## 3. Biên Dịch & Đóng Gói Ứng Dụng UWP (Visual Studio)

1. Mở solution `src/FoxM.UwpHost/FoxM.UwpHost.csproj` trong Visual Studio 2019.
2. Chọn cấu hình **Release | ARM**.
3. Sao chép các file `.dll` đã build từ Bước 2 vào thư mục đầu ra của dự án UWP.
4. Nhấn **Build -> Deploy** để nạp thẳng file `.appx` vào điện thoại Lumia kết nối qua cáp USB hoặc Device Portal!
