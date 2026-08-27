#include "D2DRenderContext.h"
#include <dxgi1_3.h>

using namespace FoxM::GoannaRuntime;
using namespace Microsoft::WRL;

D2DRenderContext::D2DRenderContext() :
    m_dpiScale(1.0f),
    m_scrollOffsetY(0.0f),
    m_isDrawing(false)
{
}

D2DRenderContext::~D2DRenderContext()
{
}

bool D2DRenderContext::Initialize(ID3D11Device* d3dDevice, IDXGISwapChain1* swapChain, float dpiScale)
{
    if (!d3dDevice || !swapChain) return false;
    m_dpiScale = dpiScale > 0.0f ? dpiScale : 1.0f;

    // 1. Tạo Direct2D Factory 2
    D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#if defined(_DEBUG)
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory2),
        &options,
        reinterpret_cast<void**>(m_d2dFactory.GetAddressOf())
    );
    if (FAILED(hr)) return false;

    // 2. Tạo DirectWrite Factory 2
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory2),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );
    if (FAILED(hr)) return false;

    // 3. Tạo D2D Device từ D3D11 Device
    ComPtr<IDXGIDevice3> dxgiDevice;
    if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) return false;

    hr = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
    if (FAILED(hr)) return false;

    hr = m_d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &m_d2dContext
    );
    if (FAILED(hr)) return false;

    // 4. Tạo Solid Color Brush
    hr = m_d2dContext->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        &m_brush
    );
    if (FAILED(hr)) return false;

    // 5. Gắn Backbuffer của Swapchain làm Target Bitmap
    Resize(swapChain, 0, 0);

    return true;
}

void D2DRenderContext::Resize(IDXGISwapChain1* swapChain, UINT width, UINT height)
{
    if (!m_d2dContext || !swapChain) return;

    // Giải phóng target bitmap cũ trước khi resize
    m_d2dContext->SetTarget(nullptr);
    m_d2dTargetBitmap = nullptr;

    ComPtr<IDXGISurface2> dxgiBackBuffer;
    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (SUCCEEDED(hr) && dxgiBackBuffer)
    {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f * m_dpiScale,
            96.0f * m_dpiScale
        );

        hr = m_d2dContext->CreateBitmapFromDxgiSurface(
            dxgiBackBuffer.Get(),
            &bitmapProperties,
            &m_d2dTargetBitmap
        );

        if (SUCCEEDED(hr) && m_d2dTargetBitmap)
        {
            m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());
            m_d2dContext->SetDpi(96.0f * m_dpiScale, 96.0f * m_dpiScale);
        }
    }
}

void D2DRenderContext::BeginDraw()
{
    if (m_d2dContext && !m_isDrawing)
    {
        m_d2dContext->BeginDraw();
        m_isDrawing = true;
    }
}

void D2DRenderContext::EndDraw()
{
    if (m_d2dContext && m_isDrawing)
    {
        m_d2dContext->EndDraw();
        m_isDrawing = false;
    }
}

void D2DRenderContext::Clear(D2D1_COLOR_F color)
{
    if (m_d2dContext && m_isDrawing)
    {
        m_d2dContext->Clear(color);
    }
}

void D2DRenderContext::DrawText(
    const std::wstring& text,
    float x,
    float y,
    float width,
    float height,
    float fontSize,
    bool isBold,
    D2D1_COLOR_F color)
{
    if (!m_d2dContext || !m_dwriteFactory || !m_brush || !m_isDrawing || text.empty()) return;

    ComPtr<IDWriteTextFormat> textFormat;
    HRESULT hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        isBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"vi-VN",
        &textFormat
    );

    if (SUCCEEDED(hr) && textFormat)
    {
        m_brush->SetColor(color);
        D2D1_RECT_F layoutRect = D2D1::RectF(x, y - m_scrollOffsetY, x + width, y + height - m_scrollOffsetY);

        m_d2dContext->DrawText(
            text.c_str(),
            static_cast<UINT32>(text.length()),
            textFormat.Get(),
            layoutRect,
            m_brush.Get()
        );
    }
}

void D2DRenderContext::FillRectangle(float x, float y, float width, float height, D2D1_COLOR_F color)
{
    if (!m_d2dContext || !m_brush || !m_isDrawing) return;

    m_brush->SetColor(color);
    D2D1_RECT_F rect = D2D1::RectF(x, y - m_scrollOffsetY, x + width, y + height - m_scrollOffsetY);
    m_d2dContext->FillRectangle(rect, m_brush.Get());
}

void D2DRenderContext::DrawRectangle(float x, float y, float width, float height, D2D1_COLOR_F color, float strokeWidth)
{
    if (!m_d2dContext || !m_brush || !m_isDrawing) return;

    m_brush->SetColor(color);
    D2D1_RECT_F rect = D2D1::RectF(x, y - m_scrollOffsetY, x + width, y + height - m_scrollOffsetY);
    m_d2dContext->DrawRectangle(rect, m_brush.Get(), strokeWidth);
}

void D2DRenderContext::DrawLine(float x1, float y1, float x2, float y2, D2D1_COLOR_F color, float strokeWidth)
{
    if (!m_d2dContext || !m_brush || !m_isDrawing) return;

    m_brush->SetColor(color);
    D2D1_POINT_2F p1 = D2D1::Point2F(x1, y1 - m_scrollOffsetY);
    D2D1_POINT_2F p2 = D2D1::Point2F(x2, y2 - m_scrollOffsetY);
    m_d2dContext->DrawLine(p1, p2, m_brush.Get(), strokeWidth);
}

void D2DRenderContext::SetScrollOffset(float scrollY)
{
    m_scrollOffsetY = scrollY >= 0.0f ? scrollY : 0.0f;
}
