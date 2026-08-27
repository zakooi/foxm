#include "NeckoClient.h"
#include <ppltasks.h>

using namespace FoxM::GoannaRuntime;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Security::Cryptography::Certificates;
using namespace concurrency;

NeckoClient::NeckoClient() :
    m_userAgent(L"Mozilla/5.0 (Android; Mobile; rv:109.0) Gecko/115.0 Firefox/115.0")
{
}

IAsyncOperationWithProgress<String^, double>^ NeckoClient::FetchPageAsync(String^ url)
{
    String^ ua = m_userAgent;

    return create_async([url, ua](progress_reporter<double> reporter) -> task<String^>
    {
        reporter.report(0.1);

        auto filter = ref new HttpBaseProtocolFilter();
        filter->AllowAutoRedirect = true;
        filter->AutomaticDecompression = true;

        // Bỏ qua lỗi chứng chỉ SSL lỗi thời trên Windows 10 Mobile để truy cập được mọi trang web hiện đại
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::Untrusted);
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::Expired);
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::WrongUsage);
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::RevocationInformationMissing);
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::RevocationFailure);
        filter->IgnorableServerCertificateErrors->Append(ChainValidationResult::InvalidCertificateAuthorityRoot);

        auto httpClient = ref new HttpClient(filter);
        httpClient->DefaultRequestHeaders->UserAgent->TryParseAdd(ua);
        httpClient->DefaultRequestHeaders->Accept->TryParseAdd(L"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        httpClient->DefaultRequestHeaders->AcceptLanguage->TryParseAdd(L"vi-VN,vi;q=0.9,en-US;q=0.8,en;q=0.7");

        auto uri = ref new Uri(url);
        reporter.report(0.3);

        return create_task(httpClient->GetStringAsync(uri)).then([reporter](String^ responseBody)
        {
            reporter.report(1.0);
            return responseBody;
        });
    });
}
