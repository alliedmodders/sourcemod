/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "MenuStyle_Valve.h"
#include "PlayerManager.h"
#include "ConCmdManager.h"

SH_DECL_HOOK4_void(IServerPluginHelpers, CreateMessage, SH_NOATTRIB, false, edict_t *, DIALOG_TYPE, KeyValues *, IServerPluginCallbacks *);

ValveMenuStyle g_ValveMenuStyle;
extern const char *g_OptionNumTable[];
extern const char *g_OptionCmdTable[];
CallClass<IServerPluginHelpers> *g_pSPHCC = NULL;

ValveMenuStyle::ValveMenuStyle() : m_players(new CValveMenuPlayer[256+1])
{
}

CBaseMenuPlayer *ValveMenuStyle::GetMenuPlayer(int client)
{
	return &m_players[client];
}

bool ValveMenuStyle::OnClientCommand(int client, const char *cmdname, const CCommand &cmd)
{
	if (strcmp(cmdname, "sm_vmenuselect") == 0)
	{
		int key_press = atoi(cmd.Arg(1));
		g_ValveMenuStyle.ClientPressedKey(client, key_press);
		return true;
	}

	return false;
}

void ValveMenuStyle::OnSourceModAllInitialized()
{
	g_Players.AddClientListener(this);
	SH_ADD_HOOK(IServerPluginHelpers, CreateMessage, serverpluginhelpers, SH_MEMBER(this, &ValveMenuStyle::HookCreateMessage), false);
	g_pSPHCC = SH_GET_CALLCLASS(serverpluginhelpers);
}

void ValveMenuStyle::OnSourceModShutdown()
{
	SH_RELEASE_CALLCLASS(g_pSPHCC);
	SH_REMOVE_HOOK(IServerPluginHelpers, CreateMessage, serverpluginhelpers, SH_MEMBER(this, &ValveMenuStyle::HookCreateMessage), false);
	g_Players.RemoveClientListener(this);
}

void ValveMenuStyle::HookCreateMessage(edict_t *pEdict,
									   DIALOG_TYPE type,
									   KeyValues *kv,
									   IServerPluginCallbacks *plugin)
{
	if (type != DIALOG_MENU)
	{
		return;
	}

	int client = IndexOfEdict(pEdict);
	if (client < 1 || client > 256)
	{
		return;
	}

	CValveMenuPlayer *player = &m_players[client];
	
	/* We don't care if the player is in a menu because, for all intents and purposes,
	 * the menu is completely private.  Instead, we just figure out the level we'll need
	 * in order to override it.
	 */
	player->curPrioLevel = kv->GetInt("level", player->curPrioLevel);

	/* Oh no! What happens if we got a menu that overwrites ours?! */
	if (player->bInMenu)
	{
		/* Okay, let the external menu survive for now.  It may live another 
		 * day to avenge its grandfather, killed in the great Menu Interruption
		 * battle.
		 */
		_CancelClientMenu(client, MenuCancel_Interrupted, true);
	}
}

IMenuPanel *ValveMenuStyle::CreatePanel()
{
	return new CValveMenuDisplay();
}

IBaseMenu *ValveMenuStyle::CreateMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner)
{
	return new CValveMenu(pHandler, pOwner);
}

const char *ValveMenuStyle::GetStyleName()
{
	return "valve";
}

unsigned int ValveMenuStyle::GetMaxPageItems()
{
	return 8;
}

void ValveMenuStyle::SendDisplay(int client, IMenuPanel *display)
{
	m_players[client].curPrioLevel--;
	CValveMenuDisplay *vDisplay = (CValveMenuDisplay *)display;
	vDisplay->SendRawDisplay(client, m_players[client].curPrioLevel, m_players[client].menuHoldTime);
}

bool ValveMenuStyle::DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time)
{
	if (vsp_interface == NULL)
	{
		return false;
	}

	return BaseMenuStyle::DoClientMenu(client, menu, mh, time);
}

