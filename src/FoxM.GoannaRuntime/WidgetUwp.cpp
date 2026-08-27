#include "WidgetUwp.h"
#include <windows.h>
#include <cmath>

using namespace FoxM::GoannaRuntime;

int WidgetUwp::s_viewportWidth = 480;
int WidgetUwp::s_viewportHeight = 800;
float WidgetUwp::s_dpiScale = 1.0f;
float WidgetUwp::s_currentZoom = 1.0f;
float WidgetUwp::s_lastTouchX = 0.0f;
float WidgetUwp::s_lastTouchY = 0.0f;
bool WidgetUwp::s_isPointerDown = false;

void WidgetUwp::InitializeWidget(int width, int height, float dpiScale)
{
    s_viewportWidth = width > 0 ? width : 480;
    s_viewportHeight = height > 0 ? height : 800;
    s_dpiScale = dpiScale > 0.0f ? dpiScale : 1.0f;
}

void WidgetUwp::DispatchTouchEvent(int eventType, float x, float y)
{
    float scaledX = x * s_dpiScale;
    float scaledY = y * s_dpiScale;

    switch (static_cast<TouchEventType>(eventType))
    {
    case TouchEventType::Pressed:
        s_isPointerDown = true;
        s_lastTouchX = scaledX;
        s_lastTouchY = scaledY;
        // Gửi WidgetGUIEvent (eMouseDown / eTouchStart) vào Gecko Event Queue
        break;

    case TouchEventType::Moved:
        if (s_isPointerDown)
        {
            float deltaX = scaledX - s_lastTouchX;
            float deltaY = scaledY - s_lastTouchY;
            // Tính toán cuộn quán tính (Kinetic Pan & Scroll)
            s_lastTouchX = scaledX;
            s_lastTouchY = scaledY;
        }
        break;

    case TouchEventType::Released:
        s_isPointerDown = false;
        // Gửi WidgetGUIEvent (eMouseUp / eTouchEnd)
        break;
    }
}

void WidgetUwp::DispatchWheelEvent(float deltaX, float deltaY, float x, float y)
{
    // Chuyển đổi Wheel Delta sang Pan scroll cho Layout Engine
}

void WidgetUwp::DispatchKeyEvent(int keyCode, bool isKeyDown)
{
    // Gửi WidgetKeyboardEvent vào Gecko Window focus
}

void WidgetUwp::OnViewportResized(int width, int height)
{
    s_viewportWidth = width;
    s_viewportHeight = height;
}

void WidgetUwp::OnDpiChanged(float newDpiScale)
{
    s_dpiScale = newDpiScale;
}

float WidgetUwp::GetCurrentZoomLevel()
{
    return s_currentZoom;
}

void WidgetUwp::SetZoomLevel(float zoom)
{
    if (zoom >= 0.25f && zoom <= 5.0f)
    {
        s_currentZoom = zoom;
    }
}
