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

#include "MenuStyle_Radio.h"
#include "sm_stringutil.h"
#include "UserMessages.h"
#include <IGameConfigs.h>
#include "PlayerManager.h"
#if defined MENU_DEBUG
#include <bridge/include/ILogger.h>
#endif
#include "logic_bridge.h"

#ifdef USE_PROTOBUF_USERMESSAGES
#include <google/protobuf/descriptor.h>
#endif

#if SOURCE_ENGINE == SE_CSGO
#include <game/shared/csgo/protobuf/cstrike15_usermessages.pb.h>
#elif SOURCE_ENGINE == SE_BLADE
#include <game/shared/berimbau/protobuf/berimbau_usermessages.pb.h>
#endif

extern const char *g_RadioNumTable[];
CRadioStyle g_RadioMenuStyle;
int g_ShowMenuId = -1;
bool g_bRadioInit = false;
unsigned int g_RadioMenuTimeout = 0;

// back, next, exit
#define MAX_PAGINATION_OPTIONS 3

#define MAX_MENUSLOT_KEYS 10

static unsigned int s_RadioMaxPageItems = MAX_MENUSLOT_KEYS;
static bool s_RadioClosesOnInvalidSlot = false;

CRadioStyle::CRadioStyle()
{
	m_players = new CRadioMenuPlayer[256+1];
	for (size_t i = 0; i < 256+1; i++)
	{
		m_players[i].Radio_SetIndex(i);
	}
}

void CRadioStyle::OnSourceModAllInitialized()
{
	g_Players.AddClientListener(this);
}

void CRadioStyle::OnSourceModLevelChange(const char *mapName)
{
	if (g_bRadioInit)
	{
		return;
	}

	g_bRadioInit = true;

	// Always register the style. Use IsSupported() to check for validity before use.
	g_Menus.AddStyle(this);

	const char *msg = g_pGameConf->GetKeyValue("HudRadioMenuMsg");
	if (!msg || msg[0] == '\0')
	{
		return;
	}

	g_ShowMenuId = g_UserMsgs.GetMessageIndex(msg);

	if (!IsSupported())
	{
		return;
	}

	const char *val = g_pGameConf->GetKeyValue("RadioMenuTimeout");
	if (val != NULL)
	{
		g_RadioMenuTimeout = atoi(val);
	}
	else
	{
		g_RadioMenuTimeout = 0;
	}

	const char *items = g_pGameConf->GetKeyValue("RadioMenuMaxPageItems");
	if (items != NULL)
	{
		int value = atoi(items);

		// Only override the mostly-safe default if it's a sane value
		if (value > MAX_PAGINATION_OPTIONS && value <= MAX_MENUSLOT_KEYS)
		{
			s_RadioMaxPageItems = value;
		}
	}

	const char *closes = g_pGameConf->GetKeyValue("RadioMenuClosesOnInvalidSlot");
	if (closes != nullptr && strcmp(closes, "yes") == 0)
	{
		s_RadioClosesOnInvalidSlot = true;
	}

	g_Menus.SetDefaultStyle(this);

	g_UserMsgs.HookUserMessage(g_ShowMenuId, this, false);
}

void CRadioStyle::OnSourceModShutdown()
{
	g_Players.RemoveClientListener(this);
	g_UserMsgs.UnhookUserMessage(g_ShowMenuId, this, false);

	while (!m_FreeDisplays.empty())
	{
		delete m_FreeDisplays.front();
		m_FreeDisplays.pop();
	}
}

bool CRadioStyle::IsSupported()
{
	return (g_ShowMenuId != -1);
}

bool CRadioStyle::OnClientCommand(int client, const char *cmdname, const CCommand &cmd)
{
	if (strcmp(cmdname, "menuselect") == 0)
	{
		if (!m_players[client].bInMenu)
		{
			m_players[client].bInExternMenu = false;
			return false;
		}

		int arg = atoi(cmd.Arg(1));
		ClientPressedKey(client, arg);
		return true;
	}

	return false;
}

static unsigned int g_last_holdtime = 0;
static unsigned int g_last_client_count = 0;
static int g_last_clients[256];

