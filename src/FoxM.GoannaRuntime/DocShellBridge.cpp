#include "DocShellBridge.h"
#include "HttpStack.h"

#include <cctype>
#include <cwctype>

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Concurrency;

namespace
{
    std::wstring TrimWs(const std::wstring& s)
    {
        size_t b = 0;
        while (b < s.size() && iswspace(s[b])) b++;
        size_t e = s.size();
        while (e > b && iswspace(s[e - 1])) e--;
        return s.substr(b, e - b);
    }

    // Trich <title> don gian (quet 64KB dau, khong phan biet hoa/thuong).
    // Tra ve rong neu khong thay — ben goi fallback ve URL.
    std::wstring ExtractTitle(const std::wstring& html)
    {
        size_t scope = html.size() > 65536 ? 65536 : html.size();
        std::wstring lower;
        lower.resize(scope);
        for (size_t i = 0; i < scope; i++)
        {
            lower[i] = towlower(html[i]);
        }
        size_t open = lower.find(L"<title");
        if (open == std::wstring::npos) return L"";
        size_t gt = lower.find(L'>', open);
        if (gt == std::wstring::npos) return L"";
        size_t close = lower.find(L"</title", gt + 1);
        if (close == std::wstring::npos || close <= gt + 1) return L"";
        std::wstring title = TrimWs(html.substr(gt + 1, close - gt - 1));
        if (title.size() > 256) title.resize(256);
        return title;
    }

    bool IsAboutUri(const std::wstring& uri)
    {
        return uri.compare(0, 6, L"about:") == 0;
    }
}

DocShellBridge::DocShellBridge() :
    m_state(NavigationState::Idle),
    m_progress(0.0),
    m_currentUri(L"about:blank"),
    m_documentTitle(L"FoxM"),
    m_currentHistoryIndex(-1),
    m_lastStatusCode(0),
    m_loadGeneration(0)
{
}

void DocShellBridge::LoadUri(Platform::String^ uri)
{
    if (uri == nullptr || uri->IsEmpty()) return;

    // Giu nguyen logic history: cat forward, push entry moi.
    std::wstring wUri = uri->Data();
    if (m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()) - 1)
    {
        m_historyUrls.erase(m_historyUrls.begin() + m_currentHistoryIndex + 1, m_historyUrls.end());
        m_historyTitles.erase(m_historyTitles.begin() + m_currentHistoryIndex + 1, m_historyTitles.end());
    }

    m_historyUrls.push_back(wUri);
    m_historyTitles.push_back(L"Loading...");
    m_currentHistoryIndex = static_cast<int>(m_historyUrls.size()) - 1;

    // Async: return ngay, fetch chay background qua HttpStack.
    int generation = ++m_loadGeneration;
    m_currentUri = uri;
    OnNavigationStart(wUri);   // -> Loading, progress 0.1 (connecting)
    UpdateProgress(0.1);

    if (IsAboutUri(wUri))
    {
        m_lastBody.clear();
        m_lastContentType = L"text/html";
        m_lastStatusCode = 200;
        UpdateTitle(wUri);
        OnNavigationComplete(true); // -> Completed, progress 1.0
        return;
    }

    StartFetch(wUri, generation);
}

void DocShellBridge::GoBack()
{
    if (CanGoBack)
    {
        m_currentHistoryIndex--;
        // Fetch lai URL lich su, KHONG push entry moi.
        std::wstring wUri = m_historyUrls[m_currentHistoryIndex];
        int generation = ++m_loadGeneration;
        m_currentUri = ref new String(wUri.c_str());
        OnNavigationStart(wUri);
        UpdateProgress(0.1);
        if (IsAboutUri(wUri))
        {
            m_lastBody.clear();
            m_lastContentType = L"text/html";
            m_lastStatusCode = 200;
            m_documentTitle = ref new String(m_historyTitles[m_currentHistoryIndex].c_str());
            OnNavigationComplete(true);
            return;
        }
        StartFetch(wUri, generation);
    }
}

void DocShellBridge::GoForward()
{
    if (CanGoForward)
    {
        m_currentHistoryIndex++;
        // Fetch lai URL lich su, KHONG push entry moi.
        std::wstring wUri = m_historyUrls[m_currentHistoryIndex];
        int generation = ++m_loadGeneration;
        m_currentUri = ref new String(wUri.c_str());
        OnNavigationStart(wUri);
        UpdateProgress(0.1);
        if (IsAboutUri(wUri))
        {
            m_lastBody.clear();
            m_lastContentType = L"text/html";
            m_lastStatusCode = 200;
            m_documentTitle = ref new String(m_historyTitles[m_currentHistoryIndex].c_str());
            OnNavigationComplete(true);
            return;
        }
        StartFetch(wUri, generation);
    }
}

