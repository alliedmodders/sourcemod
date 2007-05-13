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

#ifndef _INCLUDE_MENUSTYLE_RADIO_H
#define _INCLUDE_MENUSTYLE_RADIO_H

#include "sm_globals.h"
#include "MenuManager.h"
#include "MenuStyle_Base.h"
#include "sourcemm_api.h"
#include <IPlayerHelpers.h>
#include <IUserMessages.h>
#include "sm_fastlink.h"

using namespace SourceMod;

class CRadioStyle : 
	public BaseMenuStyle,
	public SMGlobalClass,
	public IUserMessageListener
{
public:
	CRadioStyle();
public: //SMGlobalClass
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModShutdown();
public: //BaseMenuStyle
	CBaseMenuPlayer *GetMenuPlayer(int client);
	void SendDisplay(int client, IMenuDisplay *display);
public: //IMenuStyle
	const char *GetStyleName();
	IMenuDisplay *CreateDisplay();
	IBaseMenu *CreateMenu();
	unsigned int GetMaxPageItems();
public: //IUserMessageListener
	void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter);
	void OnUserMessageSent(int msg_id);
public:
	bool IsSupported();
	bool OnClientCommand(int client);
private:
	CBaseMenuPlayer *m_players;
};

class CRadioMenu;

class CRadioDisplay : public IMenuDisplay
{
public:
	CRadioDisplay();
	CRadioDisplay(CRadioMenu *menu);
public: //IMenuDisplay
	IMenuStyle *GetParentStyle();
	void Reset();
	void DrawTitle(const char *text, bool onlyIfEmpty=false);
	unsigned int DrawItem(const ItemDrawInfo &item);
	bool DrawRawLine(const char *rawline);
	bool SetExtOption(MenuOption option, const void *valuePtr);
	bool CanDrawItem(unsigned int drawFlags);
	bool SendDisplay(int client, IMenuHandler *handler, unsigned int time);
	void DeleteThis();
	void SendRawDisplay(int client, unsigned int time);
private:
	String m_BufferText;
	String m_Title;
	unsigned int m_NextPos;
	int keys;
};

class CRadioMenu : public CBaseMenu
{
public:
	CRadioMenu();
public:
	bool SetExtOption(MenuOption option, const void *valuePtr);
	IMenuDisplay *CreateDisplay();
	bool Display(int client, IMenuHandler *handler, unsigned int time);
	void Cancel_Finally();
};

extern CRadioStyle g_RadioMenuStyle;

#endif //_INCLUDE_MENUSTYLE_RADIO_H
