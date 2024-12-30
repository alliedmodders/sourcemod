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

#ifndef _INCLUDE_MENUSTYLE_VALVE_H
#define _INCLUDE_MENUSTYLE_VALVE_H

#include "sm_globals.h"
#include "MenuManager.h"
#include "MenuStyle_Base.h"
#include "sourcemm_api.h"
#include "KeyValues.h"
#include "sm_fastlink.h"
#include <compat_wrappers.h>
#include "logic/common_logic.h"
#include "AutoHandleRooter.h"

using namespace SourceMod;

class CValveMenuPlayer : public CBaseMenuPlayer
{
public:
	CValveMenuPlayer() : curPrioLevel(1)
	{
	}
	int curPrioLevel;
};

class CValveMenu;
class CValveMenuDisplay;

class ValveMenuStyle : 
	public SMGlobalClass,
	public BaseMenuStyle
{
public:
	ValveMenuStyle();
	bool OnClientCommand(int client, const char *cmdname, const CCommand &cmd);
public: //BaseMenuStyle
	CBaseMenuPlayer *GetMenuPlayer(int client);
	void SendDisplay(int client, IMenuPanel *display);
	bool DoClientMenu(int client, CBaseMenu *menu, unsigned int first_item, IMenuHandler *mh, unsigned int time);
	bool DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IMenuStyle
	const char *GetStyleName();
	IMenuPanel *CreatePanel();
	IBaseMenu *CreateMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner);
	unsigned int GetMaxPageItems();
	unsigned int GetApproxMemUsage();
	bool IsSupported() { return true; }
private:
	void HookCreateMessage(edict_t *pEdict, DIALOG_TYPE type, KeyValues *kv, IServerPluginCallbacks *plugin);
private:
	CValveMenuPlayer *m_players;
};

class CValveMenu;

class CValveMenuDisplay : public IMenuPanel
{
public:
	CValveMenuDisplay();
	CValveMenuDisplay(CValveMenu *pMenu);
	~CValveMenuDisplay();
public:
	IMenuStyle *GetParentStyle();
	void Reset();
	void DrawTitle(const char *text, bool onlyIfEmpty=false);
	unsigned int DrawItem(const ItemDrawInfo &item);
	bool DrawRawLine(const char *rawline);
	bool SendDisplay(int client, IMenuHandler *handler, unsigned int time);
	bool SetExtOption(MenuOption option, const void *valuePtr);
	bool CanDrawItem(unsigned int drawFlags);
	void SendRawDisplay(int client, int priority, unsigned int time);
	void DeleteThis();
	bool SetSelectableKeys(unsigned int keymap);
	unsigned int GetCurrentKey();
	bool SetCurrentKey(unsigned int key);
	int GetAmountRemaining();
	unsigned int GetApproxMemUsage();
	bool DirectSet(const char *str) { return false; }
private:
	KeyValues *m_pKv;
	unsigned int m_NextPos;
	bool m_TitleDrawn;
};

class CValveMenu : public CBaseMenu
{
	friend class CValveMenuDisplay;
public:
	CValveMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner);
public: //IBaseMenu
	bool SetExtOption(MenuOption option, const void *valuePtr);
	IMenuPanel *CreatePanel();
	bool GetExitButton();
	bool SetExitButton(bool set);
	bool SetPagination(unsigned int itemsPerPage);
	bool Display(int client, unsigned int time, IMenuHandler *alt_handler=NULL);
	bool DisplayAtItem(int client,
		unsigned int time,
		unsigned int start_item,
		IMenuHandler *alt_handler/* =NULL */);
	void SetMenuOptionFlags(unsigned int flags);
	unsigned int GetApproxMemUsage();
public: //CBaseMenu
	void Cancel_Finally();
private:
	Color m_IntroColor;
	char m_IntroMsg[128];
};

extern ValveMenuStyle g_ValveMenuStyle;

#endif //_INCLUDE_MENUSTYLE_VALVE_H
