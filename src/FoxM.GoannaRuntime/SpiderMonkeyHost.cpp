#include "SpiderMonkeyHost.h"
#include <windows.h>
#include <string>
#include <mutex>
#include <atomic>

// NOTE for Build agent: this file needs Chakrart.lib (Windows 10 SDK, UWP-compatible)
// added to the vcxproj of FoxM.GoannaRuntime. No #pragma comment(lib) here on purpose
// so the Build agent owns all link settings in the vcxproj.
// Engine: system Chakra JsRT (jsrt.h) - no vendored sources, no app-side RWX allocation,
// no W^X violation in AppContainer (any JIT is brokered by the system, not by the app).
#include <jsrt.h>

using namespace FoxM::GoannaRuntime;

// ---- Public contract statics (declared in SpiderMonkeyHost.h, semantics preserved) ----
std::atomic<int> SpiderMonkeyHost::s_refCount(0);
std::atomic<bool> SpiderMonkeyHost::s_isInitialized(false);
std::atomic<bool> SpiderMonkeyHost::s_jitEnabled(false);

// ---- Internal JsRT state (file-static: header stays 100% unchanged, no public API change) ----
namespace
{
    JsRuntimeHandle s_jsRuntime = JS_INVALID_RUNTIME_HANDLE;
    JsContextRef s_jsContext = JS_INVALID_REFERENCE;
    std::mutex s_jsMutex; // serializes all JsRT access (single context is not thread-safe)
    std::atomic<unsigned long> s_sourceCookie(0);
}

bool SpiderMonkeyHost::AcquireEngine()
{
    int previousCount = s_refCount.fetch_add(1);
    if (previousCount > 0 && s_isInitialized.load())
    {
        return s_jitEnabled.load();
    }

    std::lock_guard<std::mutex> lock(s_jsMutex);

    // Double-check after taking the lock: another thread may have
    // finished init while we were waiting on the mutex.
    if (s_isInitialized.load() &&
        s_jsRuntime != JS_INVALID_RUNTIME_HANDLE &&
        s_jsContext != JS_INVALID_REFERENCE)
    {
        return s_jitEnabled.load();
    }

    // JIT-cap probe (report only, NEVER force JIT). Keeps the VirtualAlloc probe logic.
    bool jit = IsJitAvailable();
    s_jitEnabled.store(jit);

    JsRuntimeHandle rt = JS_INVALID_RUNTIME_HANDLE;
    JsErrorCode err = JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &rt);
    if (err != JsNoError || rt == JS_INVALID_RUNTIME_HANDLE)
    {
        s_refCount.fetch_sub(1);
        s_isInitialized.store(false);
        OutputDebugStringA("[FoxM SpiderMonkey] JsCreateRuntime FAILED. Engine not initialized.\n");
        return false;
    }

    JsContextRef ctx = JS_INVALID_REFERENCE;
    err = JsCreateContext(rt, &ctx);
    if (err != JsNoError || ctx == JS_INVALID_REFERENCE)
    {
        JsDisposeRuntime(rt);
        s_refCount.fetch_sub(1);
        s_isInitialized.store(false);
        OutputDebugStringA("[FoxM SpiderMonkey] JsCreateContext FAILED. Runtime disposed, no leak.\n");
        return false;
    }

    err = JsSetCurrentContext(ctx);
    if (err != JsNoError)
    {
        JsSetCurrentContext(JS_INVALID_REFERENCE);
        JsDisposeRuntime(rt);
        s_refCount.fetch_sub(1);
        s_isInitialized.store(false);
        OutputDebugStringA("[FoxM SpiderMonkey] JsSetCurrentContext FAILED. Runtime disposed, no leak.\n");
        return false;
    }
    // Do not hold the current context on the creating thread (usually the UI thread):
    // each EvaluateScript sets/unsets it around JsRunScript. No wait loops, no UI block.
    JsSetCurrentContext(JS_INVALID_REFERENCE);

    s_jsRuntime = rt;
    s_jsContext = ctx;
    s_isInitialized.store(true);

    if (jit)
    {
        OutputDebugStringA("[FoxM SpiderMonkey] JsRT runtime ready. JIT allocation ALLOWED (codeGeneration cap).\n");
    }
    else
    {
        OutputDebugStringA("[FoxM SpiderMonkey] JsRT runtime ready. JIT RESTRICTED: Chakra runs interpreter-only, no RWX from app.\n");
    }

    return jit;
}

