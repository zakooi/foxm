#include "WidgetUwp.h"
#include <windows.h>
#include <cmath>
#include <algorithm>

using namespace FoxM::GoannaRuntime;

// Giới hạn hợp lệ (đồng nhất với ZoomLevel clamp 0.25-5.0 trong contract).
static const float kMinZoom = 0.25f;
static const float kMaxZoom = 5.0f;
// Scroll clamp tuyệt đối để tránh tràn số khi content chưa có (placeholder stage).
static const float kMaxAbsScroll = 10000.0f;

WidgetUwp::WidgetUwp() :
    m_viewportWidth(480),
    m_viewportHeight(800),
    m_dpiScale(1.0f),
    m_currentZoom(1.0f),
    m_lastTouchX(0.0f),
    m_lastTouchY(0.0f),
    m_isPointerDown(false),
    m_scrollX(0.0f),
    m_scrollY(0.0f),
    m_contentWidth(480),
    m_contentHeight(3000),
    m_needsRender(true)
{
}

void WidgetUwp::Initialize(int width, int height, float dpiScale)
{
    m_viewportWidth = width > 0 ? width : 480;
    m_viewportHeight = height > 0 ? height : 800;
    m_dpiScale = dpiScale > 0.0f ? dpiScale : 1.0f;
    if (m_contentWidth <= 0) m_contentWidth = m_viewportWidth;
    if (m_contentHeight <= 0) m_contentHeight = m_viewportHeight;
    ClampScroll();
    MarkDirty();
}

void WidgetUwp::ZoomLevel::set(float value)
{
    float clamped = value;
    if (clamped < kMinZoom) clamped = kMinZoom;
    if (clamped > kMaxZoom) clamped = kMaxZoom;
    if (clamped != m_currentZoom)
    {
        m_currentZoom = clamped;
        ClampScroll();
        MarkDirty();
    }
}

void WidgetUwp::DispatchTouchEvent(TouchEventType eventType, float x, float y)
{
    float scaledX = x * m_dpiScale;
    float scaledY = y * m_dpiScale;

    switch (eventType)
    {
    case TouchEventType::Pressed:
        m_isPointerDown = true;
        m_lastTouchX = scaledX;
        m_lastTouchY = scaledY;
        break;

    case TouchEventType::Moved:
        if (m_isPointerDown)
        {
            // Kéo-scroll: ngón tay dịch (dx,dy) thì content offset dịch ngược lại.
            float dx = scaledX - m_lastTouchX;
            float dy = scaledY - m_lastTouchY;
            m_lastTouchX = scaledX;
            m_lastTouchY = scaledY;
            if (dx != 0.0f || dy != 0.0f)
            {
                m_scrollX -= dx;
                m_scrollY -= dy;
                ClampScroll();
                MarkDirty();
            }
        }
        break;

    case TouchEventType::Released:
        m_isPointerDown = false;
        break;

    default:
        break;
    }
}

void WidgetUwp::DispatchWheelEvent(float deltaX, float deltaY, float x, float y)
{
    // Wheel/mouse-pad pan: cộng trực tiếp vào scroll offset rồi clamp.
    // (Ctrl+Wheel pinch-zoom sẽ do tầng XAML/MainPage chuyển thành ZoomLevel; ở đây chỉ pan.)
    (void)x;
    (void)y;
    if (deltaX == 0.0f && deltaY == 0.0f) return;
    // Chuột wheel truyền delta theo tick (thường ±120); quy về pixel với hệ số nhẹ.
    // Touchpad đã truyền pixel thì hệ số 1.0 vẫn an toàn vì có clamp tuyệt đối.
    const float kWheelScale = 1.0f;
    m_scrollX += deltaX * kWheelScale;
    m_scrollY += deltaY * kWheelScale;
    ClampScroll();
    MarkDirty();
}

void WidgetUwp::DispatchKeyEvent(int keyCode, bool isKeyDown)
{
    // Gửi WidgetKeyboardEvent vào Gecko Window focus
    (void)keyCode;
    (void)isKeyDown;
}

void WidgetUwp::OnViewportResized(int width, int height)
{
    if (width > 0) m_viewportWidth = width;
    if (height > 0) m_viewportHeight = height;
    ClampScroll();
    MarkDirty();
}

void WidgetUwp::OnDpiChanged(float newDpiScale)
{
    if (newDpiScale > 0.0f)
    {
        m_dpiScale = newDpiScale;
        MarkDirty();
    }
}

void WidgetUwp::ClearNeedsRender()
{
    m_needsRender = false;
}

void WidgetUwp::SetContentSize(int contentWidth, int contentHeight)
{
    if (contentWidth > 0) m_contentWidth = contentWidth;
    if (contentHeight > 0) m_contentHeight = contentHeight;
    ClampScroll();
    MarkDirty();
}

void WidgetUwp::ClampScroll()
{
    // Content extents scale theo zoom; viewport cố định -> maxScroll >= 0.
    float scaledW = static_cast<float>(m_contentWidth) * m_currentZoom;
    float scaledH = static_cast<float>(m_contentHeight) * m_currentZoom;
    float maxX = scaledW - static_cast<float>(m_viewportWidth);
    float maxY = scaledH - static_cast<float>(m_viewportHeight);
    if (maxX < 0.0f) maxX = 0.0f;
    if (maxY < 0.0f) maxY = 0.0f;
    if (maxX > kMaxAbsScroll) maxX = kMaxAbsScroll;
    if (maxY > kMaxAbsScroll) maxY = kMaxAbsScroll;

    m_scrollX = std::max(0.0f, std::min(m_scrollX, maxX));
    m_scrollY = std::max(0.0f, std::min(m_scrollY, maxY));
}