void DocShellBridge::Reload(bool bypassCache)
{
    (void)bypassCache; // HttpClient khong cache disk theo kieu Goanna; giu param theo contract
    if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()))
    {
        // Reload trang hien tai, KHONG push entry trung lap.
        std::wstring wUri = m_historyUrls[m_currentHistoryIndex];
        int generation = ++m_loadGeneration;
        m_currentUri = ref new String(wUri.c_str());
        OnNavigationStart(wUri);
        UpdateProgress(0.1);
        if (IsAboutUri(wUri))
        {
            m_lastBody.clear();
            m_lastContentType = L"text/html";
            m_lastStatusCode = 200;
            OnNavigationComplete(true);
            return;
        }
        StartFetch(wUri, generation);
    }
}

void DocShellBridge::StopLoading()
{
    // Vo hieu hoa cac completion dang bay (stale) cua lan tai cu.
    ++m_loadGeneration;
    m_state = NavigationState::Idle;
    m_progress = 0.0;
}

// Helper noi bo: fetch background; hoan tat ve qua
// UpdateProgress (0.5 loading) + OnNavigationComplete (1.0/that bai).
// Capture `this` (ref class) giu object song toi khi continuation chay xong;
// continuation chay tren thread pool (use_arbitrary), khong block UI thread.
void DocShellBridge::StartFetch(const std::wstring& uri, int generation)
{
    create_task(HttpStack::GetAsync(uri, HttpStack::kDefaultTimeoutSeconds,
        HttpStack::kMaxRedirects)).then(
        [this, uri, generation](HttpFetchResult result)
    {
        if (generation != m_loadGeneration) return; // stale: da Stop/nav moi
        if (result.Success)
        {
            m_lastBody = result.Body;
            m_lastContentType = result.ContentType;
            m_lastStatusCode = result.StatusCode;
            m_currentUri = ref new String(uri.c_str());
            UpdateProgress(0.5); // loading: da co body
            std::wstring title = ExtractTitle(m_lastBody);
            if (title.empty()) title = uri;
            UpdateTitle(title);
            OnNavigationComplete(true); // -> Completed, progress 1.0
        }
        else
        {
            m_lastStatusCode = result.StatusCode;
            OnNavigationComplete(false); // -> Failed
        }
    }, task_continuation_context::use_arbitrary());
}

void DocShellBridge::SetLoadCallback(std::function<void(double progress, bool completed, bool success)> callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_loadCallback = callback;
}

void DocShellBridge::InvokeLoadCallback(double progress, bool completed, bool success)
{
    std::function<void(double, bool, bool)> cb;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        cb = m_loadCallback;
    }
    if (cb)
    {
        try { cb(progress, completed, success); }
        catch (...) { /* callback UI-side, không để exception lan vào engine */ }
    }
}

void DocShellBridge::UpdateProgress(double progress)
{
    m_progress = progress;
    InvokeLoadCallback(progress, false, true);
}

void DocShellBridge::UpdateTitle(const std::wstring& title)
{
    m_documentTitle = ref new String(title.c_str());
    if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < static_cast<int>(m_historyTitles.size()))
    {
        m_historyTitles[m_currentHistoryIndex] = title;
    }
}

void DocShellBridge::OnNavigationStart(const std::wstring& uri)
{
    (void)uri;
    m_state = NavigationState::Loading;
    m_progress = 0.1;
}

void DocShellBridge::OnNavigationComplete(bool success)
{
    m_state = success ? NavigationState::Completed : NavigationState::Failed;
    m_progress = success ? 1.0 : 0.0;
    InvokeLoadCallback(m_progress, true, success);
}

Platform::String^ DocShellBridge::GetLastBody()
{
    if (m_lastBody.empty()) return L"";
    return ref new String(m_lastBody.c_str());
}

Platform::String^ DocShellBridge::GetLastContentType()
{
    if (m_lastContentType.empty()) return L"";
    return ref new String(m_lastContentType.c_str());
}

int DocShellBridge::GetLastStatusCode()
{
    return m_lastStatusCode;
}

NavigationState DocShellBridge::State::get() { return m_state; }
double DocShellBridge::Progress::get() { return m_progress; }
Platform::String^ DocShellBridge::CurrentUri::get() { return m_currentUri; }
Platform::String^ DocShellBridge::DocumentTitle::get() { return m_documentTitle; }
bool DocShellBridge::CanGoBack::get() { return m_currentHistoryIndex > 0; }
bool DocShellBridge::CanGoForward::get() { return m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()) - 1; }
