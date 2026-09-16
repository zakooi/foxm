#include "GoannaView.h"
#include "LiteRenderer.h"
#include <windows.h>
#include <string>

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;

// =====================================================================
// Agent C — phần render (CHỈ thêm, không sửa Navigate/GoBack/GoForward/
// Reload/Stop/ExecuteScriptAsync).
// Contract Agent A: RenderFrame(IDXGISwapChain1*, UINT, UINT) dùng lại
// swapchain hiện có, KHÔNG tạo swapchain mới mỗi frame; resize + SetSwapChain
// do CreateSizeDependentResources đảm nhiệm. Không tạo thread mới: GoannaView
// poll WidgetUwp::NeedsRender trong handler pointer/size rồi vẽ đồng bộ.
// =====================================================================
namespace
{
    // Thu thập context (scroll/zoom/body/progress) rồi vẽ qua LiteRenderer.
    // Nhận mọi con trỏ/member cần thiết qua tham số để không phá private của GoannaView.h.
    static void RequestGoannaRender(
        ID3D11Device2* device,
        ID3D11DeviceContext2* context,
        IDXGISwapChain1* swapChain,
        FoxM::GoannaRuntime::WidgetUwp^ widget,
        FoxM::GoannaRuntime::DocShellBridge^ docShell,
        UINT viewportWidth,
        UINT viewportHeight)
    {
        if (!swapChain || !widget) return;
        if (viewportWidth == 0 || viewportHeight == 0) return;

        float scrollX = 0.0f;
        float scrollY = 0.0f;
        float zoom = 1.0f;
        try
        {
            scrollX = widget->ScrollX;
            scrollY = widget->ScrollY;
            zoom = widget->ZoomLevel;
        }
        catch (...) { return; }

        double progress = 0.0;
        std::wstring header = L"FoxM — about:blank";
        try
        {
            if (docShell)
            {
                progress = docShell->Progress;
                Platform::String^ title = docShell->DocumentTitle;
                Platform::String^ uri = docShell->CurrentUri;
                std::wstring t = (title && title->Length() > 0) ? std::wstring(title->Data()) : L"";
                std::wstring u = (uri && uri->Length() > 0) ? std::wstring(uri->Data()) : L"";
                if (!t.empty() || !u.empty())
                {
                    header = t;
                    if (!u.empty())
                    {
                        if (!header.empty()) header += L" — ";
                        header += u;
                    }
                }
            }
        }
        catch (...) { /* giữ giá trị mặc định, vẫn vẽ placeholder */ }

        // Body cho render: DocShellBridge::GetLastBody (internal, cùng component nên
        // GoannaView gọi được). Phòng thủ: null/rỗng/exception -> body rỗng để
        // LiteRenderer vẽ placeholder, không crash, không vỡ build khi Network đổi.
        std::wstring body;
        try
        {
            if (docShell)
            {
                Platform::String^ b = docShell->GetLastBody();
                if (b && b->Length() > 0)
                {
                    body.assign(b->Data(), b->Length());
                }
            }
        }
        catch (...) { body.clear(); }

        FoxM::GoannaRuntime::LiteRenderer::Render(
            device, context, swapChain,
            viewportWidth, viewportHeight,
            scrollX, scrollY, zoom,
            body, progress, header);

        try { widget->ClearNeedsRender(); }
        catch (...) {}
    }

    // Hàm vẽ tương lai theo contract Agent A (giữ đúng signature).
    // Dùng lại swapchain hiện có, vẽ placeholder + Present; KHÔNG tạo swapchain mới.
    static void RenderFrame(IDXGISwapChain1* swapChain, UINT viewportWidth, UINT viewportHeight)
    {
        if (!swapChain) return;
        if (viewportWidth == 0 || viewportHeight == 0) return;
        FoxM::GoannaRuntime::LiteRenderer::Render(
            nullptr, nullptr, swapChain,
            viewportWidth, viewportHeight,
            0.0f, 0.0f, 1.0f,
            std::wstring(), 0.0, L"FoxM — about:blank");
    }
}

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

    // Merge: DocShellBridge fetch async trên thread pool — forward completion về
    // UI thread qua Dispatcher (WeakReference tránh dangling khi view bị hủy).
    Platform::WeakReference weakThis(this);
    m_docShell->SetLoadCallback(
        [weakThis](double progress, bool completed, bool success)
    {
        GoannaView^ self = weakThis.Resolve<GoannaView>();
        if (self == nullptr) return;
        try
        {
            auto dispatcher = self->Dispatcher;
            if (dispatcher == nullptr) return;
            dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                ref new DispatchedHandler([weakThis, progress, completed, success]()
            {
                GoannaView^ view = weakThis.Resolve<GoannaView>();
                if (view != nullptr)
                {
                    view->OnLoadEvent(progress, completed, success);
                }
            }));
        }
        catch (...) { /* view đang teardown — bỏ qua completion trễ */ }
    });

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
            // Agent C: vẽ lại ngay sau ResizeBuffers (dùng lại swapchain, không tạo mới).
            RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, width, height);
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
        // Agent C: vẽ khung đầu tiên sau SetSwapChain (SetSwapChain đúng 1 lần/swapchain).
        RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, width, height);
    }
}

