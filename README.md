# 🦊 FoxM — Goanna/Gecko C++ Web Engine & Browser for Windows 10 Mobile

> **Dự án nghiên cứu & triển khai nhân trình duyệt Goanna/Gecko (C++ thuần) cho nền tảng Windows 10 Mobile (UWP ARM32 / ARM64).**

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20Mobile%20(UWP)-blue.svg)](https://github.com/zakooi/foxm)
[![Architecture](https://img.shields.io/badge/Arch-ARM32%20%7C%20ARM64-green.svg)](https://github.com/zakooi/foxm)
[![Engine](https://img.shields.io/badge/Engine-Goanna%20%2F%20Gecko%20C%2B%2B-orange.svg)](https://github.com/zakooi/foxm)
[![License](https://img.shields.io/badge/License-MPL%202.0-lightgrey.svg)](LICENSE)

---

## 📖 Giới thiệu (Overview)

Các thiết bị **Windows 10 Mobile** (dòng Nokia Lumia 520, 640, 730, 830, 920, 930, 950, 1520...) hiện gặp trở ngại lớn khi duyệt web do thành phần `WebView` mặc định sử dụng nhân **EdgeHTML 15/17 (năm 2017)** đã lỗi thời, không hỗ trợ các chuẩn web mới (ES2022+, Web Components, TLS 1.3, WebP, AVIF) và liên tục lỗi chứng chỉ HTTPS.

**FoxM** là giải pháp mang một **Web Engine độc lập thế hệ mới dựa trên mã nguồn Goanna / Gecko C++ (Pale Moon / Basilisk fork)** lên Windows 10 Mobile dưới dạng một WinRT Component kết nối trực tiếp vào XAML `SwapChainPanel`.

```
+-------------------------------------------------------------------------------+
|                       UWP XAML UI (MainPage.xaml)                             |
|          (Multi-Tab, Omnibox, Memory Guard, Gesture & Touch System)           |
+---------------------------------------+---------------------------------------+
                                        | (ISwapChainPanelNative / Touch Events)
+---------------------------------------v---------------------------------------+
|                 FoxM.GoannaRuntime (C++/CX WinRT Component)                   |
|  +---------------------------+  +------------------------------------------+  |
|  | Gecko Layout (HTML/CSS)   |  | SpiderMonkey JS (Interpreter / JIT-Cap)  |  |
|  +---------------------------+  +------------------------------------------+  |
|  | Necko + Mozilla NSS 2026  |  | Direct3D 11 Layers Compositor            |  |
|  +---------------------------+  +------------------------------------------+  |
|  | Single-Process Coordinator & Low-RAM Memory Watchdog                    |  |
+-------------------------------------------------------------------------------+
                                        |
+---------------------------------------v---------------------------------------+
|             Windows 10 Mobile OS Kernel (10.0.15254 / ARMv7)                  |
|        (Direct3D 11, Winsock UWP, ThreadPool, AppContainer Sandbox)          |
+-------------------------------------------------------------------------------+
```

---

## ⚡ Các giải pháp kỹ thuật cốt lõi (Core Architecture)

1. **Thuần C++ (Không phụ thuộc Rust):** Sử dụng nhánh Goanna Engine viết 100% bằng C++ chuẩn (C++14/17), tương thích hoàn hảo với bộ công cụ biên dịch MSVC ARM của Visual Studio 2017/2019 cho UWP.
2. **Xuất hình ảnh qua Direct3D 11:** Không dùng Win32 `HWND`. Lớp `WidgetUwp` kết nối trực tiếp bộ render của Gecko với `IDXGISwapChain1` gắn vào `SwapChainPanel` của XAML.
3. **Vượt rào JIT trên UWP:**
   * Mặc định hỗ trợ chế độ **C++ Interpreter** thuần túy của SpiderMonkey (an toàn tuyệt đối, không vi phạm chính sách W^X).
   * Hỗ trợ mở khóa JIT Compiler thông qua Capability đặc quyền `<rescap:Capability Name="codeGeneration"/>`.
4. **Kho chứng chỉ độc lập (Mozilla NSS Root Store):** Tích hợp chứng chỉ gốc Mozilla mới nhất năm 2026, loại bỏ 100% lỗi hết hạn chứng chỉ TLS/SSL của hệ điều hành Windows 10 Mobile.
5. **Cơ chế kiểm soát RAM cho thiết bị 1GB / 512MB:** Tinh chỉnh các thông số `all-w10m.js`, tự động giải phóng bộ nhớ đệm ảnh và lịch sử khi nhận tín hiệu từ `MemoryManager.AppMemoryUsageLimitChanging`.

---

## 📁 Cấu trúc thư mục (Repository Structure)

```
foxm/
├── docs/                               Tài liệu kiến trúc & can thiệp hệ thống
│   ├── ARCHITECTURE.md                 Thiết kế kiến trúc chi tiết của FoxM
│   ├── W10M_LIMITATIONS_AND_HACKS.md   Phân tích hạn chế W10M và các giải pháp can thiệp
│   └── BUILD_GUIDE.md                  Hướng dẫn cấu hình toolchain & build
├── config/                             Cấu hình Engine & Biên dịch
│   ├── mozconfig-uwp-arm32             Cấu hình build Goanna cho ARM32 UWP
│   ├── mozconfig-uwp-arm64             Cấu hình build Goanna cho ARM64 UWP
│   └── all-w10m.js                     Cấu hình Preferences tối ưu RAM cho W10M
├── src/
│   ├── FoxM.GoannaRuntime/             WinRT Component bọc nhân Goanna (C++/CX)
│   │   ├── GoannaView.h / .cpp         Control XAML SwapChainPanel điều phối render
│   │   ├── WidgetUwp.h / .cpp          Tầng giao tiếp đồ họa & sự kiện cảm ứng
│   │   └── SpiderMonkeyHost.h / .cpp   Khởi tạo & cấu hình Engine JS
│   ├── FoxM.UwpHost/                   Ứng dụng Browser mẫu viết bằng C# XAML
│   │   ├── App.xaml / .cs
│   │   ├── MainPage.xaml / .cs         Giao diện duyệt đa tab, thanh địa chỉ, RAM meter
│   │   └── Package.appxmanifest        Manifest cấu hình quyền đặc quyền
│   └── FoxM.NssSecurity/
│       └── update-root-certs.ps1       Script đồng bộ chứng chỉ gốc Mozilla 2026
├── .github/workflows/ci.yml            GitHub Actions CI kiểm tra build
├── LICENSE                             Giấy phép Mozilla Public License 2.0
└── README.md
```

---

## 🚀 Hướng dẫn bắt đầu (Quick Start)

Xem chi tiết trong:
* 📄 [Tài liệu Kiến trúc Engine (ARCHITECTURE.md)](docs/ARCHITECTURE.md)
* 📄 [Can thiệp sâu hệ điều hành W10M (W10M_LIMITATIONS_AND_HACKS.md)](docs/W10M_LIMITATIONS_AND_HACKS.md)
* 📄 [Hướng dẫn Biên dịch (BUILD_GUIDE.md)](docs/BUILD_GUIDE.md)

---

## 📜 Giấy phép (License)

Dự án phát hành dưới giấy phép **Mozilla Public License 2.0 (MPL-2.0)**.
