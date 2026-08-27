#pragma once

namespace FoxM
{
    namespace GoannaRuntime
    {
        enum class TouchEventType
        {
            Pressed = 0,
            Moved = 1,
            Released = 2,
            Wheel = 3
        };

        /// <summary>
        /// Tầng trừu tượng Widget thay thế cho nsWindow Win32 truyền thống.
        /// Chuyển đổi trực tiếp các tương tác Touch, Kinetic Scroll và Pinch-to-Zoom từ XAML sang Goanna Core.
        /// </summary>
        class WidgetUwp
        {
        public:
            static void InitializeWidget(int width, int height, float dpiScale);
            static void DispatchTouchEvent(int eventType, float x, float y);
            static void DispatchWheelEvent(float deltaX, float deltaY, float x, float y);
            static void DispatchKeyEvent(int keyCode, bool isKeyDown);
            static void OnViewportResized(int width, int height);
            static void OnDpiChanged(float newDpiScale);

            static float GetCurrentZoomLevel();
            static void SetZoomLevel(float zoom);

        private:
            static int s_viewportWidth;
            static int s_viewportHeight;
            static float s_dpiScale;
            static float s_currentZoom;
            static float s_lastTouchX;
            static float s_lastTouchY;
            static bool s_isPointerDown;
        };
    }
}
