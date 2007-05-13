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

#include "MenuStyle_Radio.h"
#include "sm_stringutil.h"
#include "UserMessages.h"
#include "GameConfigs.h"

extern const char *g_RadioNumTable[];
CRadioStyle g_RadioMenuStyle;
int g_ShowMenuId = -1;

CRadioStyle::CRadioStyle() : m_players(new CBaseMenuPlayer[256+1])
{
}

void CRadioStyle::OnSourceModAllInitialized()
{
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

	g_Menus.AddStyle(this);
	g_Menus.SetDefaultStyle(this);

	g_UserMsgs.HookUserMessage(g_ShowMenuId, this, false);
}

void CRadioStyle::OnSourceModShutdown()
{
	g_UserMsgs.UnhookUserMessage(g_ShowMenuId, this, false);
}

bool CRadioStyle::IsSupported()
{
	return (g_ShowMenuId != -1);
}

bool CRadioStyle::OnClientCommand(int client)
{
	const char *cmd = engine->Cmd_Argv(0);

	if (strcmp(cmd, "menuselect") == 0)
	{
		if (!m_players[client].bInMenu)
		{
			return false;
		}
		int arg = atoi(engine->Cmd_Argv(1));
		ClientPressedKey(client, arg);
	}

	return false;
}

static unsigned int g_last_holdtime = 0;
static unsigned int g_last_client_count = 0;
static int g_last_clients[256];

void CRadioStyle::OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
{
	int count = pFilter->GetRecipientCount();
	bf_read br(bf->GetBasePointer(), 2);

	br.ReadWord();
	int c = br.ReadChar();

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
		if (m_players[client].bInMenu)
		{
			_CancelClientMenu(client, true);
		}
		m_players[client].bInExternMenu = true;
		m_players[client].menuHoldTime = g_last_holdtime;
	}
	g_last_client_count = 0;
}

void CRadioStyle::SendDisplay(int client, IMenuDisplay *display)
{
	CRadioDisplay *rDisplay = (CRadioDisplay *)display;
	rDisplay->SendRawDisplay(client, m_players[client].menuHoldTime);
}

IMenuDisplay *CRadioStyle::CreateDisplay()
{
	return new CRadioDisplay();
}

IBaseMenu *CRadioStyle::CreateMenu()
{
	return new CRadioMenu();
}

unsigned int CRadioStyle::GetMaxPageItems()
{
	return 10;
}

const char *CRadioStyle::GetStyleName()
{
	return "radio";
}

CBaseMenuPlayer *CRadioStyle::GetMenuPlayer(int client)
{
	return &m_players[client];
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
	if (m_NextPos > 10 || !CanDrawItem(item.style))
	{
		return 0;
	}

	if (item.style & ITEMDRAW_RAWLINE)
	{
		if (item.style & ITEMDRAW_SPACER)
		{
			m_BufferText.append("\n");
		} else {
			m_BufferText.append(item.display);
			m_BufferText.append("\n");
		}
		return 0;
	} else if (item.style & ITEMDRAW_SPACER) {
		m_BufferText.append("\n");
		return m_NextPos++;
	}

	if (item.style & ITEMDRAW_DISABLED)
	{
		m_BufferText.append(g_RadioNumTable[m_NextPos]);
		m_BufferText.append(". ");
		m_BufferText.append(item.display);
		m_BufferText.append("\n");
	} else {
		m_BufferText.append("->. ");
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

	return true;
}

void CRadioDisplay::SendRawDisplay(int client, unsigned int time)
{
	char buffer[4096];
	size_t len;

	len = UTIL_Format(buffer, sizeof(buffer), "%s\n%s", m_Title.c_str(), m_BufferText.c_str());

	cell_t players[1] = {client};

	char *ptr = buffer;
	char save = 0;
	while (true)
	{
		if (len > 240)
		{
			save = ptr[240];
			ptr[240] = '\0';
		}
		bf_write *buffer = g_UserMsgs.StartMessage(g_ShowMenuId, players, 1, 0);
		buffer->WriteWord(keys);
		buffer->WriteChar(time ? time : -1);
		buffer->WriteByte( (len > 240) ? 1 : 0 );
		buffer->WriteString(ptr);
		g_UserMsgs.EndMessage();
		if (len > 240)
		{
			ptr[240] = save;
			ptr = &ptr[240];
			len -= 240;
		} else {
			break;
		}
	}
}

void CRadioDisplay::DeleteThis()
{
	delete this;
}

CRadioMenu::CRadioMenu() : CBaseMenu(&g_RadioMenuStyle)
{
}

bool CRadioMenu::SetExtOption(MenuOption option, const void *valuePtr)
{
	return false;
}

IMenuDisplay *CRadioMenu::CreateDisplay()
{
	return new CRadioDisplay(this);
}

bool CRadioMenu::Display(int client, IMenuHandler *handler, unsigned int time)
{
	return g_RadioMenuStyle.DoClientMenu(client, this, handler, time);
}

void CRadioMenu::Cancel_Finally()
{
	g_RadioMenuStyle.CancelMenu(this);
}

const char *g_RadioNumTable[11] = 
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"
};
