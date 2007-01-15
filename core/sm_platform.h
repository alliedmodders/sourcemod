#ifndef _INCLUDE_SOURCEMOD_PLATFORM_H_
#define _INCLUDE_SOURCEMOD_PLATFORM_H_

/**
 * @file Contains platform-specific macros for abstraction.
 */

#if defined WIN32 || defined WIN64
#define PLATFORM_WINDOWS
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined snprintf
#define snprintf _snprintf
#endif
#if !defined stat
#define stat _stat
#endif
#define strcasecmp strcmpi
#include <windows.h>
#include <direct.h>
#define PLATFORM_LIB_EXT		"dll"
#define PLATFORM_MAX_PATH		MAX_PATH
#define PLATFORM_SEP_CHAR		'\\'
#define PLATFORM_SEP_ALTCHAR	'/'
#define PLATFORM_EXTERN_C		extern "C" __declspec(dllexport)
#else if defined __linux__
#define PLATFORM_LINUX
#define PLATFORM_POSIX
#include <dirent.h>
#include <errno.h>
#define PLATFORM_MAX_PATH		PATH_MAX
#define PLATFORM_LIB_EXT		"so"
#define PLATFORM_SEP_CHAR		'/'
#define PLATFORM_SEP_ALTCHAR	'\\'
#define PLATFORM_EXTERN_C		extern "C" __attribute__((visibility("default")))
#endif

#endif //_INCLUDE_SOURCEMOD_PLATFORM_H_
