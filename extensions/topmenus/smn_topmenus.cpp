/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod TopMenus Extension
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

#include "extension.h"
#include "TopMenuManager.h"
#include "TopMenu.h"

HandleType_t hTopMenuType;

class TopMenuHandle : public IHandleTypeDispatch
{
public: 
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		ITopMenu *pTopMenu = (ITopMenu *)object;
		g_TopMenus.DestroyTopMenu(pTopMenu);
	}
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		*pSize = ((TopMenu *)object)->CalcMemUsage();
		return true;
	}
} s_TopMenuHandle;

void Initialize_Natives()
{
	hTopMenuType = handlesys->CreateType("ITopMenu",
		&s_TopMenuHandle,
		0,
		NULL,
		NULL,
		myself->GetIdentity(),
		NULL);
}

void Shutdown_Natives()
{
	handlesys->RemoveType(hTopMenuType, myself->GetIdentity());
}

enum TopMenuAction
{
	TopMenuAction_DisplayOption = 0,
	TopMenuAction_DisplayTitle = 1,
	TopMenuAction_SelectOption = 2,
	TopMenuAction_DrawOption = 3,
	TopMenuAction_RemoveObject = 4,
};

class TopMenuCallbacks : public ITopMenuObjectCallbacks
{
public:
	TopMenuCallbacks(IPluginFunction *pFunction) : m_pFunction(pFunction)
	{		
	}

	unsigned int OnTopMenuDrawOption(ITopMenu *menu,
		int client,
		unsigned int object_id)
	{
		char buffer[2] = {ITEMDRAW_DEFAULT, 0x0};
		m_pFunction->PushCell(m_hMenuHandle);
		m_pFunction->PushCell(TopMenuAction_DrawOption);
		m_pFunction->PushCell(object_id);
		m_pFunction->PushCell(client);
		m_pFunction->PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
		m_pFunction->PushCell(sizeof(buffer));
		m_pFunction->Execute(NULL);
		return (unsigned int)buffer[0];
	}

	void OnTopMenuDisplayOption(ITopMenu *menu,
		int client, 
		unsigned int object_id,
		char buffer[], 
		size_t maxlength)
	{
		m_pFunction->PushCell(m_hMenuHandle);
		m_pFunction->PushCell(TopMenuAction_DisplayOption);
		m_pFunction->PushCell(object_id);
		m_pFunction->PushCell(client);
		m_pFunction->PushStringEx(buffer, maxlength, 0, SM_PARAM_COPYBACK);
		m_pFunction->PushCell(maxlength);
		m_pFunction->Execute(NULL);
	}

	void OnTopMenuDisplayTitle(ITopMenu *menu,
		int client,
		unsigned int object_id,
		char buffer[],
		size_t maxlength)
	{
		m_pFunction->PushCell(m_hMenuHandle);
		m_pFunction->PushCell(TopMenuAction_DisplayTitle);
		m_pFunction->PushCell(object_id);
		m_pFunction->PushCell(client);
		m_pFunction->PushStringEx(buffer, maxlength, 0, SM_PARAM_COPYBACK);
		m_pFunction->PushCell(maxlength);
		m_pFunction->Execute(NULL);
	}

	void OnTopMenuSelectOption(ITopMenu *menu, 
		int client, 
		unsigned int object_id)
	{
		unsigned int old_reply = playerhelpers->SetReplyTo(SM_REPLY_CHAT);
		m_pFunction->PushCell(m_hMenuHandle);
		m_pFunction->PushCell(TopMenuAction_SelectOption);
		m_pFunction->PushCell(object_id);
		m_pFunction->PushCell(client);
		m_pFunction->PushString("");
		m_pFunction->PushCell(0);
		m_pFunction->Execute(NULL);
		playerhelpers->SetReplyTo(old_reply);
	}

	void OnTopMenuObjectRemoved(ITopMenu *menu, unsigned int object_id)
	{
		m_pFunction->PushCell(m_hMenuHandle);
		m_pFunction->PushCell(TopMenuAction_RemoveObject);
		m_pFunction->PushCell(object_id);
		m_pFunction->PushCell(0);
		m_pFunction->PushString("");
		m_pFunction->PushCell(0);
		m_pFunction->Execute(NULL);

		delete this;
	}

	Handle_t m_hMenuHandle;
	IPluginFunction *m_pFunction;
};

static cell_t CreateTopMenu(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *func = pContext->GetFunctionById(params[1]);
	if (func == NULL)
	{
		return pContext	->ThrowNativeError("Invalid function specified");
	}

	TopMenuCallbacks *cb = new TopMenuCallbacks(func);

	ITopMenu *pMenu = g_TopMenus.CreateTopMenu(cb);

	if (!pMenu)
	{
		delete cb;
		return BAD_HANDLE;
	}

	Handle_t hndl = handlesys->CreateHandle(hTopMenuType, 
		pMenu,
		pContext->GetIdentity(),
		myself->GetIdentity(),
		NULL);
	if (hndl == 0)
	{
		g_TopMenus.DestroyTopMenu(pMenu);
		return BAD_HANDLE;
	}

	cb->m_hMenuHandle = hndl;

	return hndl;
}

static cell_t LoadTopMenuConfig(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	char *file, *err_buf;
	pContext->LocalToString(params[2], &file);
	pContext->LocalToString(params[3], &err_buf);

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	return pMenu->LoadConfiguration(path, err_buf, params[4]) ? 1 : 0;
}

