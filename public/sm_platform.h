/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_PLATFORM_H_
#define _INCLUDE_SOURCEMOD_PLATFORM_H_

/**
 * @file sm_platform.h
 * @brief Contains platform-specific macros for abstraction.
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
#elif defined __linux__
#define PLATFORM_LINUX
#define PLATFORM_POSIX
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#define PLATFORM_MAX_PATH		PATH_MAX
#define PLATFORM_LIB_EXT		"so"
#define PLATFORM_SEP_CHAR		'/'
#define PLATFORM_SEP_ALTCHAR	'\\'
#define PLATFORM_EXTERN_C		extern "C" __attribute__((visibility("default")))
#endif

#if !defined SOURCEMOD_BUILD
#define SOURCEMOD_BUILD
#endif

#endif //_INCLUDE_SOURCEMOD_PLATFORM_H_
