# 🏗️ FoxM Architecture Specification

Tài liệu này quy định chi tiết cấu trúc kiến trúc và luồng dữ liệu của **FoxM** khi chạy trên môi trường **Windows 10 Mobile UWP (ARM32 / ARM64)**.

---

## 1. Phân tầng Kiến trúc (Architectural Layers)

```
+-------------------------------------------------------------------------------+
| Layer 1: XAML UI Host (FoxM.UwpHost - C# / XAML)                              |
| - Điều hướng đa tab (Multi-tab management)                                    |
| - Omnibox (Address bar & search suggestions)                                  |
| - Memory Guard (Giám sát RAM thời gian thực)                                  |
| - Bookmarks, History, Download Manager                                        |
+---------------------------------------+---------------------------------------+
                                        | WinRT ABI / Event Dispatch
+---------------------------------------v---------------------------------------+
| Layer 2: WinRT Interop Bridge (FoxM.GoannaRuntime - C++/CX)                   |
| - GoannaView: Điều khiển XAML SwapChainPanel                                  |
| - Native DirectX 11 SwapChain Hookup (ISwapChainPanelNative)                  |
| - Input Event Translator (Touch/Pointer -> nsGUIEvent)                        |
| - Lifecycle & Navigation Callbacks                                            |
+---------------------------------------+---------------------------------------+
                                        | C++ Internal API
+---------------------------------------v---------------------------------------+
| Layer 3: Goanna Core Engine (Modified Gecko 52 / C++17)                       |
| +-----------------------------------+ +-------------------------------------+ |
| | Layout & DOM (Gecko C++ Core)     | | SpiderMonkey JavaScript Engine      | |
| | - HTML5 Parser / CSS3 Grid & Flex | | - Mode A: JIT-less Interpreter      | |
| | - WebP / SVG Image Decoders       | | - Mode B: JIT with CodeGen Cap      | |
| +-----------------------------------+ +-------------------------------------+ |
| +-----------------------------------+ +-------------------------------------+ |
| | Necko Network & Security          | | D3D11 Compositor Layer (WidgetUwp)  | |
| | - HTTP/1.1 & HTTP/2 Stack         | | - DirectX 11 HW Acceleration        | |
| | - Mozilla NSS 2026 TLS 1.3        | | - DirectComposition / SwapChain1    | |
| +-----------------------------------+ +-------------------------------------+ |
+---------------------------------------+---------------------------------------+
                                        | OS System Calls
+---------------------------------------v---------------------------------------+
| Layer 4: Windows 10 Mobile Platform (10.0.15254 ARMv7)                        |
| - WinSock (UWP Network Capability)                                            |
| - Direct3D 11.0 / DXGI Runtime                                                |
| - WinRT ThreadPool & Storage API                                              |
+-------------------------------------------------------------------------------+
```

---

## 2. Chi tiết các thành phần chính

### 2.1. GoannaView & DirectX 11 Integration (`WidgetUwp`)
* `GoannaView` kế thừa từ `Windows::UI::Xaml::Controls::SwapChainPanel`.
* Khi khởi tạo, nó truy vấn giao diện gốc `ISwapChainPanelNative`:
  ```cpp
  ComPtr<ISwapChainPanelNative> panelNative;
  reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&panelNative));
  panelNative->SetSwapChain(m_dxgiSwapChain.Get());
  ```
* Goanna D3D11 Layers Compositor vẽ trực tiếp các frame web vào `m_dxgiSwapChain`, đảm bảo tốc độ cuộn mượt mà 60 FPS mà không phải qua bất kỳ lớp copy CPU nào.

### 2.2. SpiderMonkey Configuration (`SpiderMonkeyHost`)
* Được cấu hình chủ động dựa trên quyền hạn của ứng dụng:
  * Nếu máy có quyền `codeGeneration`: Khởi chạy Baseline JIT để đạt hiệu năng tối đa.
  * Nếu máy ở chế độ chuẩn: Tắt JIT (`JS_DISABLE_JIT=1`), chạy bằng C++ Interpreter ổn định 100%.

### 2.3. Necko & NSS TLS 1.3 Pipeline
* Necko độc lập hoàn toàn với Schannel của Windows.
* Tự giải mã SSL/TLS bằng thư viện **NSS (Network Security Services)** nhúng kèm file `certdata.txt` 2026.

---

## 3. Quản lý Bộ nhớ (Low-RAM Strategy)

1. **Giới hạn Cache:** RAM cache giới hạn ở mức 16MB.
2. **Bộ giải mã hình ảnh:** Ảnh ngoài khung nhìn (Off-screen) tự động được giải phóng khỏi RAM sau 10 giây.
3. **Memory Trimming:** Khi nhận thông báo `AppMemoryUsageLimitChanging`, engine lập tức kích hoạt chu trình dọn rác SpiderMonkey GC và giải phóng các DOM nodes nhàn rỗi.
