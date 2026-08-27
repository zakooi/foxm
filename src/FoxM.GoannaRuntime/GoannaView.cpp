#include "GoannaView.h"
#include <windows.h>

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;

GoannaView::GoannaView() :
    m_isJitEnabled(false)
{
    m_docShell = ref new DocShellBridge();
    m_widget = ref new WidgetUwp();

    // Đăng ký sự kiện Touch & Gesture
    this->PointerPressed += ref new PointerEventHandler(this, &GoannaView::OnPointerPressed);
    this->PointerMoved += ref new PointerEventHandler(this, &GoannaView::OnPointerMoved);
    this->PointerReleased += ref new PointerEventHandler(this, &GoannaView::OnPointerReleased);
    this->SizeChanged += ref new SizeChangedEventHandler(this, &GoannaView::OnSizeChanged);

    InitializeDirectX();
    m_isJitEnabled = SpiderMonkeyHost::AcquireEngine();
}

GoannaView::~GoannaView()
{
    SpiderMonkeyHost::ReleaseEngine();
}

void GoannaView::InitializeDirectX()
{
    CreateDeviceResources();
    CreateSizeDependentResources();
}

void GoannaView::CreateDeviceResources()
{
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel;

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    // Thử tạo device với Debug Layer trước
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        creationFlags | D3D11_CREATE_DEVICE_DEBUG,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );

    // Nếu máy không cài SDK Debug Layer (DXGI_ERROR_SDK_COMPONENT_MISSING), fallback tạo không debug flag
    if (FAILED(hr))
    {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &device,
            &featureLevel,
            &context
        );
    }
#else
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );
#endif

    if (SUCCEEDED(hr) && device && context)
    {
        device.As(&m_d3dDevice);
        context.As(&m_d3dContext);
    }
}

void GoannaView::CreateSizeDependentResources()
{
    if (!m_d3dDevice) return;

    UINT width = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : 480);
    UINT height = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : 800);

    // Nếu SwapChain đã tồn tại, dùng ResizeBuffers để tránh rò rỉ bộ nhớ
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(
            2,
            width,
            height,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
        );

        if (SUCCEEDED(hr))
        {
            m_widget->OnViewportResized(width, height);
            return;
        }

        // Nếu Resize thất bại do device removed/reset, giải phóng để tạo mới
        m_swapChain = nullptr;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
    if (FAILED(m_d3dDevice.As(&dxgiDevice)) || !dxgiDevice) return;

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    if (FAILED(dxgiDevice->GetAdapter(&dxgiAdapter)) || !dxgiAdapter) return;

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    if (FAILED(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))) || !dxgiFactory) return;

    HRESULT hr = dxgiFactory->CreateSwapChainForComposition(
        m_d3dDevice.Get(),
        &swapChainDesc,
        nullptr,
        &m_swapChain
    );

    if (SUCCEEDED(hr) && m_swapChain)
    {
        // Gắn SwapChain vào XAML SwapChainPanel thông qua ISwapChainPanelNative
        reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&m_panelNative));
        if (m_panelNative)
        {
            m_panelNative->SetSwapChain(m_swapChain.Get());
        }
        m_widget->OnViewportResized(width, height);
    }
}

void GoannaView::Navigate(Platform::String^ url)
{
    if (url == nullptr || url->IsEmpty()) return;

    NavigationStarting(url);
    ProgressChanged(0.2);

    m_docShell->LoadUri(url);

    ProgressChanged(0.6);
    TitleChanged(m_docShell->DocumentTitle);

    ProgressChanged(1.0);
    NavigationCompleted(url, true);
}

void GoannaView::GoBack()
{
    if (m_docShell->CanGoBack)
    {
        m_docShell->GoBack();
        NavigationStarting(m_docShell->CurrentUri);
        ProgressChanged(0.5);
        TitleChanged(m_docShell->DocumentTitle);
        ProgressChanged(1.0);
        NavigationCompleted(m_docShell->CurrentUri, true);
    }
}

void GoannaView::GoForward()
{
    if (m_docShell->CanGoForward)
    {
        m_docShell->GoForward();
        NavigationStarting(m_docShell->CurrentUri);
        ProgressChanged(0.5);
        TitleChanged(m_docShell->DocumentTitle);
        ProgressChanged(1.0);
        NavigationCompleted(m_docShell->CurrentUri, true);
    }
}

void GoannaView::Reload()
{
    m_docShell->Reload(false);
    NavigationStarting(m_docShell->CurrentUri);
    ProgressChanged(0.3);
    ProgressChanged(1.0);
    NavigationCompleted(m_docShell->CurrentUri, true);
}

void GoannaView::Stop()
{
    m_docShell->StopLoading();
    ProgressChanged(0.0);
}

void GoannaView::ExecuteScriptAsync(Platform::String^ script)
{
    SpiderMonkeyHost::EvaluateScript(script);
}

void GoannaView::MinimizeMemoryUsage()
{
    SpiderMonkeyHost::RunGarbageCollector();
}

void GoannaView::OnPointerPressed(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_widget->DispatchTouchEvent(TouchEventType::Pressed, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerMoved(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_widget->DispatchTouchEvent(TouchEventType::Moved, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_widget->DispatchTouchEvent(TouchEventType::Released, point->Position.X, point->Position.Y);
}

void GoannaView::OnSizeChanged(Platform::Object^ sender, SizeChangedEventArgs^ e)
{
    CreateSizeDependentResources();
}

bool GoannaView::CanGoBack::get() { return m_docShell ? m_docShell->CanGoBack : false; }
bool GoannaView::CanGoForward::get() { return m_docShell ? m_docShell->CanGoForward : false; }
Platform::String^ GoannaView::CurrentUrl::get() { return m_docShell ? m_docShell->CurrentUri : L"about:blank"; }
Platform::String^ GoannaView::DocumentTitle::get() { return m_docShell ? m_docShell->DocumentTitle : L"FoxM Browser"; }
bool GoannaView::IsJitEnabled::get() { return m_isJitEnabled; }
