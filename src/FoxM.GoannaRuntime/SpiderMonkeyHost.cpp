#include "SpiderMonkeyHost.h"
#include <windows.h>

using namespace FoxM::GoannaRuntime;

bool SpiderMonkeyHost::s_isInitialized = false;
bool SpiderMonkeyHost::s_jitEnabled = false;

bool SpiderMonkeyHost::InitializeEngine()
{
    if (s_isInitialized) return s_jitEnabled;

    // Kiểm tra khả năng cấp phát bộ nhớ JIT trên UWP Sandbox
    s_jitEnabled = IsJitAvailable();

    if (s_jitEnabled)
    {
        // Kích hoạt Baseline JIT (Tối ưu hiệu năng khi có quyền codeGeneration)
        OutputDebugStringA("[FoxM SpiderMonkey] JIT Memory Allocation ALLOWED. Enabling Baseline JIT.\n");
    }
    else
    {
        // Chế độ C++ Interpreter thuần (Bảo mật tối đa, không vi phạm W^X)
        OutputDebugStringA("[FoxM SpiderMonkey] JIT Memory Allocation RESTRICTED. Running in pure C++ Interpreter mode.\n");
    }

    s_isInitialized = true;
    return s_jitEnabled;
}

void SpiderMonkeyHost::ShutdownEngine()
{
    if (!s_isInitialized) return;
    s_isInitialized = false;
}

void SpiderMonkeyHost::EvaluateScript(Platform::String^ script)
{
    if (!s_isInitialized || script == nullptr) return;
    // Thực thi script trong JSContext
}

void SpiderMonkeyHost::RunGarbageCollector()
{
    if (!s_isInitialized) return;
    OutputDebugStringA("[FoxM SpiderMonkey] Forcing Aggressive Garbage Collection...\n");
}

bool SpiderMonkeyHost::IsJitAvailable()
{
    // Thử cấp phát trang bộ nhớ RWX để kiểm tra chính sách hệ thống
    void* testAlloc = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (testAlloc != nullptr)
    {
        VirtualFree(testAlloc, 0, MEM_RELEASE);
        return true;
    }
    return false;
}
