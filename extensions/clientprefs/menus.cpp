/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Client Preferences Extension
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

#include "menus.h"

ClientMenuHandler g_Handler;
AutoMenuHandler g_AutoHandler;

void ClientMenuHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	ItemDrawInfo draw;

	const char *info = menu->GetItemInfo(item, &draw);

	AutoMenuData *data = (AutoMenuData *)strtoul(info, NULL, 16);

	if (data->handler->forward != NULL)
	{
		data->handler->forward->PushCell(client);
		data->handler->forward->PushCell(CookieMenuAction_SelectOption);
		data->handler->forward->PushCell(data->datavalue);
		data->handler->forward->PushString("");
		data->handler->forward->PushCell(0);
		data->handler->forward->Execute(NULL);
	}

	if (!data->handler->isAutoMenu)
	{
		return;
	}

	IBaseMenu *submenu = menus->GetDefaultStyle()->CreateMenu(&g_AutoHandler, g_ClientPrefs.GetIdentity());

	char message[256];

	Translate(message, sizeof(message), "%T:", 2, NULL, "Choose Option", &client);
	submenu->SetDefaultTitle(message);

	if (data->type == CookieMenu_YesNo || data->type == CookieMenu_YesNo_Int)
	{
		Translate(message, sizeof(message), "%T", 2, NULL, "Yes", &client);
		submenu->AppendItem(info, message);

		Translate(message, sizeof(message), "%T", 2, NULL, "No", &client);
		submenu->AppendItem(info, message);
	}
	else if (data->type == CookieMenu_OnOff || data->type == CookieMenu_OnOff_Int)
	{
		Translate(message, sizeof(message), "%T", 2, NULL, "On", &client);
		submenu->AppendItem(info, message);

		Translate(message, sizeof(message), "%T", 2, NULL, "Off", &client);
		submenu->AppendItem(info, message);
	}

	submenu->Display(client, 0, NULL);
}

unsigned int ClientMenuHandler::OnMenuDisplayItem(IBaseMenu *menu, 
										   int client, 
										   IMenuPanel *panel,
										   unsigned int item, 
										   const ItemDrawInfo &dr)
{
	ItemDrawInfo draw;

	const char *info = menu->GetItemInfo(item, &draw);

	AutoMenuData *data = (AutoMenuData *)strtoul(info, NULL, 16);

	if (data->handler->forward != NULL)
	{
		char buffer[100];
		g_pSM->Format(buffer, sizeof(buffer), "%s", dr.display);

		data->handler->forward->PushCell(client);
		data->handler->forward->PushCell(CookieMenuAction_DisplayOption);
		data->handler->forward->PushCell(data->datavalue);
		data->handler->forward->PushStringEx(buffer, sizeof(buffer), SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
		data->handler->forward->PushCell(sizeof(buffer));
		data->handler->forward->Execute(NULL);

		ItemDrawInfo newdraw(buffer, draw.style);

		return panel->DrawItem(newdraw);
	}

	return 0;
}

void AutoMenuHandler::OnMenuSelect(SourceMod::IBaseMenu *menu, int client, unsigned int item)
{
	static const char settings[CookieMenu_Elements][2][4] = { {"yes", "no"}, {"1", "0"}, {"on", "off"}, {"1", "0"} };
	ItemDrawInfo draw;

	const char *info = menu->GetItemInfo(item, &draw);

	AutoMenuData *data = (AutoMenuData *)strtoul(info, NULL, 16);

	g_CookieManager.SetCookieValue(data->pCookie, client, settings[data->type][item]);
	
	char message[255];
	char *value;
	g_CookieManager.GetCookieValue(data->pCookie, client, &value);
	Translate(message, sizeof(message), "[SM] %T", 4, NULL, "Cookie Changed Value", &client, &(data->pCookie->name), value);

	gamehelpers->TextMsg(client, 3, message);
}

void AutoMenuHandler::OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
{
	HandleSecurity sec = HandleSecurity(g_ClientPrefs.GetIdentity(), g_ClientPrefs.GetIdentity());
	HandleError err = handlesys->FreeHandle(menu->GetHandle(), &sec);
	if (HandleError_None != err)
	{
		g_pSM->LogError(myself, "Error %d when attempting to free automenu handle", err);
	}
}
