#pragma once

#include <string>

namespace FoxM
{
    namespace GoannaRuntime
    {
        public delegate void PageLoadProgressHandler(double percent);

        /// <summary>
        /// Kênh mạng Necko C++/CX hỗ trợ tải nội dung trang web qua HTTPS / TLS 1.3.
        /// </summary>
        public ref class NeckoClient sealed
        {
        public:
            NeckoClient();

            Windows::Foundation::IAsyncOperationWithProgress<Platform::String^, double>^ FetchPageAsync(Platform::String^ url);

        private:
            Platform::String^ m_userAgent;
        };
    }
}
