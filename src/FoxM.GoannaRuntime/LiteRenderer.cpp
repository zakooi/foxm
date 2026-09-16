#include "LiteRenderer.h"

#include <wrl/client.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <algorithm>
#include <mutex>

using Microsoft::WRL::ComPtr;

namespace FoxM
{
    namespace GoannaRuntime
    {
        namespace
        {
            std::once_flag g_d2dInitFlag;
            ComPtr<ID2D1Factory1> g_d2dFactory;
            ComPtr<IDWriteFactory> g_dwriteFactory;
            bool g_factoriesReady = false;

            void EnsureFactories()
            {
                std::call_once(g_d2dInitFlag, []()
                {
                    D2D1_FACTORY_OPTIONS opts = {};
#if defined(_DEBUG)
                    opts.debugLevel = D2D1_DEBUG_LEVEL_WARNING;
#else
                    opts.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif
                    ComPtr<ID2D1Factory1> f;
                    if (SUCCEEDED(D2D1CreateFactory(
                        D2D1_FACTORY_TYPE_SINGLE_THREADED,
                        __uuidof(ID2D1Factory1),
                        &opts,
                        &f)) && f)
                    {
                        g_d2dFactory = f;
                    }
                    ComPtr<IDWriteFactory> wf;
                    if (SUCCEEDED(DWriteCreateFactory(
                        DWRITE_FACTORY_TYPE_SHARED,
                        __uuidof(IDWriteFactory),
                        &wf)) && wf)
                    {
                        g_dwriteFactory = wf;
                    }
                    g_factoriesReady = (g_d2dFactory && g_dwriteFactory);
                });
            }

            float ClampF(float v, float lo, float hi)
            {
                if (v < lo) return lo;
                if (v > hi) return hi;
                return v;
            }
        }

        std::wstring LiteRenderer::StripTags(const std::wstring& html)
        {
            // Giới hạn đầu vào để tránh TextLayout khổng lồ khi body còn là HTML nguyên trang.
            const size_t kMaxIn = 32768;
            const size_t kMaxOut = 8192;
            size_t n = std::min(html.size(), kMaxIn);
            std::wstring out;
            out.reserve(std::min(n, kMaxOut));
            bool inTag = false;
            bool lastWasSpace = true; // collapse leading space
            for (size_t i = 0; i < n && out.size() < kMaxOut; ++i)
            {
                wchar_t c = html[i];
                if (inTag)
                {
                    if (c == L'>') inTag = false;
                    continue;
                }
                if (c == L'<')
                {
                    inTag = true;
                    continue;
                }
                if (c == L'\r' || c == L'\n' || c == L'\t' || c == L' ')
                {
                    if (!lastWasSpace)
                    {
                        out.push_back(L' ');
                        lastWasSpace = true;
                    }
                    continue;
                }
                out.push_back(c);
                lastWasSpace = false;
            }
            // Trim trailing space.
            while (!out.empty() && out.back() == L' ') out.pop_back();
            return out;
        }

        void LiteRenderer::Render(
            ID3D11Device2* /*device*/,
            ID3D11DeviceContext2* /*context*/,
            IDXGISwapChain1* swapChain,
            UINT viewportWidth,
            UINT viewportHeight,
            float scrollX,
            float scrollY,
            float zoom,
            const std::wstring& htmlBody,
            double progress,
            const std::wstring& titleForHeader)
        {
            if (!swapChain) return;
            if (viewportWidth == 0 || viewportHeight == 0) return;

            EnsureFactories();
            if (!g_factoriesReady) return;

            float w = static_cast<float>(viewportWidth);
            float h = static_cast<float>(viewportHeight);
            scrollX = ClampF(scrollX, 0.0f, 10000.0f);
            scrollY = ClampF(scrollY, 0.0f, 10000.0f);
            zoom = ClampF(zoom, 0.25f, 5.0f);
            if (progress < 0.0) progress = 0.0;
            if (progress > 1.0) progress = 1.0;

            // Lấy DXGI surface của backbuffer (KHÔNG tạo swapchain mới).
            ComPtr<IDXGISurface> dxgiSurface;
            HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));
            if (FAILED(hr) || !dxgiSurface) return;

