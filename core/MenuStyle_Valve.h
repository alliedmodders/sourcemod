/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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
	bool OnClientCommand(int client);
public: //BaseMenuStyle
	CBaseMenuPlayer *GetMenuPlayer(int client);
	void SendDisplay(int client, IMenuPanel *display);
	bool DoClientMenu(int client, CBaseMenu *menu, IMenuHandler *mh, unsigned int time);
	bool DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time);
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModVSPReceived(IServerPluginCallbacks *iface);
public: //IMenuStyle
	const char *GetStyleName();
	IMenuPanel *CreatePanel();
	IBaseMenu *CreateMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner);
	unsigned int GetMaxPageItems();
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
	bool Display(int client, unsigned int time);
	void VoteDisplay(int client, unsigned int maxTime);
public: //CBaseMenu
	void Cancel_Finally();
private:
	Color m_IntroColor;
	char m_IntroMsg[128];
};

extern ValveMenuStyle g_ValveMenuStyle;

#endif //_INCLUDE_MENUSTYLE_VALVE_H
