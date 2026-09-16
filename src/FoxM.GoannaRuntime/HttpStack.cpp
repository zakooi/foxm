// HttpStack.cpp — xem ghi chu contract trong HttpStack.h
#include "HttpStack.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Concurrency;
using namespace FoxM::GoannaRuntime;

namespace
{
    HttpFetchResult MakeFailure(int statusCode)
    {
        HttpFetchResult r;
        r.StatusCode = statusCode;
        r.Success = false;
        r.TimedOut = false;
        return r;
    }
}

task<HttpFetchResult> HttpStack::GetAsync(
    const std::wstring& url,
    int timeoutSeconds,
    int maxRedirects)
{
    if (timeoutSeconds < 5) timeoutSeconds = 5;
    if (timeoutSeconds > 30) timeoutSeconds = 30;
    if (maxRedirects < 0) maxRedirects = 0;
    if (maxRedirects > 10) maxRedirects = 10;
    if (url.empty())
    {
        return task_from_result(MakeFailure(0));
    }

    auto cts = std::make_shared<cancellation_token_source>();
    cts->cancel_after(std::chrono::seconds(timeoutSeconds));
    cancellation_token token = cts->get_token();

    // Map moi ngoai le (mang, huy, parse) ve HttpFetchResult de
    // DocShellBridge khong bao gio nhan exception tu task.
    return FetchWithRedirectsAsync(url, cts, token, maxRedirects)
        .then([](task<HttpFetchResult> t) -> HttpFetchResult
    {
        try
        {
            return t.get();
        }
        catch (const task_canceled&)
        {
            HttpFetchResult r;
            r.TimedOut = true;
            return r;
        }
        catch (Exception^)
        {
            return MakeFailure(0);
        }
        catch (...)
        {
            return MakeFailure(0);
        }
    });
}

task<HttpFetchResult> HttpStack::FetchWithRedirectsAsync(
    const std::wstring& url,
    std::shared_ptr<cancellation_token_source> cts,
    cancellation_token token,
    int redirectsLeft)
{
    Uri^ uri = nullptr;
    try
    {
        uri = ref new Uri(ref new String(url.c_str()));
    }
    catch (Exception^)
    {
        return task_from_result(MakeFailure(0));
    }
    catch (...)
    {
        return task_from_result(MakeFailure(0));
    }

    String^ scheme = uri->SchemeName;
    if (scheme != L"http" && scheme != L"https")
    {
        return task_from_result(MakeFailure(0));
    }

    // Tu redirect thu cong (AllowAutoRedirect=false) de gioi han dung 5.
    HttpBaseProtocolFilter^ filter = ref new HttpBaseProtocolFilter();
    filter->AllowAutoRedirect = false;
    HttpClient^ client = ref new HttpClient(filter);

    // UA theo config/all-w10m.js (general.useragent.override).
    try
    {
        client->DefaultRequestHeaders->UserAgent->ParseAdd(
            L"Mozilla/5.0 (Android; Mobile; rv:109.0) Gecko/115.0 Firefox/115.0");
    }
    catch (...)
    {
        // Bo qua: UA mac dinh cua OS van fetch duoc.
    }

    task<HttpResponseMessage^> requestTask;
    try
    {
        requestTask = create_task(client->GetAsync(uri), token);
    }
    catch (...)
    {
        delete client; // giai phong socket/filter xac dinh (IDisposable)
        return task_from_result(MakeFailure(0));
    }

    return requestTask.then(
        [client, uri, url, cts, token, redirectsLeft](
            task<HttpResponseMessage^> t) -> task<HttpFetchResult>
    {
        HttpResponseMessage^ response = nullptr;
        try
        {
            response = t.get();
        }
        catch (const task_canceled&)
        {
            delete client;
            throw; // de lop ngoai (GetAsync) map thanh TimedOut
        }
        catch (...)
        {
            delete client;
            return task_from_result(MakeFailure(0));
        }

        int status = static_cast<int>(response->StatusCode);

        // --- Redirect thu cong (toi da kMaxRedirects) ---
        if (IsRedirectStatus(status) && redirectsLeft > 0)
        {
            // HttpResponseHeaderCollection::Location (Uri^, nullptr neu thieu);
            // dung property thay vi Lookup de tranh phu thuoc IMap.
            String^ location = nullptr;
            try
            {
                Uri^ locUri = response->Headers->Location;
                if (locUri != nullptr)
                {
                    location = locUri->ToString();
                }
            }
            catch (...)
            {
                location = nullptr;
            }
            delete client;
            if (location == nullptr || location->IsEmpty())
            {
                return task_from_result(MakeFailure(status));
            }
            std::wstring next;
            try
            {
                // Ho tro Location tuong doi (Uri co ctor base+relative).
                Uri^ nextUri = ref new Uri(uri, location);
                String^ nextScheme = nextUri->SchemeName;
                if (nextScheme != L"http" && nextScheme != L"https")
                {
                    return task_from_result(MakeFailure(status));
                }
                next = nextUri->ToString()->Data();
            }
            catch (...)
            {
                return task_from_result(MakeFailure(status));
            }
            return FetchWithRedirectsAsync(next, cts, token, redirectsLeft - 1);
        }

        // --- Doc body (khong block UI: continuation chay background) ---
        IHttpContent^ content = response->Content;
        if (content == nullptr)
        {
            delete client;
            HttpFetchResult r;
            r.StatusCode = status;
            r.Success = (status >= 200 && status < 300);
            r.FinalUrl = url;
            return task_from_result(r);
        }

        task<String^> bodyTask;
        try
        {
            bodyTask = create_task(content->ReadAsStringAsync(), token);
        }
        catch (...)
        {
            delete client;
            return task_from_result(MakeFailure(status));
        }

        return bodyTask.then(
            [client, response, status, url](task<String^> bt) -> HttpFetchResult
        {
            delete client;
            HttpFetchResult r;
            r.StatusCode = status;
            r.FinalUrl = url;
            r.Success = (status >= 200 && status < 300);
            try
            {
                String^ bodyStr = bt.get();
                if (bodyStr != nullptr && bodyStr->Length() > 0)
                {
                    size_t n = static_cast<size_t>(bodyStr->Length());
                    if (n > HttpStack::kMaxBodyChars)
                    {
                        n = HttpStack::kMaxBodyChars; // cat chong tran RAM
                    }
                    r.Body.assign(bodyStr->Data(), bodyStr->Data() + n);
                }
            }
            catch (const task_canceled&)
            {
                r.Success = false;
                r.TimedOut = true;
                return r;
            }
            catch (...)
            {
                r.Success = false;
                return r;
            }
            try
            {
                if (response->Content != nullptr &&
                    response->Content->Headers != nullptr &&
                    response->Content->Headers->ContentType != nullptr)
                {
                    r.ContentType =
                        response->Content->Headers->ContentType->ToString()->Data();
                }
            }
            catch (...)
            {
                // Giu ContentType rong: ben goi tu suy luan.
            }
            return r;
        });
    });
}
