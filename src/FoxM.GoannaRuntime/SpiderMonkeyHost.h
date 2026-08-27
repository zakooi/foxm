#pragma once

#include <string>

namespace FoxM
{
    namespace GoannaRuntime
    {
        /// <summary>
        /// Bộ điều phối Engine JavaScript SpiderMonkey cho môi trường Windows 10 Mobile.
        /// Tự động phát hiện quyền hạn JIT hoặc fallback về Interpreter an toàn.
        /// </summary>
        class SpiderMonkeyHost
        {
        public:
            static bool InitializeEngine();
            static void ShutdownEngine();
            static void EvaluateScript(Platform::String^ script);
            static void RunGarbageCollector();
            static bool IsJitAvailable();

        private:
            static bool s_isInitialized;
            static bool s_jitEnabled;
        };
    }
}
