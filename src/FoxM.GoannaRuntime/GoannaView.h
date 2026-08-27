#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <windows.ui.xaml.media.dxinterop.h>

namespace FoxM
{
    namespace GoannaRuntime
    {
        public delegate void NavigationCompletedEventHandler(Platform::String^ url, bool success);
        public delegate void TitleChangedEventHandler(Platform::String^ newTitle);
        public delegate void ProgressChangedEventHandler(double progress);

        /// <summary>
        /// Control XAML chính bọc nhân Goanna Engine, kế thừa từ SwapChainPanel.
        /// Cho phép nhúng trực tiếp vào giao diện C# XAML của Windows 10 Mobile.
        /// </summary>
        [Windows::Foundation::Metadata::WebHostHidden]
        public ref class GoannaView sealed : public Windows::UI::Xaml::Controls::SwapChainPanel
        {
        public:
            GoannaView();
            virtual ~GoannaView();

            // Điều khiển Duyệt Web
            void Navigate(Platform::String^ url);
            void GoBack();
            void GoForward();
            void Refresh();
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

            Microsoft::WRL::ComPtr<ID3D11Device2> m_d3dDevice;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_d3dContext;
            Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
            Microsoft::WRL::ComPtr<ISwapChainPanelNative> m_panelNative;

            Platform::String^ m_currentUrl;
            Platform::String^ m_documentTitle;
            bool m_canGoBack;
            bool m_canGoForward;
            bool m_isJitEnabled;
        };
    }
}