#ifdef USE_PROTOBUF_USERMESSAGES
void CRadioStyle::OnUserMessage(int msg_id, protobuf::Message &msg, IRecipientFilter *pFilter)
#else
void CRadioStyle::OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
#endif
{
	int count = pFilter->GetRecipientCount();

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	int c = ((CCSUsrMsg_ShowMenu &)msg).display_time();
#else
	bf_read br(bf->GetBasePointer(), 3);

	br.ReadWord();
	int c = br.ReadChar();
#endif

	g_last_holdtime = (c == -1) ? 0 : (unsigned)c;

	for (int i=0; i<count; i++)
	{
		g_last_clients[g_last_client_count++] = pFilter->GetRecipientIndex(i);
	}
}

void CRadioStyle::OnUserMessageSent(int msg_id)
{
	for (unsigned int i=0; i<g_last_client_count; i++)
	{
		int client = g_last_clients[i];
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] CRadioStyle got ShowMenu (client %d) (bInMenu %d)",
			client,
			m_players[client].bInExternMenu);
#endif
		if (m_players[client].bInMenu)
		{
			_CancelClientMenu(client, MenuCancel_Interrupted, true);
		}
		m_players[client].bInExternMenu = true;
		m_players[client].menuHoldTime = g_last_holdtime;
	}
	g_last_client_count = 0;
}

void CRadioStyle::SendDisplay(int client, IMenuPanel *display)
{
	CRadioDisplay *rDisplay = (CRadioDisplay *)display;
	rDisplay->SendRawDisplay(client, m_players[client].menuHoldTime);
}

IMenuPanel *CRadioStyle::CreatePanel()
{
	return g_RadioMenuStyle.MakeRadioDisplay();
}

IBaseMenu *CRadioStyle::CreateMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner)
{
	return new CRadioMenu(pHandler, pOwner);
}

unsigned int CRadioStyle::GetMaxPageItems()
{
	return s_RadioMaxPageItems;
}

const char *CRadioStyle::GetStyleName()
{
	return "radio";
}

CBaseMenuPlayer *CRadioStyle::GetMenuPlayer(int client)
{
	return &m_players[client];
}

CRadioDisplay *CRadioStyle::MakeRadioDisplay(CRadioMenu *menu)
{
	CRadioDisplay *display;
	if (m_FreeDisplays.empty())
	{
		display = new CRadioDisplay();
	}
	else
	{
		display = m_FreeDisplays.front();
		m_FreeDisplays.pop();
		display->Reset();
	}
	return display;
}

void CRadioStyle::FreeRadioDisplay(CRadioDisplay *display)
{
	m_FreeDisplays.push(display);
}

CRadioMenuPlayer *CRadioStyle::GetRadioMenuPlayer(int client)
{
	return &m_players[client];
}

void CRadioStyle::ProcessWatchList()
{
	if (!g_RadioMenuTimeout)
	{
		BaseMenuStyle::ProcessWatchList();
		return;
	}

	BaseMenuStyle::ProcessWatchList();

	CRadioMenuPlayer *pPlayer;
	unsigned int max_clients = g_Players.GetMaxClients();
	for (unsigned int i = 1; i <= max_clients; i++)
	{
		pPlayer = GetRadioMenuPlayer(i);
		if (!pPlayer->bInMenu || pPlayer->bInExternMenu)
		{
			continue;
		}
		if (pPlayer->Radio_NeedsRefresh())
		{
			pPlayer->Radio_Refresh();
		}
	}
}

unsigned int CRadioStyle::GetApproxMemUsage()
{
	return sizeof(CRadioStyle) + (sizeof(CRadioMenuPlayer) * 257);
}

CRadioDisplay::CRadioDisplay()
{
	Reset();
}

CRadioDisplay::CRadioDisplay(CRadioMenu *menu)
{
	Reset();
}

bool CRadioDisplay::DrawRawLine(const char *rawline)
{
	m_BufferText.append(rawline);
	m_BufferText.append("\n");
	return true;
}

void CRadioDisplay::Reset()
{
	m_BufferText.assign("");
	m_Title.assign("");
	m_NextPos = 1;
	keys = 0;
}

bool CRadioDisplay::DirectSet(const char *str)
{
	m_Title.clear();
	m_BufferText.assign(str);
	
	return true;
}

unsigned int CRadioDisplay::GetCurrentKey()
{
	return m_NextPos;
}

