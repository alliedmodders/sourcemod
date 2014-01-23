/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
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

#include <sourcemod_version.h>
#include "extension.h"
#include <stdarg.h>
#include <sm_platform.h>
#include <curl/curl.h>
#include "curlapi.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CurlExt curl_ext;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&curl_ext);

bool CurlExt::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	long flags;
	CURLcode code;

	flags = CURL_GLOBAL_NOTHING;
#if defined PLATFORM_WINDOWS
	flags = CURL_GLOBAL_WIN32;
#endif

	code = curl_global_init(flags);
	if (code)
	{
		smutils->Format(error, maxlength, "%s", curl_easy_strerror(code));
		return false;
	}

	if (!sharesys->AddInterface(myself, &g_webternet))
	{
		smutils->Format(error, maxlength, "Could not add IWebternet interface");
		return false;
	}

	return true;
}

void CurlExt::SDK_OnUnload()
{
	curl_global_cleanup();
}

const char *CurlExt::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *CurlExt::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

