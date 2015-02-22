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

#ifndef _INCLUDE_MENUSTYLE_RADIO_H
#define _INCLUDE_MENUSTYLE_RADIO_H

#include "sm_globals.h"
#include "MenuManager.h"
#include "MenuStyle_Base.h"
#include "sourcemm_api.h"
#include <IPlayerHelpers.h>
#include "UserMessages.h"
#include "sm_fastlink.h"
#include <sh_stack.h>
#include <compat_wrappers.h>
#include "logic/common_logic.h"
#include "AutoHandleRooter.h"

using namespace SourceMod;

class CRadioDisplay;
class CRadioMenu;

class CRadioMenuPlayer : public CBaseMenuPlayer
{
public:
	void Radio_Init(int keys, const char *title, const char *buffer);
	bool Radio_NeedsRefresh();
	void Radio_Refresh();
	void Radio_SetIndex(unsigned int index);
private:
	unsigned int m_index;
	size_t display_len;
	char display_pkt[512];
	int display_keys;
	float display_last_refresh;
};

class CRadioStyle : 
	public BaseMenuStyle,
	public SMGlobalClass,
#ifdef USE_PROTOBUF_USERMESSAGES
	public IProtobufUserMessageListener
#else
	public IBitBufUserMessageListener
#endif
{
public:
	CRadioStyle();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModShutdown();
public: //BaseMenuStyle
	CBaseMenuPlayer *GetMenuPlayer(int client);
	void SendDisplay(int client, IMenuPanel *display);
	void ProcessWatchList();
public: //IMenuStyle
	const char *GetStyleName();
	IMenuPanel *CreatePanel();
	IBaseMenu *CreateMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner);
	unsigned int GetMaxPageItems();
	unsigned int GetApproxMemUsage();
public: //IUserMessageListener
#ifdef USE_PROTOBUF_USERMESSAGES
	void OnUserMessage(int msg_id, protobuf::Message &msg, IRecipientFilter *pFilter);
#else
	void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter);
#endif
	void OnUserMessageSent(int msg_id);
public:
	bool IsSupported();
	bool OnClientCommand(int client, const char *cmdname, const CCommand &cmd);
public:
	CRadioDisplay *MakeRadioDisplay(CRadioMenu *menu=NULL);
	void FreeRadioDisplay(CRadioDisplay *display);
	CRadioMenuPlayer *GetRadioMenuPlayer(int client);
private:
	CRadioMenuPlayer *m_players;
	CStack<CRadioDisplay *> m_FreeDisplays;
};

class CRadioDisplay : public IMenuPanel
{
	friend class CRadioStyle;
public:
	CRadioDisplay();
	CRadioDisplay(CRadioMenu *menu);
public: //IMenuPanel
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
	bool SetSelectableKeys(unsigned int keymap);
	unsigned int GetCurrentKey();
	bool SetCurrentKey(unsigned int key);
	int GetAmountRemaining();
	unsigned int GetApproxMemUsage();
	bool DirectSet(const char *str);
private:
	String m_BufferText;
	String m_Title;
	unsigned int m_NextPos;
	int keys;
};

class CRadioMenu : public CBaseMenu
{
public:
	CRadioMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner);
public:
	bool SetExtOption(MenuOption option, const void *valuePtr);
	IMenuPanel *CreatePanel();
	bool Display(int client, unsigned int time, IMenuHandler *alt_handler=NULL);
	bool DisplayAtItem(int client,
		unsigned int time,
		unsigned int start_item,
		IMenuHandler *alt_handler/* =NULL */);
	bool SetPagination(unsigned int itemsPerPage);
	void Cancel_Finally();
	unsigned int GetApproxMemUsage();
};

extern CRadioStyle g_RadioMenuStyle;

#endif //_INCLUDE_MENUSTYLE_RADIO_H
