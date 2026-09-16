#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <windows.ui.xaml.media.dxinterop.h>

#include "DocShellBridge.h"
#include "WidgetUwp.h"
#include "SpiderMonkeyHost.h"

namespace FoxM
{
    namespace GoannaRuntime
    {
        public delegate void NavigationStartingEventHandler(Platform::String^ url);
        public delegate void NavigationCompletedEventHandler(Platform::String^ url, bool success);
        public delegate void TitleChangedEventHandler(Platform::String^ newTitle);
        public delegate void ProgressChangedEventHandler(double progress);

        // =====================================================================
        // AGENT CONTRACTS (do Agent A chốt — các agent khác implement theo,
        // KHÔNG đổi signature/event đã chốt bên dưới nếu chưa bàn với Agent A):
        //
        // (a) NETWORK (Agent Network owns DocShellBridge.cpp):
        //     - DocShellBridge::LoadUri(Platform::String^ uri) là ASYNC
        //       (không block UI thread; fetch chạy background).
        //     - Tiến trình báo về qua internal DocShellBridge::UpdateProgress(double 0.0-1.0)
        //       và DocShellBridge::OnNavigationComplete(bool success).
        //     - GoannaView chỉ forward sang events WinRT: ProgressChanged /
        //       TitleChanged / NavigationCompleted. Không poll, không block.
        //
        // (b) RENDER (Agent Render owns WidgetUwp.cpp + phần vẽ trong GoannaView.cpp):
        //     - Hàm vẽ TƯƠNG LAI phải có dạng:
        //         void RenderFrame(IDXGISwapChain1* swapChain, UINT viewportWidth, UINT viewportHeight);
        //       nhận swapchain HIỆN CÓ + viewport w/h, vẽ vào backbuffer rồi Present.
        //     - TUYỆT ĐỐI KHÔNG tạo swapchain mới mỗi frame; resize dùng
        //       IDXGISwapChain1::ResizeBuffers như CreateSizeDependentResources đã làm.
        //     - Gắn panel duy nhất 1 lần qua ISwapChainPanelNative::SetSwapChain.
        //
        // (c) JS (Agent JS owns SpiderMonkeyHost.cpp):
        //     - GoannaView::ExecuteScriptAsync(Platform::String^ script) nhận
        //       UTF-16 string (Platform::String^), KHÔNG block UI thread
        //       (dispatch sang background/thread JS rồi return ngay).
        //     - SpiderMonkeyHost::EvaluateScript giữ nguyên signature;
        //       GC dọn qua RunGarbageCollector/MinimizeMemoryUsage.
        // =====================================================================

        /// <summary>
        /// Control XAML chính bọc nhân Goanna Engine, kế thừa từ SwapChainPanel.
        /// Cho phép nhúng trực tiếp vào giao diện C# XAML của Windows 10 Mobile.
        /// </summary>
        public ref class GoannaView sealed : public Windows::UI::Xaml::Controls::SwapChainPanel
        {
        public:
            GoannaView();
            virtual ~GoannaView();

            // Điều khiển Duyệt Web
            void Navigate(Platform::String^ url);
            void GoBack();
            void GoForward();
            void Reload();
            void Stop();

            // JavaScript Interop
            void ExecuteScriptAsync(Platform::String^ script);

            // Quản lý Bộ nhớ & Thu gom rác
            void MinimizeMemoryUsage();

            // Properties
            property bool CanGoBack { bool get(); }
            property bool CanGoForward { bool get(); }
            property Platform::String^ CurrentUrl { Platform::String^ get(); }
            property Platform::String^ DocumentTitle { Platform::String^ get(); }
            property bool IsJitEnabled { bool get(); }

            // Events
            event NavigationStartingEventHandler^ NavigationStarting;
            event NavigationCompletedEventHandler^ NavigationCompleted;
            event TitleChangedEventHandler^ TitleChanged;
            event ProgressChangedEventHandler^ ProgressChanged;

        private:
            void InitializeDirectX();
            void CreateDeviceResources();
            void CreateSizeDependentResources();
            void OnPointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
            void OnPointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
            void OnPointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
            void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
            // Merge: nhận completion async từ DocShellBridge (đã marshal về UI thread),
            // forward sang events WinRT + vẽ lại. Không gọi trực tiếp từ background thread.
            void OnLoadEvent(double progress, bool completed, bool success);

            Microsoft::WRL::ComPtr<ID3D11Device2> m_d3dDevice;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_d3dContext;
            Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
            Microsoft::WRL::ComPtr<ISwapChainPanelNative> m_panelNative;

            DocShellBridge^ m_docShell;
            WidgetUwp^ m_widget;
            bool m_isJitEnabled;
        };
    }
}
