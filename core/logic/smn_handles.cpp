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

#include "common_logic.h"
#include <IHandleSys.h>
#include <IPluginSys.h>

using namespace SourceMod;

static cell_t sm_IsValidHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	HandleError err = handlesys->ReadHandle(hndl, 0, NULL, NULL);

	if (err != HandleError_Access
		&& err != HandleError_None)
	{
		return 0;
	}

	return 1;
}

static cell_t sm_CloseHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);

	if (!hndl)
		return 0;

	HandleSecurity sec;

	sec.pIdentity = NULL;
	sec.pOwner = pContext->GetIdentity();

	HandleError err = handlesys->FreeHandle(hndl, &sec);
  
	if (err == HandleError_None)
	{
		return 1;
	} else if (err == HandleError_Access) {
		return 0;
	} else {
		return pContext->ThrowNativeError("Handle %x is invalid (error %d)", hndl, err);
	}
}

static cell_t sm_CloneHandle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t new_hndl;
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError err;
	IdentityToken_t *pNewOwner;
	
	if (params[2] == 0)
	{
		pNewOwner = pContext->GetIdentity();
	} else {
		Handle_t hPlugin = static_cast<Handle_t>(params[2]);
		IPlugin *pPlugin = pluginsys->PluginFromHandle(hPlugin, &err);
		if (!pPlugin)
		{
			return pContext->ThrowNativeError("Plugin handle %x is invalid (error %d)", hndl, err);
		}
		pNewOwner = pPlugin->GetIdentity();
	}

	err = handlesys->CloneHandle(hndl, &new_hndl, pNewOwner, NULL);

	if (err == HandleError_Access)
	{
		return 0;
	} else if (err == HandleError_None) {
		return new_hndl;
	} else {
		return pContext->ThrowNativeError("Handle %x cannot be cloned because it is invalid (error %d)", hndl, err);
	}
}

static cell_t sm_GetMyHandle(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = pluginsys->FindPluginByContext(pContext->GetContext());

	return pPlugin->GetMyHandle();
}

REGISTER_NATIVES(handles)
{
	{"IsValidHandle",			sm_IsValidHandle},
	{"CloseHandle",				sm_CloseHandle},
	{"CloneHandle",				sm_CloneHandle},
	{"GetMyHandle",				sm_GetMyHandle},
	{NULL,						NULL},
};