static cell_t AddToTopMenu(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	IPluginFunction *func = pContext->GetFunctionById(params[4]);
	if (func == NULL)
	{
		return pContext	->ThrowNativeError("Invalid function specified");
	}

	TopMenuCallbacks *cb = new TopMenuCallbacks(func);

	char *name, *cmdname, *info_string = NULL;
	pContext->LocalToString(params[2], &name);
	pContext->LocalToString(params[6], &cmdname);

	if (params[0] >= 8)
	{
		pContext->LocalToString(params[8], &info_string);
	}

	TopMenuObjectType obj_type = (TopMenuObjectType)params[3];
	
	unsigned int object_id;
	if ((object_id = pMenu->AddToMenu2(name,
		obj_type,
		cb,
		pContext->GetIdentity(),
		cmdname,
		params[7], 
		params[5],
		info_string)) == 0)
	{
		delete cb;
		return 0;
	}

	cb->m_hMenuHandle = params[1];

	return object_id;
}

static cell_t RemoveFromTopMenu(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	pMenu->RemoveFromMenu(params[2]);

	return 1;
}

static cell_t FindTopMenuCategory(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	char *name;
	pContext->LocalToString(params[2], &name);

	return pMenu->FindCategory(name);
}

static cell_t DisplayTopMenu(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	int client = params[2];
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (!player)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	return pMenu->DisplayMenu(client, 0, (TopMenuPosition)params[3]);
}

static cell_t DisplayTopMenuCategory(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	TopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	int client = params[3];
	IGamePlayer *player = playerhelpers->GetGamePlayer(client);
	if (!player)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!player->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	return pMenu->DisplayMenuAtCategory(client, params[2]);
}

static cell_t GetTopMenuInfoString(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	const char *str;
	if ((str = pMenu->GetObjectInfoString(params[2])) == NULL)
	{
		return pContext->ThrowNativeError("Invalid menu object %d", params[2]);
	}

	char *buffer;
	pContext->LocalToString(params[3], &buffer);

	return strncopy(buffer, str, params[4]);
}

static cell_t GetTopMenuName(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	ITopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	const char *str;
	if ((str = pMenu->GetObjectName(params[2])) == NULL)
	{
		return pContext->ThrowNativeError("Invalid menu object %d", params[2]);
	}

	char *buffer;
	pContext->LocalToString(params[3], &buffer);

	return strncopy(buffer, str, params[4]);
}

static cell_t SetTopMenuTitleCaching(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	TopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	bool cache_titles = params[2]==1?true:false;
	pMenu->SetTitleCaching(cache_titles);
	return 0;
}

static cell_t TopMenu_AddItem(IPluginContext *pContext, const cell_t *params)
{
	cell_t new_params[] = {
		8,
		params[1],				// this
		params[2],				// name
		TopMenuObject_Item,		// type
		params[3],				// handler
		params[4],				// parent
		params[5],				// cmdname
		params[6],				// flags
		params[7],				// info_string
	};
	return AddToTopMenu(pContext, new_params);
}

static cell_t TopMenu_AddCategory(IPluginContext *pContext, const cell_t *params)
{
	cell_t new_params[] = {
		8,
		params[1],				// this
		params[2],				// name
		TopMenuObject_Category,	// type
		params[3],				// handler
		0,						// parent
		params[4],				// cmdname
		params[5],				// flags
		params[6],				// info_string
	};
	return AddToTopMenu(pContext, new_params);
}

static cell_t TopMenu_FromHandle(IPluginContext *pContext, const cell_t *params)
{
	HandleError err;
	TopMenu *pMenu;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], hTopMenuType, &sec, (void **)&pMenu))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	return params[1];
}

sp_nativeinfo_t g_TopMenuNatives[] = 
{
	{"AddToTopMenu",			AddToTopMenu},
	{"CreateTopMenu",			CreateTopMenu},
	{"DisplayTopMenu",			DisplayTopMenu},
	{"DisplayTopMenuCategory",	DisplayTopMenuCategory},
	{"LoadTopMenuConfig",		LoadTopMenuConfig},
	{"RemoveFromTopMenu",		RemoveFromTopMenu},
	{"FindTopMenuCategory",		FindTopMenuCategory},
	{"GetTopMenuInfoString",	GetTopMenuInfoString},
	{"GetTopMenuObjName",		GetTopMenuName},
	{"SetTopMenuTitleCaching",		SetTopMenuTitleCaching},

	// Transitional variants.
	{"TopMenu.TopMenu",			CreateTopMenu},
	{"TopMenu.AddItem",			TopMenu_AddItem},
	{"TopMenu.AddCategory",		TopMenu_AddCategory},
	{"TopMenu.Display",			DisplayTopMenu},
	{"TopMenu.DisplayCategory",	DisplayTopMenuCategory},
	{"TopMenu.LoadConfig",		LoadTopMenuConfig},
	{"TopMenu.Remove",			RemoveFromTopMenu},
	{"TopMenu.FindCategory",	FindTopMenuCategory},
	{"TopMenu.GetInfoString",	GetTopMenuInfoString},
	{"TopMenu.GetObjName",		GetTopMenuName},
	{"TopMenu.CacheTitles.set",	SetTopMenuTitleCaching},
	{"TopMenu.FromHandle",      TopMenu_FromHandle},

	{NULL,					NULL},
};
