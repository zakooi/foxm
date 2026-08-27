#pragma once

#include <string>
#include <atomic>

namespace FoxM
{
    namespace GoannaRuntime
    {
        /// <summary>
        /// Bộ điều phối Engine JavaScript SpiderMonkey cho môi trường Windows 10 Mobile.
        /// Quản lý vòng đời theo ref-count đa luồng, tự động phát hiện quyền hạn JIT hoặc fallback về Interpreter an toàn.
        /// </summary>
        public ref class SpiderMonkeyHost sealed
        {
        public:
            static bool AcquireEngine();
            static void ReleaseEngine();
            static void EvaluateScript(Platform::String^ script);
            static void RunGarbageCollector();
            static bool IsJitAvailable();

        internal:
            static std::atomic<int> s_refCount;
            static std::atomic<bool> s_isInitialized;
            static std::atomic<bool> s_jitEnabled;
        };
    }
}
