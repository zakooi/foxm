#include "WidgetUwp.h"
#include <windows.h>
#include <cmath>

using namespace FoxM::GoannaRuntime;

WidgetUwp::WidgetUwp() :
    m_viewportWidth(480),
    m_viewportHeight(800),
    m_dpiScale(1.0f),
    m_currentZoom(1.0f),
    m_lastTouchX(0.0f),
    m_lastTouchY(0.0f),
    m_isPointerDown(false)
{
}

void WidgetUwp::Initialize(int width, int height, float dpiScale)
{
    m_viewportWidth = width > 0 ? width : 480;
    m_viewportHeight = height > 0 ? height : 800;
    m_dpiScale = dpiScale > 0.0f ? dpiScale : 1.0f;
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
            m_lastTouchX = scaledX;
            m_lastTouchY = scaledY;
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
    // Chuyển đổi Wheel Delta sang Pan scroll cho Layout Engine
    (void)deltaX;
    (void)deltaY;
    (void)x;
    (void)y;
}

void WidgetUwp::DispatchKeyEvent(int keyCode, bool isKeyDown)
{
    // Gửi WidgetKeyboardEvent vào Gecko Window focus
    (void)keyCode;
    (void)isKeyDown;
}

void WidgetUwp::OnViewportResized(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void WidgetUwp::OnDpiChanged(float newDpiScale)
{
    m_dpiScale = newDpiScale;
}
