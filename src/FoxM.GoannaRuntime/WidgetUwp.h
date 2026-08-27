#pragma once

namespace FoxM
{
    namespace GoannaRuntime
    {
        public enum class TouchEventType
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
        public ref class WidgetUwp sealed
        {
        public:
            WidgetUwp();

            void Initialize(int width, int height, float dpiScale);
            void DispatchTouchEvent(TouchEventType eventType, float x, float y);
            void DispatchWheelEvent(float deltaX, float deltaY, float x, float y);
            void DispatchKeyEvent(int keyCode, bool isKeyDown);
            void OnViewportResized(int width, int height);
            void OnDpiChanged(float newDpiScale);

            property float ZoomLevel
            {
                float get() { return m_currentZoom; }
                void set(float value)
                {
                    if (value >= 0.25f && value <= 5.0f)
                    {
                        m_currentZoom = value;
                    }
                }
            }

        private:
            int m_viewportWidth;
            int m_viewportHeight;
            float m_dpiScale;
            float m_currentZoom;
            float m_lastTouchX;
            float m_lastTouchY;
            bool m_isPointerDown;
        };
    }
}