            D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
            ComPtr<ID2D1RenderTarget> rt;
            hr = g_d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface.Get(), &rtProps, &rt);
            if (FAILED(hr) || !rt) return;

            // Chuẩn bị text.
            std::wstring plain = StripTags(htmlBody);
            bool isPlaceholder = plain.empty();
            std::wstring bodyText = isPlaceholder
                ? L"FoxM Goanna — placeholder render (Agent C). Network/JS chua cap nhat noi dung; doi DocShellBridge::GetLastBody. Keo de scroll, pinch/wheel de kiem tra clamp."
                : plain;

            std::wstring header = titleForHeader.empty() ? L"FoxM — about:blank" : titleForHeader;

            float baseFont = ClampF(15.0f * zoom, 8.0f, 64.0f);
            float headerFont = ClampF(13.0f * zoom, 8.0f, 40.0f);

            ComPtr<IDWriteTextFormat> bodyFormat;
            hr = g_dwriteFactory->CreateTextFormat(
                L"Segoe UI", nullptr,
                DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                baseFont, L"en-us", &bodyFormat);
            if (FAILED(hr) || !bodyFormat) return;
            bodyFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
            bodyFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            bodyFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            ComPtr<IDWriteTextFormat> headerFormat;
            hr = g_dwriteFactory->CreateTextFormat(
                L"Segoe UI", nullptr,
                DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                headerFont, L"en-us", &headerFormat);
            if (FAILED(hr) || !headerFormat) return;
            headerFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            headerFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

            // Vẽ.
            rt->BeginDraw();
            // Nền #121214.
            rt->Clear(D2D1::ColorF(0x12 / 255.0f, 0x12 / 255.0f, 0x14 / 255.0f, 1.0f));

            ComPtr<ID2D1SolidColorBrush> textBrush;
            ComPtr<ID2D1SolidColorBrush> dimBrush;
            ComPtr<ID2D1SolidColorBrush> accentBrush;
            ComPtr<ID2D1SolidColorBrush> barBackBrush;
            if (FAILED(rt->CreateSolidColorBrush(D2D1::ColorF(0xE8 / 255.0f, 0xE8 / 255.0f, 0xE8 / 255.0f, 1.0f), &textBrush)) || !textBrush)
            {
                rt->EndDraw();
                return;
            }
            rt->CreateSolidColorBrush(D2D1::ColorF(0x9A / 255.0f, 0x9A / 255.0f, 0xA0 / 255.0f, 1.0f), &dimBrush);
            rt->CreateSolidColorBrush(D2D1::ColorF(0x4C / 255.0f, 0x8C / 255.0f, 0xF5 / 255.0f, 1.0f), &accentBrush);
            rt->CreateSolidColorBrush(D2D1::ColorF(0x2A / 255.0f, 0x2A / 255.0f, 0x2E / 255.0f, 1.0f), &barBackBrush);

            const float kPad = 16.0f;
            const float kHeaderH = 28.0f * zoom + 12.0f;
            const float kBarH = 4.0f;

            // Header.
            if (dimBrush)
            {
                D2D1_RECT_F headerBg = D2D1::RectF(0.0f, 0.0f, w, kHeaderH);
                rt->FillRectangle(&headerBg, dimBrush.Get());
            }
            D2D1_RECT_F headerRect = D2D1::RectF(kPad, 6.0f, w - kPad, kHeaderH - 4.0f);
            rt->DrawTextW(header.c_str(), static_cast<UINT32>(header.length()), headerFormat.Get(), &headerRect, textBrush.Get());

            // Thanh progress ngay dưới header (dùng ProgressChanged/DocShellBridge::Progress).
            float barY = kHeaderH;
            if (barBackBrush)
            {
                D2D1_RECT_F back = D2D1::RectF(0.0f, barY, w, barY + kBarH);
                rt->FillRectangle(&back, barBackBrush.Get());
            }
            if (accentBrush && progress > 0.0)
            {
                D2D1_RECT_F fill = D2D1::RectF(0.0f, barY, w * static_cast<float>(progress), barY + kBarH);
                rt->FillRectangle(&fill, accentBrush.Get());
            }

            // Vùng body: áp scroll offset (chữ chạy lên khi scrollY tăng).
            float bodyTop = barY + kBarH + kPad - scrollY;
            float bodyLeft = kPad - scrollX;
            D2D1_RECT_F bodyRect = D2D1::RectF(bodyLeft, bodyTop, w - kPad, h - kPad);
            // Cắt dưới header để text không đè lên header/progress.
            rt->PushAxisAlignedClip(D2D1::RectF(0.0f, barY + kBarH, w, h), D2D1_ANTIALIASED_MODE_ALIASED);
            rt->DrawTextW(bodyText.c_str(), static_cast<UINT32>(bodyText.length()), bodyFormat.Get(), &bodyRect, textBrush.Get());
            rt->PopAxisAlignedClip();

            // Dòng debug nhỏ góc dưới (scroll/zoom) — hữu ích khi chưa có DevTools.
            if (dimBrush)
            {
                wchar_t dbg[128] = {};
                // swprintf_s là UWP-allowed (không phải Win32 cấm); fallback an toàn nếu lỗi.
                if (swprintf_s(dbg, L"scroll=(%.0f, %.0f) zoom=%.2f %ls", (double)scrollX, (double)scrollY, (double)zoom,
                    isPlaceholder ? L"placeholder" : L"") > 0)
                {
                    D2D1_RECT_F dbgRect = D2D1::RectF(kPad, h - 24.0f, w - kPad, h - 6.0f);
                    rt->DrawTextW(dbg, static_cast<UINT32>(wcslen(dbg)), headerFormat.Get(), &dbgRect, dimBrush.Get());
                }
            }

            hr = rt->EndDraw();
            if (FAILED(hr))
            {
                return; // Khung sau sẽ vẽ lại; không crash.
            }

            // Present verdical-sync (KHÔNG tạo swapchain mới, KHÔNG ResizeBuffers ở đây).
            swapChain->Present(1, 0);
        }
    }
}
