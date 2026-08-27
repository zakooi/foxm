#include "GoannaView.h"
#include "SpiderMonkeyHost.h"
#include "WidgetUwp.h"

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;

GoannaView::GoannaView() :
    m_canGoBack(false),
    m_canGoForward(false),
    m_isJitEnabled(false),
    m_currentUrl(L"about:blank"),
    m_documentTitle(L"FoxM Browser")
{
    // Đăng ký sự kiện Touch & Gesture
    this->PointerPressed += ref new PointerEventHandler(this, &GoannaView::OnPointerPressed);
    this->PointerMoved += ref new PointerEventHandler(this, &GoannaView::OnPointerMoved);
    this->PointerReleased += ref new PointerEventHandler(this, &GoannaView::OnPointerReleased);
    this->SizeChanged += ref new SizeChangedEventHandler(this, &GoannaView::OnSizeChanged);

    InitializeDirectX();
    m_isJitEnabled = SpiderMonkeyHost::InitializeEngine();
}

GoannaView::~GoannaView()
{
    SpiderMonkeyHost::ShutdownEngine();
}

void GoannaView::InitializeDirectX()
{
    CreateDeviceResources();
    CreateSizeDependentResources();
}

void GoannaView::CreateDeviceResources()
{
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3
    };

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel;

    D3D11CreateDevice(
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

    device.As(&m_d3dDevice);
    context.As(&m_d3dContext);
}

void GoannaView::CreateSizeDependentResources()
{
    if (!m_d3dDevice) return;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : 480);
    swapChainDesc.Height = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : 800);
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
    m_d3dDevice.As(&dxgiDevice);

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

    dxgiFactory->CreateSwapChainForComposition(
        m_d3dDevice.Get(),
        &swapChainDesc,
        nullptr,
        &m_swapChain
    );

    // Gắn SwapChain vào XAML SwapChainPanel thông qua ISwapChainPanelNative
    reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&m_panelNative));
    if (m_panelNative && m_swapChain)
    {
        m_panelNative->SetSwapChain(m_swapChain.Get());
    }
}

void GoannaView::Navigate(Platform::String^ url)
{
    m_currentUrl = url;
    ProgressChanged(0.1);
    // Gửi yêu cầu nạp URI vào Necko / Goanna DocShell
    ProgressChanged(1.0);
    NavigationCompleted(m_currentUrl, true);
}

void GoannaView::GoBack()
{
    if (m_canGoBack)
    {
        // Điều hướng ngược lịch sử DocShell
    }
}

void GoannaView::GoForward()
{
    if (m_canGoForward)
    {
        // Điều hướng tiến lịch sử DocShell
    }
}

void GoannaView::Refresh()
{
    Navigate(m_currentUrl);
}

void GoannaView::Stop()
{
    // Dừng tiến trình tải kênh Necko
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
    WidgetUwp::DispatchTouchEvent(0 /* Pressed */, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerMoved(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    WidgetUwp::DispatchTouchEvent(1 /* Moved */, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    WidgetUwp::DispatchTouchEvent(2 /* Released */, point->Position.X, point->Position.Y);
}

void GoannaView::OnSizeChanged(Platform::Object^ sender, SizeChangedEventArgs^ e)
{
    CreateSizeDependentResources();
}

bool GoannaView::CanGoBack::get() { return m_canGoBack; }
bool GoannaView::CanGoForward::get() { return m_canGoForward; }
Platform::String^ GoannaView::CurrentUrl::get() { return m_currentUrl; }
Platform::String^ GoannaView::DocumentTitle::get() { return m_documentTitle; }
bool GoannaView::IsJitEnabled::get() { return m_isJitEnabled; }