bool CRadioDisplay::SetCurrentKey(unsigned int key)
{
	if (key < m_NextPos || m_NextPos > s_RadioMaxPageItems)
	{
		return false;
	}

	m_NextPos = key;

	return true;
}

bool CRadioDisplay::SendDisplay(int client, IMenuHandler *handler, unsigned int time)
{
	return g_RadioMenuStyle.DoClientMenu(client, this, handler, time);
}

bool CRadioDisplay::SetExtOption(MenuOption option, const void *valuePtr)
{
	return false;
}

IMenuStyle *CRadioDisplay::GetParentStyle()
{
	return &g_RadioMenuStyle;
}

void CRadioDisplay::DrawTitle(const char *text, bool onlyIfEmpty/* =false */)
{
	if (m_Title.size() != 0 && onlyIfEmpty)
	{
		return;
	}
	m_Title.assign(text);
}

unsigned int CRadioDisplay::DrawItem(const ItemDrawInfo &item)
{
	if (m_NextPos > s_RadioMaxPageItems || !CanDrawItem(item.style))
	{
		return 0;
	}

	if (item.style & ITEMDRAW_RAWLINE)
	{
		if (item.style & ITEMDRAW_SPACER)
		{
			m_BufferText.append(" \n");
		} else {
			m_BufferText.append(item.display);
			m_BufferText.append("\n");
		}
		return 0;
	} else if (item.style & ITEMDRAW_SPACER) {
		m_BufferText.append(" \n");
		return m_NextPos++;
	} else if (item.style & ITEMDRAW_NOTEXT) {
		return m_NextPos++;
	}

	if (item.style & ITEMDRAW_DISABLED)
	{
		m_BufferText.append(g_RadioNumTable[m_NextPos]);
		m_BufferText.append(item.display);
		m_BufferText.append("\n");
	} else {
		m_BufferText.append("->");
		m_BufferText.append(g_RadioNumTable[m_NextPos]);
		m_BufferText.append(item.display);
		m_BufferText.append("\n");
		keys |= (1<<(m_NextPos-1));
	}

	return m_NextPos++;
}

bool CRadioDisplay::CanDrawItem(unsigned int drawFlags)
{
	if ((drawFlags & ITEMDRAW_IGNORE) == ITEMDRAW_IGNORE)
	{
		return false;
	}

	if ((drawFlags & ITEMDRAW_DISABLED) && (drawFlags & ITEMDRAW_CONTROL))
	{
		return false;
	}

	return true;
}

void CRadioDisplay::SendRawDisplay(int client, unsigned int time)
{
	int _sel_keys = (keys == 0) ? (1<<9) : keys;
	CRadioMenuPlayer *pPlayer = g_RadioMenuStyle.GetRadioMenuPlayer(client);
	pPlayer->Radio_Init(_sel_keys, m_Title.c_str(), m_BufferText.c_str());
	pPlayer->Radio_Refresh();
}

void CRadioMenuPlayer::Radio_SetIndex(unsigned int index)
{
	m_index = index;
}

bool CRadioMenuPlayer::Radio_NeedsRefresh()
{
	return (gpGlobals->curtime - display_last_refresh >= g_RadioMenuTimeout);
}

void CRadioMenuPlayer::Radio_Init(int keys, const char *title, const char *text)
{
	if (title[0] != '\0')
	{
		display_len = ke::SafeSprintf(display_pkt, 
			sizeof(display_pkt), 
			"%s\n%s", 
			title,
			text);
	}
	else
	{
		display_len = ke::SafeStrcpy(display_pkt, 
			sizeof(display_pkt), 
			text);
	}

	// Some games have implemented CHudMenu::SelectMenuItem to close the menu
	// even if an invalid slot has been selected, which causes us a problem as
	// we'll never get any notification from the client and we'll keep the menu
	// alive on our end indefinitely. For these games, pretend that every slot
	// is valid for selection so we're guaranteed to get a menuselect command.
	// We don't want to do this for every game as the common SelectMenuItem
	// implementation ignores invalid selections and keeps the menu open, which
	// is a much nicer user experience.
	display_keys = s_RadioClosesOnInvalidSlot ? 0x7ff : keys;
}

