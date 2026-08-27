#include "GoannaView.h"
#include <windows.h>
#include <ppltasks.h>

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace concurrency;

GoannaView::GoannaView() :
    m_isJitEnabled(false),
    m_isPointerDown(false),
    m_lastPointerY(0.0f),
    m_scrollOffset(0.0f)
{
    m_docShell = ref new DocShellBridge();
    m_widget = ref new WidgetUwp();
    m_neckoClient = ref new NeckoClient();

    m_renderContext = std::make_unique<D2DRenderContext>();
    m_domParser = std::make_unique<GoannaDOMParser>();

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

    // Nếu SwapChain đã tồn tại, dùng ResizeBuffers
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
            if (m_renderContext)
            {
                m_renderContext->Resize(m_swapChain.Get(), width, height);
            }
            m_widget->OnViewportResized(width, height);
            Render();
            return;
        }

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
        reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&m_panelNative));
        if (m_panelNative)
        {
            m_panelNative->SetSwapChain(m_swapChain.Get());
        }

        if (m_renderContext)
        {
            m_renderContext->Initialize(m_d3dDevice.Get(), m_swapChain.Get(), 1.0f);
        }
        m_widget->OnViewportResized(width, height);
        Render();
    }
}

void GoannaView::Render()
{
    if (!m_renderContext || !m_swapChain) return;

    m_renderContext->BeginDraw();
    m_renderContext->Clear(D2D1::ColorF(0x121214));

    if (m_domParser)
    {
        const auto& boxes = m_domParser->GetRenderBoxes();
        for (const auto& box : boxes)
        {
            if (box.type == NodeType::Divider)
            {
                m_renderContext->DrawLine(box.x, box.y, box.x + box.width, box.y, box.textColor, 1.0f);
            }
            else if (!box.text.empty())
            {
                m_renderContext->DrawText(box.text, box.x, box.y, box.width, box.height, box.fontSize, box.isBold, box.textColor);
            }
        }
    }

    m_renderContext->EndDraw();
    m_swapChain->Present(1, 0);
}

void GoannaView::Navigate(Platform::String^ url)
{
    if (url == nullptr || url->IsEmpty()) return;

    NavigationStarting(url);
    ProgressChanged(0.2);

    m_docShell->LoadUri(url);
    m_scrollOffset = 0.0f;
    if (m_renderContext) m_renderContext->SetScrollOffset(0.0f);

    float currentWidth = static_cast<float>(this->ActualWidth > 0 ? this->ActualWidth : 480.0f);

    // Kích hoạt NeckoClient tải trang web thật
    create_task(m_neckoClient->FetchPageAsync(url)).then([this, url, currentWidth](task<String^> previousTask)
    {
        try
        {
            String^ htmlResponse = previousTask.get();
            ProgressChanged(0.7);

            if (m_domParser && htmlResponse)
            {
                m_domParser->ParseAndLayout(htmlResponse->Data(), currentWidth);
                std::wstring title = m_domParser->GetPageTitle();
                m_docShell->UpdateTitle(title);
                TitleChanged(ref new String(title.c_str()));
            }

            ProgressChanged(1.0);
            Render();
            NavigationCompleted(url, true);
        }
        catch (...)
        {
            ProgressChanged(0.0);
            NavigationCompleted(url, false);
        }
    });
}

void GoannaView::GoBack()
{
    if (m_docShell->CanGoBack)
    {
        m_docShell->GoBack();
        Navigate(m_docShell->CurrentUri);
    }
}

void GoannaView::GoForward()
{
    if (m_docShell->CanGoForward)
    {
        m_docShell->GoForward();
        Navigate(m_docShell->CurrentUri);
    }
}

void GoannaView::Reload()
{
    Navigate(m_docShell->CurrentUri);
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
    m_isPointerDown = true;
    m_lastPointerY = point->Position.Y;
    m_widget->DispatchTouchEvent(TouchEventType::Pressed, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerMoved(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    if (m_isPointerDown && m_renderContext && m_domParser)
    {
        float deltaY = point->Position.Y - m_lastPointerY;
        m_lastPointerY = point->Position.Y;

        m_scrollOffset -= deltaY;
        if (m_scrollOffset < 0.0f) m_scrollOffset = 0.0f;
        float maxScroll = m_domParser->GetTotalContentHeight() - static_cast<float>(this->ActualHeight);
        if (maxScroll > 0.0f && m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;

        m_renderContext->SetScrollOffset(m_scrollOffset);
        Render();
    }
    m_widget->DispatchTouchEvent(TouchEventType::Moved, point->Position.X, point->Position.Y);
}

void GoannaView::OnPointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
    auto point = e->GetCurrentPoint(this);
    m_isPointerDown = false;
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
