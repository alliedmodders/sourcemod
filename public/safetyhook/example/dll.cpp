#if __has_include(<Windows.h>)
#include <Windows.h>
#elif __has_include(<windows.h>)
#include <windows.h>
#else
#error "Windows.h not found"
#endif

#include <safetyhook.hpp>

SafetyHookInline g_PeekMessageA_hook{};
SafetyHookInline g_PeekMessageW_hook{};

BOOL WINAPI hooked_PeekMessageA(LPMSG msg, HWND wnd, UINT filter_min, UINT filter_max, UINT remove_msg) {
    OutputDebugString("hooked_PeekMessageA\n");
    return g_PeekMessageA_hook.stdcall<BOOL>(msg, wnd, filter_min, filter_max, remove_msg);
}

BOOL WINAPI hooked_PeekMessageW(LPMSG msg, HWND wnd, UINT filter_min, UINT filter_max, UINT remove_msg) {
    OutputDebugString("hooked_PeekMessageW\n");
    return g_PeekMessageW_hook.stdcall<BOOL>(msg, wnd, filter_min, filter_max, remove_msg);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_PeekMessageA_hook = safetyhook::create_inline(
            reinterpret_cast<void*>(&PeekMessageA), reinterpret_cast<void*>(&hooked_PeekMessageA));
        g_PeekMessageW_hook = safetyhook::create_inline(
            reinterpret_cast<void*>(&PeekMessageW), reinterpret_cast<void*>(&hooked_PeekMessageW));
        break;

    case DLL_PROCESS_DETACH:
        g_PeekMessageW_hook.reset();
        g_PeekMessageA_hook.reset();
        break;
    }
    return TRUE;
}