void CRadioMenuPlayer::Radio_Refresh()
{
	cell_t players[1] = { (cell_t)m_index };
	char *ptr = display_pkt;
	char save = 0;
	size_t len = display_len;
	unsigned int time;

	/* Compute the new time */
	if (menuHoldTime == 0)
	{
		time = 0;
	}
	else
	{
		time = menuHoldTime - (unsigned int)(gpGlobals->curtime - menuStartTime);
	}

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
	// TODO: find what happens past 240 on CS:GO
	CCSUsrMsg_ShowMenu *msg = (CCSUsrMsg_ShowMenu *)g_UserMsgs.StartProtobufMessage(g_ShowMenuId, players, 1, USERMSG_BLOCKHOOKS);
	msg->set_bits_valid_slots(display_keys);
	msg->set_display_time(time);
	msg->set_menu_string(ptr);
	g_UserMsgs.EndMessage();
#else
	while (true)
	{
		if (len > 240)
		{
			save = ptr[240];
			ptr[240] = '\0';
		}

		bf_write *buffer = g_UserMsgs.StartBitBufMessage(g_ShowMenuId, players, 1, USERMSG_BLOCKHOOKS);
		buffer->WriteWord(display_keys);
		buffer->WriteChar(time ? time : -1);
		buffer->WriteByte( (len > 240) ? 1 : 0 );
		buffer->WriteString(ptr);
		g_UserMsgs.EndMessage();
		if (len > 240)
		{
			ptr[240] = save;
			ptr = &ptr[240];
			len -= 240;
		}
		else
		{
			break;
		}
	}
#endif

	display_last_refresh = gpGlobals->curtime;
}

int CRadioDisplay::GetAmountRemaining()
{
	size_t amt = m_Title.size() + 1 + m_BufferText.size();
	if (amt >= 511)
	{
		return 0;
	}
	return (int)(511 - amt);
}

void CRadioDisplay::DeleteThis()
{
	delete this;
}

bool CRadioDisplay::SetSelectableKeys(unsigned int keymap)
{
	keys = (signed)keymap;
	return true;
}

unsigned int CRadioDisplay::GetApproxMemUsage()
{
	return sizeof(CRadioDisplay)
		+ m_BufferText.size()
		+ m_Title.size();
}

CRadioMenu::CRadioMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner) : 
CBaseMenu(pHandler, &g_RadioMenuStyle, pOwner)
{
	m_Pagination = s_RadioMaxPageItems - MAX_PAGINATION_OPTIONS;
}

bool CRadioMenu::SetExtOption(MenuOption option, const void *valuePtr)
{
	return false;
}

IMenuPanel *CRadioMenu::CreatePanel()
{
	return g_RadioMenuStyle.MakeRadioDisplay(this);
}

bool CRadioMenu::Display(int client, unsigned int time, IMenuHandler *alt_handler)
{
	return DisplayAtItem(client, time, 0, alt_handler);
}

bool CRadioMenu::DisplayAtItem(int client,
							   unsigned int time,
							   unsigned int start_item,
							   IMenuHandler *alt_handler)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] CRadioMenu::Display(%p) (client %d) (time %d)",
		this,
		client,
		time);
#endif
	if (m_bCancelling)
	{
		return false;
	}

	AutoHandleRooter ahr(this->GetHandle());
	return g_RadioMenuStyle.DoClientMenu(client,
		this,
		start_item,
		alt_handler ? alt_handler : m_pHandler, 
		time);
}

bool CRadioMenu::SetPagination(unsigned int itemsPerPage)
{
	const unsigned int maxPerPage = s_RadioMaxPageItems - MAX_PAGINATION_OPTIONS;
	if (itemsPerPage > maxPerPage)
	{
		return false;
	}

	return CBaseMenu::SetPagination(itemsPerPage);
}

void CRadioMenu::Cancel_Finally()
{
	g_RadioMenuStyle.CancelMenu(this);
}

unsigned int CRadioMenu::GetApproxMemUsage()
{
	return sizeof(CRadioMenu) + GetBaseMemUsage();
}

const char *g_RadioNumTable[11] = 
{
	"0. ", "1. ", "2. ", "3. ", "4. ", "5. ", "6. ", "7. ", "8. ", "9. ", "0. "
};
