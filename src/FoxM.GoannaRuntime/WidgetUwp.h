#pragma once

namespace FoxM
{
    namespace GoannaRuntime
    {
        /// <summary>
        /// Tầng trừu tượng Widget thay thế cho nsWindow Win32 truyền thống.
        /// Chuyển đổi trực tiếp các tương tác từ XAML sang Goanna Core.
        /// </summary>
        class WidgetUwp
        {
        public:
            static void DispatchTouchEvent(int eventType, float x, float y);
            static void DispatchKeyEvent(int keyCode, bool isKeyDown);
            static void OnViewportResized(int width, int height);
        };
    }
}
