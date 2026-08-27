#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include <memory>

#include "DocShellBridge.h"
#include "WidgetUwp.h"
#include "SpiderMonkeyHost.h"
#include "D2DRenderContext.h"
#include "NeckoClient.h"
#include "GoannaDOMParser.h"

namespace FoxM
{
    namespace GoannaRuntime
    {
        public delegate void NavigationStartingEventHandler(Platform::String^ url);
        public delegate void NavigationCompletedEventHandler(Platform::String^ url, bool success);
        public delegate void TitleChangedEventHandler(Platform::String^ newTitle);
        public delegate void ProgressChangedEventHandler(double progress);

        /// <summary>
        /// Control XAML chính bọc nhân Goanna Engine với Direct2D/DirectWrite Hardware Compositor 60 FPS.
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
            void Reload();
            void Stop();

            // JavaScript Interop
            void ExecuteScriptAsync(Platform::String^ script);

            // Quản lý Bộ nhớ & Thu gom rác
            void MinimizeMemoryUsage();

            // Vẽ lại toàn bộ trang lên SwapChain
            void Render();

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

            Microsoft::WRL::ComPtr<ID3D11Device2> m_d3dDevice;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_d3dContext;
            Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
            Microsoft::WRL::ComPtr<ISwapChainPanelNative> m_panelNative;

            DocShellBridge^ m_docShell;
            WidgetUwp^ m_widget;
            NeckoClient^ m_neckoClient;

            std::unique_ptr<D2DRenderContext> m_renderContext;
            std::unique_ptr<GoannaDOMParser> m_domParser;

            bool m_isJitEnabled;
            bool m_isPointerDown;
            float m_lastPointerY;
            float m_scrollOffset;
        };
    }
}
