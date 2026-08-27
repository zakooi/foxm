#pragma once

#include <string>
#include <vector>

namespace FoxM
{
    namespace GoannaRuntime
    {
        public enum class NavigationState
        {
            Idle,
            Connecting,
            Loading,
            Completed,
            Failed
        };

        public ref class HistoryEntry sealed
        {
        public:
            property Platform::String^ Url;
            property Platform::String^ Title;
            property int64 Timestamp;
        };

        /// <summary>
        /// Cầu nối điều phối nạp trang, lịch sử duyệt (Session History) và trạng thái tải của Goanna DocShell.
        /// </summary>
        public ref class DocShellBridge sealed
        {
        public:
            DocShellBridge();

            void LoadUri(Platform::String^ uri);
            void GoBack();
            void GoForward();
            void Reload(bool bypassCache);
            void StopLoading();

            property NavigationState State { NavigationState get(); }
            property double Progress { double get(); }
            property Platform::String^ CurrentUri { Platform::String^ get(); }
            property Platform::String^ DocumentTitle { Platform::String^ get(); }
            property bool CanGoBack { bool get(); }
            property bool CanGoForward { bool get(); }

        internal:
            void UpdateProgress(double progress);
            void UpdateTitle(const std::wstring& title);
            void OnNavigationStart(const std::wstring& uri);
            void OnNavigationComplete(bool success);

        private:
            NavigationState m_state;
            double m_progress;
            Platform::String^ m_currentUri;
            Platform::String^ m_documentTitle;
            int m_currentHistoryIndex;
            std::vector<std::wstring> m_historyUrls;
            std::vector<std::wstring> m_historyTitles;
        };
    }
}
