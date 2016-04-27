/**
* vim: set ts=4 sw=4 tw=99 noet :
* =============================================================================
* SourceMod
* Copyright (C) 2004-2016 AlliedModders LLC.  All rights reserved.
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

#include "common_logic.h"
#include <IGameHelpers.h>
#include <am-string.h>

#include <inttypes.h>

static cell_t GetServerAuthId(IPluginContext *pContext, const cell_t *params)
{
	cell_t *pOut;
	pContext->LocalToPhysAddr(params[2], &pOut);

	switch ((AuthIdType)params[1])
	{
	case AuthIdType::Steam3:
		gamehelpers->GetServerSteam3Id((char *)pOut, params[3]);
		break;

	case AuthIdType::SteamId64:
		ke::SafeSprintf((char *)pOut, params[3], "%" PRIu64, gamehelpers->GetServerSteamId64());
		break;
	default:
		return pContext->ThrowNativeError("Unsupported AuthIdType (%d) for GetServerAuthId.", params[1]);
	}

	return 1;
}

static cell_t GetServerAccountId(IPluginContext *pContext, const cell_t *params)
{
	return (cell_t)(gamehelpers->GetServerSteamId64() & 0xFFFFFFFF);
}

REGISTER_NATIVES(halflifeNatives)
{
	{ "GetServerAuthId",			GetServerAuthId },
	{ "GetServerSteamAccountId",	GetServerAccountId },
	{ nullptr, 						nullptr }
};
