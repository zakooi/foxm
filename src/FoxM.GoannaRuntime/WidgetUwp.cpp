#include "WidgetUwp.h"
#include <windows.h>

using namespace FoxM::GoannaRuntime;

void WidgetUwp::DispatchTouchEvent(int eventType, float x, float y)
{
    // Chuyển đổi sang WidgetTouchEvent của Gecko và đẩy vào nsIWidget Event Queue
}

void WidgetUwp::DispatchKeyEvent(int keyCode, bool isKeyDown)
{
    // Chuyển đổi sang WidgetKeyboardEvent của Gecko
}

void WidgetUwp::OnViewportResized(int width, int height)
{
    // Cập nhật Viewport bounds cho Gecko Layout Engine
}