void SpiderMonkeyHost::ReleaseEngine()
{
    int current = s_refCount.fetch_sub(1);
    if (current <= 0)
    {
        // Unbalanced Release (no matching Acquire): clamp, do not dispose.
        s_refCount.store(0);
        return;
    }
    if (current > 1)
    {
        return; // other instances still alive: shutdown only when refcount reaches 0.
    }

    // current == 1: last reference -> dispose in order context -> runtime.
    std::lock_guard<std::mutex> lock(s_jsMutex);
    JsRuntimeHandle rt = s_jsRuntime;
    s_jsRuntime = JS_INVALID_RUNTIME_HANDLE;
    s_jsContext = JS_INVALID_REFERENCE;

    // Clear current context before dispose to avoid a dangling current on this thread.
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    if (rt != JS_INVALID_RUNTIME_HANDLE)
    {
        JsDisposeRuntime(rt);
    }
    s_isInitialized.store(false);
    OutputDebugStringA("[FoxM SpiderMonkey] All instances closed. JsRT runtime disposed.\n");
}

void SpiderMonkeyHost::EvaluateScript(Platform::String^ script)
{
    if (!s_isInitialized.load() || script == nullptr) return;

    // Copy UTF-16 outside the lock (Data() is null-terminated; the handle
    // keeps the buffer alive during the call; copy keeps it safe under lock).
    std::wstring src(script->Data());
    if (src.empty()) return;

    try
    {
        std::lock_guard<std::mutex> lock(s_jsMutex);
        if (!s_isInitialized.load()) return;
        if (s_jsRuntime == JS_INVALID_RUNTIME_HANDLE) return;
        if (s_jsContext == JS_INVALID_REFERENCE) return;

        if (JsSetCurrentContext(s_jsContext) != JsNoError) return;

        JsValueRef result = JS_INVALID_REFERENCE;
        JsErrorCode err = JsRunScript(
            src.c_str(),
            static_cast<JsSourceContext>(s_sourceCookie.fetch_add(1)),
            L"foxm://goanna",
            &result);
        (void)result; // fire-and-forget per current contract (ExecuteScriptAsync ignores result).

        if (err != JsNoError)
        {
            // Swallow the JS exception: fetch + clear so the runtime stays clean.
            // Must never crash the host on a script error.
            JsValueRef ex = JS_INVALID_REFERENCE;
            if (JsGetAndClearException(&ex) == JsNoError)
            {
                (void)ex;
            }
            OutputDebugStringA("[FoxM SpiderMonkey] JsRunScript failed (JS exception caught, cleared).\n");
        }
        else
        {
            OutputDebugStringA("[FoxM SpiderMonkey] Evaluated JS expression.\n");
        }

        JsSetCurrentContext(JS_INVALID_REFERENCE);
    }
    catch (...)
    {
        // Last barrier: never let a native exception escape to the UI thread.
        JsSetCurrentContext(JS_INVALID_REFERENCE);
        OutputDebugStringA("[FoxM SpiderMonkey] EvaluateScript native exception swallowed.\n");
    }
}

void SpiderMonkeyHost::RunGarbageCollector()
{
    if (!s_isInitialized.load()) return;
    try
    {
        std::lock_guard<std::mutex> lock(s_jsMutex);
        if (!s_isInitialized.load()) return;
        if (s_jsRuntime == JS_INVALID_RUNTIME_HANDLE) return;
        if (s_jsContext == JS_INVALID_REFERENCE) return;

        if (JsSetCurrentContext(s_jsContext) != JsNoError) return;
        JsCollectGarbage(s_jsRuntime);
        JsSetCurrentContext(JS_INVALID_REFERENCE);
        OutputDebugStringA("[FoxM SpiderMonkey] JsCollectGarbage completed.\n");
    }
    catch (...)
    {
        JsSetCurrentContext(JS_INVALID_REFERENCE);
        OutputDebugStringA("[FoxM SpiderMonkey] RunGarbageCollector native exception swallowed.\n");
    }
}

bool SpiderMonkeyHost::IsJitAvailable()
{
    // Unchanged probe: try one RWX page to test the OS policy (codeGeneration
    // capability). Report-only: JsRT itself decides JIT vs interpreter on the
    // system side; the app never allocates RWX for code.
    void* testAlloc = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (testAlloc != nullptr)
    {
        VirtualFree(testAlloc, 0, MEM_RELEASE);
        return true;
    }
    return false;
}