bool ValveMenuStyle::DoClientMenu(int client, CBaseMenu *menu, unsigned int first_item, IMenuHandler *mh, unsigned int time)
{
	if (vsp_interface == NULL)
	{
		mh->OnMenuStart(menu);
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu, MenuEnd_Cancelled);
		return false;
	}

	return BaseMenuStyle::DoClientMenu(client, menu, first_item, mh, time);
}

unsigned int ValveMenuStyle::GetApproxMemUsage()
{
	return sizeof(ValveMenuStyle) + (sizeof(CValveMenuPlayer) * 257);
}

CValveMenuDisplay::CValveMenuDisplay()
{
	m_pKv = NULL;
	Reset();
}

CValveMenuDisplay::CValveMenuDisplay(CValveMenu *pMenu)
{
	m_pKv = NULL;
	Reset();
	m_pKv->SetColor("color", pMenu->m_IntroColor);
	m_pKv->SetString("title", pMenu->m_IntroMsg);
}

void CValveMenuDisplay::DeleteThis()
{
	delete this;
}

CValveMenuDisplay::~CValveMenuDisplay()
{
	m_pKv->deleteThis();
}

IMenuStyle *CValveMenuDisplay::GetParentStyle()
{
	return &g_ValveMenuStyle;
}

void CValveMenuDisplay::Reset()
{
	if (m_pKv)
	{
		m_pKv->deleteThis();
	}
	m_pKv = new KeyValues("menu");
	m_NextPos = 1;
	m_TitleDrawn = false;
}

unsigned int CValveMenuDisplay::GetCurrentKey()
{
	return m_NextPos;
}

bool CValveMenuDisplay::SetCurrentKey(unsigned int key)
{
	if (key < m_NextPos || key > 9)
	{
		return false;
	}

	m_NextPos = key;

	return true;
}

bool CValveMenuDisplay::SetExtOption(MenuOption option, const void *valuePtr)
{
	if (option == MenuOption_IntroMessage)
	{
		m_pKv->SetString("title", (const char *)valuePtr);
		return true;
	} else if (option == MenuOption_IntroColor) {
		int *array = (int *)valuePtr;
		m_pKv->SetColor("color", Color(array[0], array[1], array[2], array[3]));
		return true;
	} else if (option == MenuOption_Priority) {
		m_pKv->SetInt("level", *(int *)valuePtr);
		return true;
	}

	return false;
}

bool CValveMenuDisplay::CanDrawItem(unsigned int drawFlags)
{
	/**
	 * ITEMDRAW_RAWLINE - We can't draw random text, and this doesn't add a slot,
	 *  so it's completely safe to ignore it.
	 * -----------------------------------------
	 */
	if (drawFlags & ITEMDRAW_RAWLINE)
	{
		return false;
	}

	/**
	 * Special cases, explained in DrawItem()
	 */
	if ((drawFlags & ITEMDRAW_NOTEXT)
		|| (drawFlags & ITEMDRAW_SPACER))
	{
		return true;
	}

	/**
	 * We can't draw disabled text.  We could bump the position, but we
	 * want DirectDraw() to find some actual items to display.
	 */
	if (drawFlags & ITEMDRAW_DISABLED)
	{
		return false;
	}

	return true;
}

unsigned int CValveMenuDisplay::DrawItem(const ItemDrawInfo &item)
{
	if (m_NextPos > 9 || !CanDrawItem(item.style))
	{
		return 0;
	}

	/**
	 * For these cases we can't draw anything at all, but
	 * we can at least bump the position since we were explicitly asked to.
	 */
	if ((item.style & ITEMDRAW_NOTEXT)
		|| (item.style & ITEMDRAW_SPACER))
	{
		return m_NextPos++;
	}

	char buffer[255];
	ke::SafeSprintf(buffer, sizeof(buffer), "%d. %s", m_NextPos, item.display);

	KeyValues *ki = m_pKv->FindKey(g_OptionNumTable[m_NextPos], true);
	ki->SetString("command", g_OptionCmdTable[m_NextPos]);
	ki->SetString("msg", buffer);

	return m_NextPos++;
}