// Merge: LoadUri/GoBack/GoForward/Reload là async — chỉ fire NavigationStarting
// + progress ban đầu ở đây; completion (TitleChanged/NavigationCompleted/vẽ lại)
// đến qua OnLoadEvent từ callback của DocShellBridge.
void GoannaView::Navigate(Platform::String^ url)
{
    if (url == nullptr || url->IsEmpty()) return;

    NavigationStarting(url);
    ProgressChanged(0.1);

    m_docShell->LoadUri(url);
}

void GoannaView::OnLoadEvent(double progress, bool completed, bool success)
{
    ProgressChanged(progress);
    if (completed)
    {
        TitleChanged(m_docShell->DocumentTitle);
        NavigationCompleted(m_docShell->CurrentUri, success);
        if (m_swapChain && m_widget)
        {
            UINT w = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : m_widget->ViewportWidth);
            UINT h = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : m_widget->ViewportHeight);
            RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, w, h);
        }
    }
}

void GoannaView::GoBack()
{
    if (m_docShell->CanGoBack)
    {
        m_docShell->GoBack();
        NavigationStarting(m_docShell->CurrentUri);
        ProgressChanged(0.1);
    }
}

void GoannaView::GoForward()
{
    if (m_docShell->CanGoForward)
    {
        m_docShell->GoForward();
        NavigationStarting(m_docShell->CurrentUri);
        ProgressChanged(0.1);
    }
}

void GoannaView::Reload()
{
    if (m_docShell->CurrentUri == nullptr || m_docShell->CurrentUri->IsEmpty())
    {
        NavigationCompleted(L"about:blank", false);
        return;
    }
    m_docShell->Reload(false);
    NavigationStarting(m_docShell->CurrentUri);
    ProgressChanged(0.1);
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
    // Agent C: poll nhẹ NeedsRender (không tạo thread); Pressed thường chưa đổi scroll
    // nhưng vẫn vẽ lại để phản hồi chạm kịp thời khi widget đánh dấu dirty.
    if (m_widget && m_widget->NeedsRender && m_swapChain)
    {
        UINT w = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : m_widget->ViewportWidth);
        UINT h = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : m_widget->ViewportHeight);
        RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, w, h);
    }
}

void GoannaView::OnPointerMoved(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_widget->DispatchTouchEvent(TouchEventType::Moved, point->Position.X, point->Position.Y);
    // Agent C: kéo-scroll (pointer down+move) đổi scroll trong WidgetUwp -> vẽ lại đồng bộ.
    if (m_widget && m_widget->NeedsRender && m_swapChain)
    {
        UINT w = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : m_widget->ViewportWidth);
        UINT h = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : m_widget->ViewportHeight);
        RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, w, h);
    }
}

void GoannaView::OnPointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_widget->DispatchTouchEvent(TouchEventType::Released, point->Position.X, point->Position.Y);
    if (m_widget && m_widget->NeedsRender && m_swapChain)
    {
        UINT w = static_cast<UINT>(this->ActualWidth > 0 ? this->ActualWidth : m_widget->ViewportWidth);
        UINT h = static_cast<UINT>(this->ActualHeight > 0 ? this->ActualHeight : m_widget->ViewportHeight);
        RequestGoannaRender(m_d3dDevice.Get(), m_d3dContext.Get(), m_swapChain.Get(), m_widget, m_docShell, w, h);
    }
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
