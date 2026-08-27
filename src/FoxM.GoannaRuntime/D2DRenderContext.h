#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1effects_1.h>
#include <dwrite_2.h>
#include <string>

namespace FoxM
{
    namespace GoannaRuntime
    {
        /// <summary>
        /// Bộ kết xuất đồ họa Direct2D 1.2 & DirectWrite tăng tốc phần cứng cho Goanna Compositor.
        /// Vẽ trực tiếp layout hộp CSS, văn bản sắc nét và cuộn cảm ứng 60 FPS lên SwapChain.
        /// </summary>
        class D2DRenderContext
        {
        public:
            D2DRenderContext();
            ~D2DRenderContext();

            bool Initialize(ID3D11Device* d3dDevice, IDXGISwapChain1* swapChain, float dpiScale = 1.0f);
            void Resize(IDXGISwapChain1* swapChain, UINT width, UINT height);
            
            void BeginDraw();
            void EndDraw();
            void Clear(D2D1_COLOR_F color);

            void DrawText(
                const std::wstring& text,
                float x,
                float y,
                float width,
                float height,
                float fontSize = 16.0f,
                bool isBold = false,
                D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::White)
            );

            void FillRectangle(
                float x,
                float y,
                float width,
                float height,
                D2D1_COLOR_F color
            );

            void DrawRectangle(
                float x,
                float y,
                float width,
                float height,
                D2D1_COLOR_F color,
                float strokeWidth = 1.0f
            );

            void DrawLine(
                float x1,
                float y1,
                float x2,
                float y2,
                D2D1_COLOR_F color,
                float strokeWidth = 1.0f
            );

            void SetScrollOffset(float scrollY);
            float GetScrollOffset() const { return m_scrollOffsetY; }

        private:
            Microsoft::WRL::ComPtr<ID2D1Factory2> m_d2dFactory;
            Microsoft::WRL::ComPtr<ID2D1Device1> m_d2dDevice;
            Microsoft::WRL::ComPtr<ID2D1DeviceContext1> m_d2dContext;
            Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_d2dTargetBitmap;
            Microsoft::WRL::ComPtr<IDWriteFactory2> m_dwriteFactory;
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;

            float m_dpiScale;
            float m_scrollOffsetY;
            bool m_isDrawing;
        };
    }
}