void CValveMenuDisplay::DrawTitle(const char *text, bool onlyIfEmpty/* =false */)
{
	if (onlyIfEmpty && m_TitleDrawn)
	{
		return;
	}

	m_pKv->SetString("msg", text);
	m_TitleDrawn = true;
}

bool CValveMenuDisplay::DrawRawLine(const char *rawline)
{
	return false;
}

void CValveMenuDisplay::SendRawDisplay(int client, int priority, unsigned int time)
{
	m_pKv->SetInt("level", priority);
	m_pKv->SetInt("time", time ? time : 200);

	SH_CALL(g_pSPHCC, &IServerPluginHelpers::CreateMessage)(
		PEntityOfEntIndex(client),
		DIALOG_MENU,
		m_pKv,
		vsp_interface);
}

bool CValveMenuDisplay::SendDisplay(int client, IMenuHandler *handler, unsigned int time)
{
	return g_ValveMenuStyle.DoClientMenu(client, this, handler, time);
}

bool CValveMenuDisplay::SetSelectableKeys(unsigned int keymap)
{
	return false;
}

int CValveMenuDisplay::GetAmountRemaining()
{
	/* :TODO: this is a lie, but nothing really seems meaningful... */
	return -1;
}

unsigned int CValveMenuDisplay::GetApproxMemUsage()
{
	return sizeof(CValveMenuDisplay) + (sizeof(KeyValues) * m_NextPos * 10);
}

CValveMenu::CValveMenu(IMenuHandler *pHandler, IdentityToken_t *pOwner) : 
CBaseMenu(pHandler, &g_ValveMenuStyle, pOwner), 
	m_IntroColor(255, 0, 0, 255)
{
	strcpy(m_IntroMsg, "You have a menu, press ESC");
	m_Pagination = 5;
}

void CValveMenu::Cancel_Finally()
{
	g_ValveMenuStyle.CancelMenu(this);
}

bool CValveMenu::SetPagination(unsigned int itemsPerPage)
{
	if (itemsPerPage > 5)
	{
		return false;
	}

	return CBaseMenu::SetPagination(itemsPerPage);
}

bool CValveMenu::SetExtOption(MenuOption option, const void *valuePtr)
{
	if (option == MenuOption_IntroMessage)
	{
		ke::SafeStrcpy(m_IntroMsg, sizeof(m_IntroMsg), (const char *)valuePtr);
		return true;
	} else if (option == MenuOption_IntroColor) {
		unsigned int *array = (unsigned int *)valuePtr;
		m_IntroColor = Color(array[0], array[1], array[2], array[3]);
		return true;
	}

	return false;
}

bool CValveMenu::Display(int client, unsigned int time, IMenuHandler *alt_handler)
{
	return DisplayAtItem(client, time, 0, alt_handler);
}

bool CValveMenu::DisplayAtItem(int client,
							   unsigned int time,
							   unsigned int start_item,
							   IMenuHandler *alt_handler/* =NULL */)
{
	if (m_bCancelling)
	{
		return false;
	}

	AutoHandleRooter ahr(this->GetHandle());
	return g_ValveMenuStyle.DoClientMenu(client, this, start_item, alt_handler ? alt_handler : m_pHandler, time);
}

IMenuPanel *CValveMenu::CreatePanel()
{
	return new CValveMenuDisplay(this);
}

void CValveMenu::SetMenuOptionFlags(unsigned int flags)
{
	flags |= MENUFLAG_BUTTON_EXIT;
	CBaseMenu::SetMenuOptionFlags(flags);
}

unsigned int CValveMenu::GetApproxMemUsage()
{
	return sizeof(CValveMenu) + GetBaseMemUsage();
}

const char *g_OptionNumTable[] = 
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
};

const char *g_OptionCmdTable[] = 
{
	"sm_vmenuselect 0", /* INVALID! */
	"sm_vmenuselect 1",
	"sm_vmenuselect 2",
	"sm_vmenuselect 3",
	"sm_vmenuselect 4",
	"sm_vmenuselect 5",
	"sm_vmenuselect 6",
	"sm_vmenuselect 7",
	"sm_vmenuselect 8",
	"sm_vmenuselect 9",
	"sm_vmenuselect 10"
};
