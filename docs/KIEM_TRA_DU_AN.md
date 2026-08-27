# 🔍 Báo cáo kiểm tra toàn diện dự án FoxM

> **Ngày kiểm tra:** 2026-08-27
> **Phạm vi:** Toàn bộ 37 file (C++/CX, C# XAML, PowerShell, CI, config, docs)
> **Kết quả:** ~40 vấn đề phát hiện, chia 4 mức độ P0–P3

---

## 1. Cấu trúc dự án hiện tại

```
foxm/
├── .github/workflows/
│   └── build-appx.yml                 # CI build ARM + x86 → .appx + ký số
├── config/
│   ├── all-w10m.js                    # Prefs engine tối ưu RAM (16MB cache, tắt Ion JIT...)
│   ├── mozconfig-uwp-arm32            # Cấu hình build Goanna ARM32
│   └── mozconfig-uwp-arm64            # Cấu hình build Goanna ARM64
├── docs/
│   ├── ARCHITECTURE.md                # Thiết kế 4 tầng
│   ├── BUILD_GUIDE.md                 # Hướng dẫn build UXP + UWP
│   ├── W10M_LIMITATIONS_AND_HACKS.md  # W^X, HWND, RAM, TLS
│   └── KIEM_TRA_DU_AN.md              # (file này)
├── scripts/
│   ├── generate-assets.ps1            # Sinh PNG placeholder
│   └── generate-cert.ps1              # Self-signed cert CN=FoxMDev
├── src/
│   ├── FoxM.GoannaRuntime/            # C++/CX WinRT Component (7 file — CHƯA có .vcxproj)
│   │   ├── GoannaView.h/.cpp          # SwapChainPanel + D3D11
│   │   ├── WidgetUwp.h/.cpp           # Touch/Zoom events
│   │   ├── SpiderMonkeyHost.h/.cpp    # Khởi tạo JS engine
│   │   └── DocShellBridge.h/.cpp      # Điều hướng + lịch sử (dead code)
│   ├── FoxM.NssSecurity/
│   │   └── update-root-certs.ps1      # Tải certdata.txt từ Mozilla (chỉ download, chưa convert)
│   └── FoxM.UwpHost/                  # App C# XAML
│       ├── App.xaml / App.xaml.cs
│       ├── MainPage.xaml / .cs        # UI đa tab, omnibox
│       ├── Package.appxmanifest
│       ├── FoxM.UwpHost.csproj        # KHÔNG tham chiếu GoannaRuntime
│       ├── Assets/ (4 PNG)
│       └── Services/ (6 file)         # Bookmark, History, MemoryGuard,
│                                      # ReaderMode, Settings, TabManager
├── LICENSE                            # MPL 2.0
└── README.md                          # Ghi sai tên file CI (ci.yml → build-appx.yml)
```

### Trạng thái kết nối giữa các module

```
FoxM.UwpHost (C#)  ──✗──►  FoxM.GoannaRuntime (C++/CX)   ← KHÔNG kết nối
     │                              │
     │ GoBack/Forward = empty       │ Navigate() = giả lập (fire event ngay)
     │ ReaderMode = sinh script     │ SpiderMonkey = chỉ OutputDebugString
     │ nhưng không execute          │ DocShellBridge = dead code
     └──────────────────────────────┘
```

---

## 2. Vấn đề P0 — Nghiêm trọng (chặn hoạt động)

| # | File | Vấn đề | Hướng sửa |
|---|------|--------|-----------|
| 1 | `FoxM.UwpHost.csproj` | **Không có ProjectReference** đến GoannaRuntime → C# không nhìn thấy `GoannaView` | Thêm ProjectReference + reference `.winmd` |
| 2 | `MainPage.xaml` | Không `xmlns` GoannaView, không instantiate → viewport chỉ là `Grid` rỗng | Thêm xmlns + `<goanna:GoannaView>` vào ViewportContainer |
| 3 | `FoxM.GoannaRuntime/` | **Không có `.vcxproj`** → không build được, không sinh `.winmd` | Tạo vcxproj C++/CX WinRT Component |
| 4 | `GoannaView.cpp:24,29` + `SpiderMonkeyHost.cpp` | **Ref-count sai**: đóng 1 tab → `s_isInitialized=false` trong khi tab khác còn sống → EvaluateScript từ chối. Không thread-safe | Ref-count hoặc chỉ shutdown khi không còn instance; thêm lock/atomic |
| 5 | `GoannaView.cpp:41-44` | `D3D11_CREATE_DEVICE_DEBUG` trong _DEBUG → fail `DXGI_ERROR_SDK_COMPONENT_MISSING` trên máy không có debug layer → màn hình trắng | Thử với DEBUG flag, fail thì retry không flag |
| 6 | `GoannaView.cpp:58-105` | **Không check HRESULT** của D3D11CreateDevice, `As()`, `GetAdapter`, `GetParent`, `CreateSwapChainForComposition` → null deref dòng 95; FL 9.3/10.x mâu thuẫn với ID3D11Device2 (cần FL 11.1) | Check từng HRESULT; bỏ FL < 11.0 hoặc dùng ID3D11Device1 |
| 7 | `GoannaView.cpp:178-181` | `OnSizeChanged` tạo swapchain MỚI mỗi resize, không Release cũ → leak | Dùng `ResizeBuffers` hoặc Release trước khi tạo mới |

## 3. Vấn đề P1 — Cao

| # | File | Vấn đề |
|---|------|--------|
| 8 | `GoannaView.cpp:115-122` | `Navigate()` giả lập: fire ProgressChanged(0.1)→(1.0)→NavigationCompleted ngay, không nạp gì thật. GoBack/GoForward/Stop rỗng |
| 9 | `DocShellBridge.*` | Dead code — GoannaView không dùng; mỗi nơi tự giả lập riêng |
| 10 | `DocShellBridge.cpp:61-67` | `Reload()` gọi `LoadUri` → push entry lịch sử trùng lặp mỗi lần reload |
| 11 | `MainPage.xaml.cs:181-189` | `BackButton_Click` / `ForwardButton_Click` rỗng → nút không hoạt động |
| 12 | `MainPage.xaml.cs:196-200` | Reader Mode sinh script nhưng không execute → tính năng không tồn tại |
| 13 | `App.xaml.cs:23-39` | Memory Guard chỉ Debug.WriteLine, không gọi `MinimizeMemoryUsage` / GC thật |
| 14 | `MemoryGuardService.cs:51-58` | `GC.Collect()` mỗi 3 giây khi level High/>75% → performance killer trên máy 512MB. Cần cooldown 30-60s |
| 15 | `mozconfig-uwp-arm32/arm64` | Flag **không tồn tại trong UXP**: `--enable-winuwp`, `--enable-windows-mobile`, `--enable-js-interpreter`, `--disable-vulkan` → `./mach build` lỗi unknown option. Target `arm-windows-msvc` / `aarch64-windows-msvc` chưa được UXP hỗ trợ sẵn → cần patch configure |
| 16 | `all-w10m.js:30` | `security.tls.version.min = 1` → cho phép TLS 1.0 (BEAST, bị nhiều server từ chối). Nên `= 3` (TLS 1.2) |
| 17 | CI `build-appx.yml` | Mỗi build tạo cert MỚI → người dùng phải gỡ app + cài lại cert mỗi lần update. Nên lưu 1 cert cố định trong GitHub Secrets |
| 18 | `Package.appxmanifest:54` | `rescap:codeGeneration` → app không cài được trên máy không mở Developer Mode |

## 4. Vấn đề P2 — Trung bình

| # | File | Vấn đề |
|---|------|--------|
| 19 | `MainPage.xaml.cs:55-62` | `ActiveTabChanged` → `NavigateTo` → `AddEntryAsync` → ghi lịch sử trùng mỗi lần switch tab/refresh; title lưu = URL |
| 20 | `MainPage.xaml.cs:162-179` | Race: click ⭐ trước khi `MainPage_Loaded` xong → `_bookmarks` null → NullReferenceException |
| 21 | `MainPage.xaml.cs:232-239` | TabListView giữ SelectedItem sau khi đóng overlay → mở lại, chạm cùng tab không fire SelectionChanged |
| 22 | `ReaderModeService.cs:75` | **Bug màu CSS**: `#FFD85012` theo CSS = `#RRGGBBAA` → alpha 0x12 (7%) → nút gần như vô hình. Đúng: `#D85012` |
| 23 | `ReaderModeService.cs:72-75` | title + innerHTML chèn thẳng vào HTML → injection từ nội dung trang (nên escape) |
| 24 | `App.xaml.cs:67-71` | `OnSuspending` deferral rồi Complete ngay, không lưu tabs/bookmarks → mất trạng thái khi W10M kill app. Thiếu `OnResuming` |
| 25 | `WidgetUwp.cpp:39-40` | `deltaX`/`deltaY` tính rồi bỏ → warning C4189; touch không gửi event thật vào Gecko |
| 26 | `WidgetUwp.*` | State static dùng chung mọi tab → multi-tab không khả thi với thiết kế hiện tại |
| 27 | `generate-assets.ps1` | CI ghi đè assets mỗi lần chạy bằng placeholder → mất logo thật. Chỉ generate khi file chưa tồn tại |
| 28 | `generate-cert.ps1` | Không set `-NotAfter` → cert hết hạn 1 năm (cần 5-10 năm cho sideload). Cần quyền Admin → chạy local fail |
| 29 | CI matrix | Thiếu ARM64 dù tuyên bố hỗ trợ Lumia 950 |
| 30 | `all-w10m.js:39` | UA giả IEMobile/iPhone → site trả bản mobile cũ; nên dùng UA Firefox mobile (đã có sẵn trong `SettingsService.GetUserAgentString`) |
| 31 | `all-w10m.js:33` | `network.http.pipelining = true` — nhiều server/CDN không hỗ trợ → lỗi kết nối rải rác |

## 5. Vấn đề P3 — Nhỏ / dọn dẹp

- `DocShellBridge.cpp:96-98`: fail cũng set progress = 1.0
- `SpiderMonkeyHost.h`: dùng `Platform::String^` nhưng thiếu include C++/CX → header không self-contained
- `MemoryGuardService.cs:35`: `MemoryPressureDetected` không ai subscribe — dead code
- `SettingsService.cs:45-80`: `UserAgentMode` / `GetUserAgentString` không dùng — dead code
- `MainPage.xaml:71`: text cứng "JIT Status: CodeGeneration Capability Active" trong khi trạng thái thật kiểm tra runtime — gây hiểu nhầm
- `update-root-certs.ps1`: docs nói "chuyển đổi sang nssckbi" nhưng script chỉ download, không convert; `Tls13` enum cần .NET 4.8+
- `build-appx.yml:43`: `-PackagesDirectory packages` là no-op với PackageReference; nên dùng `dotnet restore`
- README ghi `.github/workflows/ci.yml` nhưng thực tế là `build-appx.yml`
- Password cert hardcode `FoxM2026Password!` (key ephemeral, rủi ro thấp, nhưng nên đưa vào secret)
- `BookmarkService`: ghi file sau mỗi thao tác → IO flash thường xuyên, có thể debounce
- `Package.appxmanifest:15`: `PhonePublisherId="00000000-..."` là placeholder

## 6. Điểm tốt

- Dùng `Windows.Data.Json` thay Newtonsoft — đúng hướng zero-dependency cho W10M
- `.gitignore` đầy đủ (bin/obj, mozbuild artifacts)
- Docs kiến trúc & W10M hacks chi tiết, có giá trị tham khảo tốt
- Truncation forward history trong `DocShellBridge::LoadUri` — logic đúng
- Null-check tương đối đầy đủ ở các service

## 7. Lộ trình khắc phục (theo thứ tự ưu tiên)

1. **Nối cấu trúc (P0):** tạo `FoxM.GoannaRuntime.vcxproj` + `.sln` + ProjectReference + xmlns XAML
2. **Sửa C++ (P0):** ref-count SpiderMonkeyHost, HRESULT checks, fallback D3D debug flag, ResizeBuffers
3. **Nối luồng điều hướng:** `MainPage.NavigateTo` → `GoannaView.Navigate`, Back/Forward → `DocShellBridge`
4. **Bảo mật & hiệu năng:** TLS min=3, GC cooldown, cert cố định trong Secrets, màu CSS reader mode
5. **Build engine:** đối chiếu mozconfig với configure UXP thực tế, thêm ARM64 vào CI
