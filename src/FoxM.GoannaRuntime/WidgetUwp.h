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
        /// Agent C: toàn bộ state là member per-instance (mỗi tab một WidgetUwp),
        /// KHÔNG dùng static dùng chung. Scroll/zoom per-instance, clamp an toàn.
        /// </summary>
        public ref class WidgetUwp sealed
        {
        public:
            WidgetUwp();

            // API public đã chốt — giữ nguyên signature.
            void Initialize(int width, int height, float dpiScale);
            void DispatchTouchEvent(TouchEventType eventType, float x, float y);
            void DispatchWheelEvent(float deltaX, float deltaY, float x, float y);
            void DispatchKeyEvent(int keyCode, bool isKeyDown);
            void OnViewportResized(int width, int height);
            void OnDpiChanged(float newDpiScale);

            property float ZoomLevel
            {
                float get() { return m_currentZoom; }
                void set(float value);
            }

            // BỔ SUNG (additive, phục vụ render — không đổi signature cũ):
            // GoannaView.cpp poll các property này rồi gọi RenderFrame (không tạo thread mới).
            property float ScrollX { float get() { return m_scrollX; } }
            property float ScrollY { float get() { return m_scrollY; } }
            property int ViewportWidth { int get() { return m_viewportWidth; } }
            property int ViewportHeight { int get() { return m_viewportHeight; } }
            property bool NeedsRender { bool get() { return m_needsRender; } }
            void ClearNeedsRender();
            void SetContentSize(int contentWidth, int contentHeight);

        private:
            void ClampScroll();
            void MarkDirty() { m_needsRender = true; }

            int m_viewportWidth;
            int m_viewportHeight;
            float m_dpiScale;
            float m_currentZoom;
            float m_lastTouchX;
            float m_lastTouchY;
            bool m_isPointerDown;
            // Per-instance scroll/zoom state (fix bug P2-26: trước đây static dùng chung mọi tab).
            float m_scrollX;
            float m_scrollY;
            int m_contentWidth;
            int m_contentHeight;
            bool m_needsRender;
        };
    }
}
