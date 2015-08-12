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
		cell_t *eOff;
		pCtx->LocalToPhysAddr(params[5], &eOff);
		const char *err = x->mError;
		*eOff = x->mErrorOffset;
		pCtx->StringToLocal(params[3], params[4], err ? err:"unknown");
		return 0;
	}

	return g_pHandleSys->CreateHandle(g_RegexHandle, (void*)x, pCtx->GetIdentity(), myself->GetIdentity(), NULL);
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

	char *str;
	pCtx->LocalToString(params[2], &str);

	int e = x->Match(str);

	if (e == -1)
	{
		/* there was a match error.  move on. */
		cell_t *res;
		pCtx->LocalToPhysAddr(params[3], &res);
		*res = x->mErrorOffset;
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
		return x->mSubStrings;
	}
}

static cell_t GetRegexSubString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl=static_cast<Handle_t>(params[1]);
	HandleError err;
	HandleSecurity sec;
	sec.pOwner=NULL;
	sec.pIdentity=myself->GetIdentity();

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

	static char buffer[4096];
	const char *ret=x->GetSubstring(params[2], buffer, sizeof(buffer));

	if(!ret)
	{
		return 0;
	}

	pCtx->StringToLocalUTF8(params[3], params[4], ret, NULL);

	return 1;
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
	{NULL,							NULL},
};
