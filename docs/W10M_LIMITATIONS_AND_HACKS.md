# 🛡️ Windows 10 Mobile: Hạn chế & Các Kỹ thuật Can thiệp (Hacks & Workarounds)

Tài liệu này ghi lại toàn bộ kinh nghiệm thực chiến về các rào cản hệ thống trên **Windows 10 Mobile (W10M)** và cách thức dự án **FoxM** can thiệp để vượt qua.

---

## 1. Rào cản JIT Compiler (W^X Policy)

### Vấn đề:
Hệ điều hành Windows 10 Mobile áp dụng chính sách bảo mật **W^X (Write XOR Execute)** nghiêm ngặt trong AppContainer. Bất kỳ lệnh gọi `VirtualAlloc` hoặc `VirtualProtect` nào yêu cầu quyền `PAGE_EXECUTE_READWRITE` từ ứng dụng bên thứ 3 đều bị từ chối với lỗi `STATUS_ACCESS_VIOLATION`.

### Giải pháp Can thiệp của FoxM:
1. **Chế độ Mặc định (An toàn tuyệt đối):** Ép SpiderMonkey chạy thuần **C++ Interpreter** bằng cách truyền tham số khởi tạo:
   ```cpp
   JS_SetGlobalJitCompilerOption(cx, JSJITCOMPILER_BASELINE_ENABLE, 0);
   JS_SetGlobalJitCompilerOption(cx, JSJITCOMPILER_ION_ENABLE, 0);
   ```
2. **Chế độ Tối ưu (Dành cho Developer Mode / Sideload):** Thêm Restricted Capability vào `Package.appxmanifest`:
   ```xml
   <Package
     xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
     IgnorableNamespaces="rescap">
     <Capabilities>
       <rescap:Capability Name="codeGeneration"/>
     </Capabilities>
   </Package>
   ```
   Khi máy bật *Developer Mode*, quyền này cho phép ứng dụng cấp phát bộ nhớ JIT để tăng tốc độ JavaScript lên gấp $3 - 5$ lần.

---

## 2. Rào cản Cửa sổ Win32 (`HWND` & GDI)

### Vấn đề:
Gecko trên máy tính Desktop phụ thuộc vào `CreateWindowEx` và vòng lặp thông điệp `GetMessage` / `DispatchMessage` (`widget/windows/nsWindow.cpp`). Trong UWP, những API này hoàn toàn bị cấm.

### Giải pháp Can thiệp của FoxM:
* Xây dựng lớp dẫn xuất `WidgetUwp` kế thừa `nsBaseWidget`.
* Kết nối trực tiếp với giao diện Direct3D 11:
  * Lấy `IDXGISwapChain1` từ XAML `SwapChainPanel`.
  * Truyền con trỏ Surface vào `D3D11LayersCompositor` của Gecko.
  * Bản đồ hóa sự kiện Touch / Pointer từ XAML sang `WidgetTouchEvent` của Gecko.

---

## 3. Rào cản Bộ nhớ RAM (512MB / 1GB) & Cơ chế Kill App

### Vấn đề:
W10M tự động đóng ứng dụng nếu vượt quá ngưỡng RAM phân bổ ($\approx 380\text{MB}$ trên máy 1GB RAM như Lumia 920/830/730).

### Giải pháp Can thiệp:

#### A. Can thiệp cấp OS (Dành cho máy Interop Unlock):
* Sử dụng **Interop Tools** chỉnh sửa Registry để mở rộng bộ nhớ ảo (Pagefile):
  * **Key:** `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management`
  * **Name:** `PagingFiles`
  * **Value (REG_MULTI_SZ):** `C:\pagefile.sys 1024 2048`
  * *Kết quả:* Thiết bị có thêm 1GB - 2GB RAM ảo trên bộ nhớ trong, chống văng app khi lướt web nặng.

#### B. Can thiệp cấp Engine (Cấu hình `all-w10m.js`):
```javascript
pref("image.mem.max_decoded_image_kb", 32768);  // Tối đa 32MB cho ảnh giải mã
pref("browser.sessionhistory.max_entries", 4);   // Giới hạn lịch sử tab
pref("javascript.options.mem.gc_frequency", 50); // Thu gom rác JS liên tục
pref("browser.cache.memory.capacity", 16384);    // Giới hạn cache RAM 16MB
```

---

## 4. Rào cản Chứng chỉ TLS/SSL Quá Hạn

### Vấn đề:
Schannel trên W10M không được cập nhật Root Certificate từ năm 2018, khiến phần lớn website hiện nay (dùng chứng chỉ Let's Encrypt ISRG Root X1, Cloudflare, DigiCert 2026) đều bị báo lỗi SSL.

### Giải pháp Can thiệp của FoxM:
* FoxM **hoàn toàn không sử dụng Windows Schannel**.
* Dự án tích hợp thư viện **Mozilla NSS (Network Security Services)** kèm bộ chứng chỉ `certdata.txt` độc lập mới nhất 2026. Mọi kết nối TLS 1.3 và HTTPS đều được xác thực độc lập, an toàn và không phụ thuộc vào hệ điều hành.
