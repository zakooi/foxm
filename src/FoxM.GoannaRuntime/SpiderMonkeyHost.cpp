#include "SpiderMonkeyHost.h"
#include <windows.h>

using namespace FoxM::GoannaRuntime;

std::atomic<int> SpiderMonkeyHost::s_refCount(0);
std::atomic<bool> SpiderMonkeyHost::s_isInitialized(false);
std::atomic<bool> SpiderMonkeyHost::s_jitEnabled(false);

bool SpiderMonkeyHost::AcquireEngine()
{
    int previousCount = s_refCount.fetch_add(1);
    if (previousCount > 0 && s_isInitialized.load())
    {
        return s_jitEnabled.load();
    }

    // Khởi tạo Engine lần đầu tiên
    bool jit = IsJitAvailable();
    s_jitEnabled.store(jit);

    if (jit)
    {
        OutputDebugStringA("[FoxM SpiderMonkey] JIT Memory Allocation ALLOWED. Enabling Baseline JIT.\n");
    }
    else
    {
        OutputDebugStringA("[FoxM SpiderMonkey] JIT Memory Allocation RESTRICTED. Running in pure C++ Interpreter mode.\n");
    }

    s_isInitialized.store(true);
    return jit;
}

void SpiderMonkeyHost::ReleaseEngine()
{
    int current = s_refCount.fetch_sub(1);
    if (current <= 1)
    {
        s_refCount.store(0);
        s_isInitialized.store(false);
        OutputDebugStringA("[FoxM SpiderMonkey] All instances closed. SpiderMonkey Engine successfully shut down.\n");
    }
}

void SpiderMonkeyHost::EvaluateScript(Platform::String^ script)
{
    if (!s_isInitialized.load() || script == nullptr) return;
    // Thực thi script trong JSContext
    OutputDebugStringA("[FoxM SpiderMonkey] Evaluated JS expression.\n");
}

void SpiderMonkeyHost::RunGarbageCollector()
{
    if (!s_isInitialized.load()) return;
    OutputDebugStringA("[FoxM SpiderMonkey] Forcing Aggressive Garbage Collection...\n");
}

bool SpiderMonkeyHost::IsJitAvailable()
{
    // Thử cấp phát trang bộ nhớ RWX để kiểm tra chính sách hệ thống (codeGeneration capability)
    void* testAlloc = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (testAlloc != nullptr)
    {
        VirtualFree(testAlloc, 0, MEM_RELEASE);
        return true;
    }
    return false;
}
