#include "DocShellBridge.h"

using namespace FoxM::GoannaRuntime;
using namespace Platform;

DocShellBridge::DocShellBridge() :
    m_state(NavigationState::Idle),
    m_progress(0.0),
    m_currentUri(L"about:blank"),
    m_documentTitle(L"FoxM"),
    m_currentHistoryIndex(-1)
{
}

void DocShellBridge::LoadUri(Platform::String^ uri)
{
    if (uri == nullptr || uri->IsEmpty()) return;

    m_currentUri = uri;
    m_state = NavigationState::Connecting;
    m_progress = 0.1;

    std::wstring wUri = uri->Data();
    if (m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()) - 1)
    {
        m_historyUrls.erase(m_historyUrls.begin() + m_currentHistoryIndex + 1, m_historyUrls.end());
        m_historyTitles.erase(m_historyTitles.begin() + m_currentHistoryIndex + 1, m_historyTitles.end());
    }

    m_historyUrls.push_back(wUri);
    m_historyTitles.push_back(L"Loading...");
    m_currentHistoryIndex = static_cast<int>(m_historyUrls.size()) - 1;

    // Mô phỏng tiến trình tải trang Necko
    m_state = NavigationState::Loading;
    m_progress = 0.5;
    m_progress = 1.0;
    m_state = NavigationState::Completed;
}

void DocShellBridge::GoBack()
{
    if (CanGoBack)
    {
        m_currentHistoryIndex--;
        m_currentUri = ref new String(m_historyUrls[m_currentHistoryIndex].c_str());
        m_documentTitle = ref new String(m_historyTitles[m_currentHistoryIndex].c_str());
    }
}

void DocShellBridge::GoForward()
{
    if (CanGoForward)
    {
        m_currentHistoryIndex++;
        m_currentUri = ref new String(m_historyUrls[m_currentHistoryIndex].c_str());
        m_documentTitle = ref new String(m_historyTitles[m_currentHistoryIndex].c_str());
    }
}

void DocShellBridge::Reload(bool bypassCache)
{
    if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()))
    {
        LoadUri(ref new String(m_historyUrls[m_currentHistoryIndex].c_str()));
    }
}

void DocShellBridge::StopLoading()
{
    m_state = NavigationState::Idle;
    m_progress = 0.0;
}

void DocShellBridge::UpdateProgress(double progress)
{
    m_progress = progress;
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
    m_state = NavigationState::Loading;
    m_progress = 0.1;
}

void DocShellBridge::OnNavigationComplete(bool success)
{
    m_state = success ? NavigationState::Completed : NavigationState::Failed;
    m_progress = 1.0;
}

NavigationState DocShellBridge::State::get() { return m_state; }
double DocShellBridge::Progress::get() { return m_progress; }
Platform::String^ DocShellBridge::CurrentUri::get() { return m_currentUri; }
Platform::String^ DocShellBridge::DocumentTitle::get() { return m_documentTitle; }
bool DocShellBridge::CanGoBack::get() { return m_currentHistoryIndex > 0; }
bool DocShellBridge::CanGoForward::get() { return m_currentHistoryIndex < static_cast<int>(m_historyUrls.size()) - 1; }
