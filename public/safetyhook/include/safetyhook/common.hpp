#pragma once

#if defined(_MSC_VER)
#define SAFETYHOOK_COMPILER_MSVC 1
#define SAFETYHOOK_COMPILER_GCC 0
#define SAFETYHOOK_COMPILER_CLANG 0
#elif defined(__GNUC__)
#define SAFETYHOOK_COMPILER_MSVC 0
#define SAFETYHOOK_COMPILER_GCC 1
#define SAFETYHOOK_COMPILER_CLANG 0
#elif defined(__clang__)
#define SAFETYHOOK_COMPILER_MSVC 0
#define SAFETYHOOK_COMPILER_GCC 0
#define SAFETYHOOK_COMPILER_CLANG 1
#else
#error "Unsupported compiler"
#endif

#if SAFETYHOOK_COMPILER_MSVC
#if defined(_M_IX86)
#define SAFETYHOOK_ARCH_X86_32 1
#define SAFETYHOOK_ARCH_X86_64 0
#elif defined(_M_X64)
#define SAFETYHOOK_ARCH_X86_32 0
#define SAFETYHOOK_ARCH_X86_64 1
#else
#error "Unsupported architecture"
#endif
#elif SAFETYHOOK_COMPILER_GCC || SAFETYHOOK_COMPILER_CLANG
#if defined(__i386__)
#define SAFETYHOOK_ARCH_X86_32 1
#define SAFETYHOOK_ARCH_X86_64 0
#elif defined(__x86_64__)
#define SAFETYHOOK_ARCH_X86_32 0
#define SAFETYHOOK_ARCH_X86_64 1
#else
#error "Unsupported architecture"
#endif
#endif

#if defined(_WIN32)
#define SAFETYHOOK_OS_WINDOWS 1
#define SAFETYHOOK_OS_LINUX 0
#elif defined(__linux__)
#define SAFETYHOOK_OS_WINDOWS 0
#define SAFETYHOOK_OS_LINUX 1
#else
#error "Unsupported OS"
#endif

#if SAFETYHOOK_OS_WINDOWS
#if SAFETYHOOK_COMPILER_MSVC
#define SAFETYHOOK_CCALL __cdecl
#define SAFETYHOOK_STDCALL __stdcall
#define SAFETYHOOK_FASTCALL __fastcall
#define SAFETYHOOK_THISCALL __thiscall
#elif SAFETYHOOK_COMPILER_GCC || SAFETYHOOK_COMPILER_CLANG
#define SAFETYHOOK_CCALL __attribute__((cdecl))
#define SAFETYHOOK_STDCALL __attribute__((stdcall))
#define SAFETYHOOK_FASTCALL __attribute__((fastcall))
#define SAFETYHOOK_THISCALL __attribute__((thiscall))
#endif
#else
#define SAFETYHOOK_CCALL
#define SAFETYHOOK_STDCALL
#define SAFETYHOOK_FASTCALL
#define SAFETYHOOK_THISCALL
#endif

#if SAFETYHOOK_COMPILER_MSVC
#define SAFETYHOOK_NOINLINE __declspec(noinline)
#elif SAFETYHOOK_COMPILER_GCC || SAFETYHOOK_COMPILER_CLANG
#define SAFETYHOOK_NOINLINE __attribute__((noinline))
#endif
