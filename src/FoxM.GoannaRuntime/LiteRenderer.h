#pragma once

// Agent C — LiteRenderer: bộ vẽ placeholder nội bộ (native C++, không phải WinRT ref class).
// Nhận device/context/swapchain HIỆN CÓ + viewport/scroll/zoom, vẽ text thô rồi Present.
// Contract Agent A: KHÔNG tạo swapchain mới mỗi frame; resize do GoannaView::CreateSizeDependentResources
// đảm nhiệm bằng ResizeBuffers + SetSwapChain đúng 1 lần. Ở đây chỉ GetBuffer(0) -> vẽ -> Present.
// Phòng thủ: mọi con trỏ null / HRESULT thất bại đều return lặng lẽ, không crash.
// Chỉ dùng SDK UWP cho phép: d3d11_2.h, dxgi1_3.h, d2d1, dwrite. Không dùng API Win32 cấm (HWND, GDI, file IO).

#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <string>

namespace FoxM
{
    namespace GoannaRuntime
    {
        class LiteRenderer
        {
        public:
            // Body là text/HTML thô (đã strip tags bên trong). Nếu rỗng -> vẽ placeholder.
            // progress: 0.0-1.0 (lấy từ DocShellBridge::Progress) để vẽ thanh tiến trình.
            // titleForHeader: tiêu đề/URL hiển thị dòng header (lấy từ DocShellBridge, có thể rỗng).
            // 7 tham số đầu khớp spec task: device, context, swapChain, w/h, scroll, zoom.
            static void Render(
                ID3D11Device2* device,
                ID3D11DeviceContext2* context,
                IDXGISwapChain1* swapChain,
                UINT viewportWidth,
                UINT viewportHeight,
                float scrollX,
                float scrollY,
                float zoom,
                const std::wstring& htmlBody,
                double progress,
                const std::wstring& titleForHeader);

            // Bản rút gọn: strip tags đơn giản "<...>" -> text, collapse khoảng trắng.
            // Public để GoannaView/tests tái dùng; không throw, không cấp phát quá 64KB.
            static std::wstring StripTags(const std::wstring& html);

        private:
            LiteRenderer() = delete;
        };
    }
}
