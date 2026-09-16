#pragma once

// =====================================================================
// FoxM GoannaRuntime — HttpStack (internal, KHONG phai WinRT public API)
// ---------------------------------------------------------------------
// GET http/https async, dung API UWP hop le:
//   Windows::Web::Http::HttpClient + HttpBaseProtocolFilter
//   (khong dung API Win32 bi cam trong AppContainer:
//    CreateWindow, GetMessage, socket Win32 tran, v.v.)
// Chay tren background qua Concurrency::task, khong block UI thread.
//
// Ghi chu TLS: HttpClient di qua OS stack (Schannel) cua W10M.
// Bundle PEM tu update-root-certs.ps1 hien duoc stage tai
//   src/FoxM.UwpHost/Assets/mozilla-roots.pem
// de dung cho buoc pin/validate thu cong tiep theo (Agent Network),
// chua tu dong nap vao HttpClient (UWP khong cho inject root).
// =====================================================================

#include <chrono>
#include <memory>
#include <string>

#include <ppltasks.h>

namespace FoxM
{
    namespace GoannaRuntime
    {
        // Ket qua fetch: body UTF-16 (wchar_t), contentType, status.
        struct HttpFetchResult
        {
            int StatusCode = 0;
            bool Success = false;   // true khi 2xx va doc body OK
            bool TimedOut = false;  // true khi qua timeout / task_canceled
            std::wstring Body;
            std::wstring ContentType;
            std::wstring FinalUrl;  // URL sau redirect
        };

        // Class noi bo: plain C++ class, khong ref class, khong metadata.
        class HttpStack
        {
        public:
            static const int kDefaultTimeoutSeconds = 20; // trong khoang 15-30s
            static const int kMaxRedirects = 5;
            // Chan tran bo nho cho thiet bi RAM thap (W10M): cat body qua muc nay.
            static const size_t kMaxBodyChars = 2 * 1024 * 1024; // ~2M wchar (~4MB)

            static Concurrency::task<HttpFetchResult> GetAsync(
                const std::wstring& url,
                int timeoutSeconds = kDefaultTimeoutSeconds,
                int maxRedirects = kMaxRedirects);

        private:
            static Concurrency::task<HttpFetchResult> FetchWithRedirectsAsync(
                const std::wstring& url,
                std::shared_ptr<Concurrency::cancellation_token_source> cts,
                Concurrency::cancellation_token token,
                int redirectsLeft);

            static bool IsRedirectStatus(int status)
            {
                return status == 301 || status == 302 || status == 303
                    || status == 307 || status == 308;
            }
        };
    }
}
