/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Regular Expressions Extension
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
#include <sh_string.h>
#include "pcre.h"
#include "posix_map.h"
#include "CRegEx.h"
using namespace SourceHook;

/**
 * @file extension.cpp
 * @brief Implement Regex extension code here.
 */

RegexExtension g_RegexExtension;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_RegexExtension);

RegexHandler g_RegexHandler;
HandleType_t g_RegexHandle=0;



bool RegexExtension::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	g_pShareSys->AddNatives(myself,regex_natives);
	g_RegexHandle = g_pHandleSys->CreateType("Regex", &g_RegexHandler, 0, NULL, NULL, myself->GetIdentity(), NULL);
	return true;
}

void RegexExtension::SDK_OnUnload()
{
	g_pHandleSys->RemoveType(g_RegexHandle, myself->GetIdentity());

}

const char *RegexExtension::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *RegexExtension::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

static cell_t CompileRegex(IPluginContext *pCtx, const cell_t *params)
{
	char *regex;
	pCtx->LocalToString(params[1], &regex);

	RegEx *x = new RegEx();
	
	if (x->Compile(regex, params[2]) == 0)
	{
		cell_t *eError;
		pCtx->LocalToPhysAddr(params[5], &eError);
		const char *err = x->mError;
		// Convert error code to posix error code but use pcre's error string since it is more detailed.
		*eError = pcre_posix_compile_error_map[x->mErrorCode];
		pCtx->StringToLocal(params[3], params[4], err ? err:"unknown");
		delete x;
		return 0;
	}

	HandleError error = HandleError_None;
	Handle_t regexHandle = g_pHandleSys->CreateHandle(g_RegexHandle, (void*)x, pCtx->GetIdentity(), myself->GetIdentity(), &error);
	if (!regexHandle || error != HandleError_None)
	{
		delete x;
		pCtx->ReportError("Allocation of regex handle failed, error code #%d", error);
		return 0;
	}
	
	return regexHandle;
}


static cell_t MatchRegex(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	RegEx *x;
	if ((err=g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}
	if (!x)
	{
		pCtx->ThrowNativeError("Regex data not found\n");
		return 0;
	}

	size_t offset = 0;
	if (params[0] >= 4)
	{
		offset = static_cast<size_t>(params[4]);
	}

	char *str;
	pCtx->LocalToString(params[2], &str);

	int e = x->Match(str, offset);
	if (e == -1)
	{
		/* there was a match error.  move on. */
		cell_t *res;
		pCtx->LocalToPhysAddr(params[3], &res);
		*res = x->mErrorCode;
		/* only clear the match results, since the regex object
		   may still be referenced later */
		x->ClearMatch();

		return -1;
	}
	else if (e == 0) 
	{
		/* only clear the match results, since the regex object
		   may still be referenced later */
		x->ClearMatch();

		return 0;
	} 
	else 
	{
		return x->mMatches[0].mSubStringCount;
	}
}

static cell_t MatchRegexAll(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	RegEx *x;

	if ((err = g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}

	if (!x)
	{
		pCtx->ThrowNativeError("Regex data not found\n");

		return 0;
	}

	char *str;
	pCtx->LocalToString(params[2], &str);

	int e = x->MatchAll(str);

	if (e == -1)
	{
		/* there was a match error.  move on. */
		cell_t *res;
		pCtx->LocalToPhysAddr(params[3], &res);
		*res = x->mErrorCode;
		/* only clear the match results, since the regex object
		may still be referenced later */
		x->ClearMatch();

		return -1;
	}
	else if (e == 0)
	{
		/* only clear the match results, since the regex object
		may still be referenced later */
		x->ClearMatch();

		return 0;
	}
	else
	{
		return x->mMatchCount;
	}
}

static cell_t GetRegexSubString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl=static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner=NULL;
	sec.pIdentity=myself->GetIdentity();

	int match = 0;

	RegEx *x;

	if ((err=g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}

	if (!x)
	{
		pCtx->ThrowNativeError("Regex data not found\n");
		return 0;
	}

	if (params[0] >= 5)
	{
		match = params[5];
	}

	if(match >= x->mMatchCount || match < 0)
		return pCtx->ThrowNativeError("Invalid match index passed.\n");

	char *buffer;
	pCtx->LocalToString(params[3], &buffer);
	
	return x->GetSubstring(params[2], buffer, params[4], match);
}

static cell_t GetRegexMatchCount(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	RegEx *x;

	if ((err = g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("Regex data not found\n");
	}

	return x->mMatchCount;
}

static cell_t GetRegexCaptureCount(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	RegEx *x;

	if ((err = g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("Regex data not found\n");
	}

	if (params[2] >= x->mMatchCount || params[2] < 0)
		return pCtx->ThrowNativeError("Invalid match index passed.\n");

	return x->mMatches[params[2]].mSubStringCount;
}

static cell_t GetRegexOffset(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner = NULL;
	sec.pIdentity = myself->GetIdentity();

	RegEx *x;

	if ((err = g_pHandleSys->ReadHandle(hndl, g_RegexHandle, &sec, (void **)&x)) != HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid regex handle %x (error %d)", hndl, err);
	}

	if (!x)
	{
		return pCtx->ThrowNativeError("Regex data not found\n");
	}

	if (params[2] >= x->mMatchCount || params[2] < 0)
		return pCtx->ThrowNativeError("Invalid match index passed.\n");

	return x->mMatches[params[2]].mVector[1];
}

void RegexHandler::OnHandleDestroy(HandleType_t type, void *object)
{
	RegEx *x = (RegEx *)object;

	x->Clear();
	delete x;
}

const sp_nativeinfo_t regex_natives[] = 
{
	{"GetRegexSubString",			GetRegexSubString},
	{"MatchRegex",					MatchRegex},
	{"CompileRegex",				CompileRegex},

	// Methodmap versions/
	{"Regex.GetSubString",		GetRegexSubString},
	{"Regex.Match",				MatchRegex},
	{"Regex.Regex",				CompileRegex},
	{"Regex.MatchAll",			MatchRegexAll},
	{"Regex.MatchCount",		GetRegexMatchCount},
	{"Regex.CaptureCount",		GetRegexCaptureCount},
	{"Regex.MatchOffset",			GetRegexOffset},
	{NULL,							NULL},
};
