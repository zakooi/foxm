#pragma once

#include <functional>
#include <mutex>
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

            // Body/content cua lan tai gan nhat cho agent Render doc.
            // (internal them moi — khong doi public API da chot.)
            Platform::String^ GetLastBody();
            Platform::String^ GetLastContentType();
            int GetLastStatusCode();

            // Callback async cho GoannaView (merge): (progress 0..1, completed, success).
            // Được gọi từ UpdateProgress (completed=false) và OnNavigationComplete
            // (completed=true), có thể trên thread pool — bên nhận phải marshal về UI.
            void SetLoadCallback(std::function<void(double progress, bool completed, bool success)> callback);

        private:
            void StartFetch(const std::wstring& uri, int generation);
            void InvokeLoadCallback(double progress, bool completed, bool success);
            NavigationState m_state;
            double m_progress;
            Platform::String^ m_currentUri;
            Platform::String^ m_documentTitle;
            int m_currentHistoryIndex;
            std::vector<std::wstring> m_historyUrls;
            std::vector<std::wstring> m_historyTitles;
            // Tai gan nhat (cho Render): body UTF-16 + content type + status.
            std::wstring m_lastBody;
            std::wstring m_lastContentType;
            int m_lastStatusCode;
            // Tang moi lan LoadUri/GoBack/GoForward/Reload/StopLoading de
            // bo qua completion tre (stale) cua cac lan tai cu.
            int m_loadGeneration;
            // Callback async (merge) + lock vì completion chạy trên thread pool.
            std::function<void(double progress, bool completed, bool success)> m_loadCallback;
            std::mutex m_callbackMutex;
        };
    }
}
