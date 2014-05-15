/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2011 AlliedModders LLC.  All rights reserved.
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


#include "extension.h"
#include "clientnatives.h"
#include "iserver.h"
#include "iclient.h"

static cell_t smn_InactivateClient(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}


	if (!iserver)
	{
		return pContext->ThrowNativeError("IServer interface not supported, file a bug report.");
	}

	IClient* pClient = iserver->GetClient(params[1] - 1);
	if (pClient)
	{
		pClient->Inactivate();
	}
	else
	{
		pContext->ThrowNativeError("Could not get IClient for client %d", params[1]);
	}

	return 1;
}

static cell_t smn_ReconnectClient(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	
	if (player == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}

	if (!iserver)
	{
		return pContext->ThrowNativeError("IServer interface not supported, file a bug report.");
	}

	IClient* pClient = iserver->GetClient(params[1] - 1);
	if (pClient)
	{
		pClient->Reconnect();
	}
	else
	{
		pContext->ThrowNativeError("Could not get IClient for client %d", params[1]);
	}
	return 1;
}

sp_nativeinfo_t g_ClientNatives[] = 
{
	{ "InactivateClient",	smn_InactivateClient	},
	{ "ReconnectClient",	smn_ReconnectClient		},
	{ NULL,					NULL					},
};