/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
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
#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS		1
#endif

#if defined WIN64 || defined _WIN64
#ifndef PLATFORM_X64
# define PLATFORM_X64           1
#endif
#else
#ifndef PLATFORM_X86
# define PLATFORM_X86           1
#endif
#endif // defined WIN64 || defined _WIN64

#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined snprintf && defined _MSC_VER && _MSC_VER < 1900
#define snprintf _snprintf
#endif
#if !defined stat
#define stat _stat
#endif
#define strcasecmp strcmpi
#define strncasecmp strnicmp
#include <windows.h>
#include <direct.h>
#define PLATFORM_LIB_EXT		"dll"
#define PLATFORM_MAX_PATH		MAX_PATH
#define PLATFORM_SEP_CHAR		'\\'
#define PLATFORM_SEP_ALTCHAR	'/'
#define PLATFORM_SEP			"\\"
#define PLATFORM_EXTERN_C		extern "C" __declspec(dllexport)
#elif defined __linux__ || defined __APPLE__
#if defined __linux__
# define PLATFORM_LINUX			1
# define PLATFORM_LIB_EXT		"so"
#elif defined __APPLE__
# define PLATFORM_APPLE			1
# define PLATFORM_LIB_EXT		"dylib"
#endif
#ifndef PLATFORM_POSIX
# define PLATFORM_POSIX			1
#endif

#if defined __x86_64__
#ifndef PLATFORM_X64
# define PLATFORM_X64           1
#endif
#else
#ifndef PLATFORM_X86
# define PLATFORM_X86           1
#endif
#endif // defined __x86_64__

#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#if defined PLATFORM_APPLE
#include <sys/syslimits.h>
#endif
#define PLATFORM_MAX_PATH		PATH_MAX
#define PLATFORM_SEP_CHAR		'/'
#define PLATFORM_SEP_ALTCHAR	'\\'
#define PLATFORM_SEP			"/"
#define PLATFORM_EXTERN_C		extern "C" __attribute__((visibility("default")))
#endif

#if !defined SOURCEMOD_BUILD
#define SOURCEMOD_BUILD
#endif

#if !defined SM_ARRAYSIZE
#define SM_ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#endif

#if defined PLATFORM_X64
#define PLATFORM_ARCH_FOLDER	"x64" PLATFORM_SEP
#ifdef PLATFORM_WINDOWS
#define PLATFORM_FOLDER 		"win64" PLATFORM_SEP
#elif defined PLATFORM_LINUX
#define PLATFORM_FOLDER 		"linux64" PLATFORM_SEP
#elif defined PLATFORM_APPLE
#define PLATFORM_FOLDER 		"osx64" PLATFORM_SEP
#endif
#else
#define PLATFORM_ARCH_FOLDER	 ""
#ifdef PLATFORM_WINDOWS
#define PLATFORM_FOLDER 		"win32" PLATFORM_SEP
#elif defined PLATFORM_LINUX
#define PLATFORM_FOLDER 		"linux32" PLATFORM_SEP
#elif defined PLATFORM_APPLE
#define PLATFORM_FOLDER 		"osx32"	PLATFORM_SEP
#endif
#endif // defined PLATFORM_X64

#endif //_INCLUDE_SOURCEMOD_PLATFORM_H_

