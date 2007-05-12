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

#include "sm_stringutil.h"
#include "PlayerManager.h"
#include "MenuStyle_Valve.h"
#include "Translator.h"
#include "PlayerManager.h"
#include "ConCmdManager.h"

SH_DECL_HOOK4_void(IServerPluginHelpers, CreateMessage, SH_NOATTRIB, false, edict_t *, DIALOG_TYPE, KeyValues *, IServerPluginCallbacks *);

ValveMenuStyle g_ValveMenuStyle;
const char *g_OptionNumTable[];
const char *g_OptionCmdTable[];
IServerPluginCallbacks *g_pVSPHandle = NULL;
CallClass<IServerPluginHelpers> *g_pSPHCC = NULL;

class TestHandler : public IMenuHandler
{
public:
	virtual void OnMenuEnd(IBaseMenu *menu)
	{
		menu->Destroy();
	}
};

ValveMenuStyle::ValveMenuStyle() : m_players(new CValveMenuPlayer[256+1]), m_WatchList(256)
{
}

bool ValveMenuStyle::OnClientCommand(int client)
{
	const char *cmd = engine->Cmd_Argv(0);

	if (strcmp(cmd, "sm_vmenuselect") == 0)
	{
		int key_press = atoi(engine->Cmd_Argv(1));
		g_ValveMenuStyle.ClientPressedKey(client, key_press);
		return true;
	}

	if (strcmp(cmd, "sm_test") == 0)
	{
		IBaseMenu *menu = g_ValveMenuStyle.CreateMenu();
		menu->AppendItem("test1", ItemDrawInfo("Test #1", 0));
		menu->AppendItem("test2", ItemDrawInfo("Test #2", 0));
		menu->AppendItem("test3", ItemDrawInfo("Test #3", 0));
		menu->AppendItem("test4", ItemDrawInfo("Test #4", 0));
		menu->AppendItem("test5", ItemDrawInfo("Test #5", 0));
		menu->AppendItem("test6", ItemDrawInfo("Test #6", 0));
		menu->AppendItem("test7", ItemDrawInfo("Test #7", 0));
		menu->AppendItem("test8", ItemDrawInfo("Test #8", 0));
		menu->AppendItem("test9", ItemDrawInfo("Test #9", 0));
		menu->AppendItem("test10", ItemDrawInfo("Test #10", 0));
		menu->AppendItem("test11", ItemDrawInfo("Test #11", 0));
		menu->AppendItem("test12", ItemDrawInfo("Test #12", 0));
		menu->AppendItem("test13", ItemDrawInfo("Test #13", 0));
		menu->AppendItem("test14", ItemDrawInfo("Test #14", 0));
		menu->AppendItem("test15", ItemDrawInfo("Test #15", 0));
		menu->AppendItem("test16", ItemDrawInfo("Test #16", 0));
		menu->AppendItem("test17", ItemDrawInfo("Test #17", 0));
		menu->AppendItem("test18", ItemDrawInfo("Test #18", 0));
		menu->AppendItem("test19", ItemDrawInfo("Test #19", 0));
		menu->AppendItem("test20", ItemDrawInfo("Test #20", 0));

		menu->Display(client, new TestHandler, 20);
		return true;
	} else if (strcmp(cmd, "gaben") == 0) {
		KeyValues *kv = new KeyValues("menu");
		kv->SetString("msg", "hi");
		serverpluginhelpers->CreateMessage(engine->PEntityOfEntIndex(client), DIALOG_MENU, kv, g_pVSPHandle);
		kv->deleteThis();
	}

	return false;
}

void ValveMenuStyle::OnSourceModAllInitialized()
{
	g_Players.AddClientListener(this);
	SH_ADD_HOOK_MEMFUNC(IServerPluginHelpers, CreateMessage, serverpluginhelpers, this, &ValveMenuStyle::HookCreateMessage, false);
	g_pSPHCC = SH_GET_CALLCLASS(serverpluginhelpers);
}

void ValveMenuStyle::OnSourceModShutdown()
{
	SH_RELEASE_CALLCLASS(g_pSPHCC);
	SH_REMOVE_HOOK_MEMFUNC(IServerPluginHelpers, CreateMessage, serverpluginhelpers, this, &ValveMenuStyle::HookCreateMessage, false);
	g_Players.RemoveClientListener(this);
}

void ValveMenuStyle::OnClientDisconnected(int client)
{
	CValveMenuPlayer *player = &m_players[client];
	if (!player->bInMenu)
	{
		return;
	}

	menu_states_t &states = player->states;
	states.mh->OnMenuCancel(states.menu, client, MenuCancel_Disconnect);
	states.mh->OnMenuEnd(states.menu);

	if (player->menuHoldTime)
	{
		m_WatchList.remove(client);
	}

	player->bInMenu = false;
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

	int client = engine->IndexOfEdict(pEdict);
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
		_CancelMenu(client, true);
	}
}

void ValveMenuStyle::OnSourceModVSPReceived(IServerPluginCallbacks *iface)
{
	g_pVSPHandle = iface;
}

IMenuDisplay *ValveMenuStyle::CreateDisplay()
{
	return new CValveMenuDisplay();
}

IBaseMenu *ValveMenuStyle::CreateMenu()
{
	return new CValveMenu();
}

const char *ValveMenuStyle::GetStyleName()
{
	return "valve";
}

unsigned int ValveMenuStyle::GetMaxPageItems()
{
	return 8;
}

static int do_lookup[256];
void ValveMenuStyle::ProcessWatchList()
{
	if (!m_WatchList.size())
	{
		return;
	}

	FastLink<int>::iterator iter;

	unsigned int total = 0;
	for (iter=m_WatchList.begin(); iter!=m_WatchList.end(); ++iter)
	{
		do_lookup[total++] = (*iter);
	}

	int client;
	CValveMenuPlayer *player;
	float curTime = gpGlobals->curtime;
	for (unsigned int i=0; i<total; i++)
	{
		client = do_lookup[i];
		player = &m_players[client];
		if (!player->bInMenu || !player->menuHoldTime)
		{
			m_WatchList.remove(i);
			continue;
		}
		if (curTime > player->menuStartTime + player->menuHoldTime)
		{
			_CancelMenu(i, false);
		}
	}
}

void ValveMenuStyle::_CancelMenu(int client, bool bAutoIgnore)
{
	CValveMenuPlayer *player = &m_players[client];
	menu_states_t &states = player->states;

	bool bOldIgnore = player->bAutoIgnore;
	if (bAutoIgnore)
	{
		player->bAutoIgnore = true;
	}

	/* Save states */
	IMenuHandler *mh = states.mh;
	IBaseMenu *menu = states.menu;

	/* Clear menu */
	player->bInMenu = false;
	if (player->menuHoldTime)
	{
		m_WatchList.remove(client);
	}

	/* Fire callbacks */
	mh->OnMenuCancel(menu, client, MenuCancel_Interrupt);
	mh->OnMenuEnd(menu);

	if (bAutoIgnore)
	{
		player->bAutoIgnore = bOldIgnore;
	}
}

void ValveMenuStyle::CancelMenu(CValveMenu *menu)
{
	int maxClients = g_Players.GetMaxClients();
	for (int i=1; i<=maxClients; i++)
	{
		if (m_players[i].bInMenu)
		{
			menu_states_t &states = m_players[i].states;
			if (states.menu == menu)
			{
				_CancelMenu(i);
			}
		}
	}
}

bool ValveMenuStyle::CancelClientMenu(int client, bool autoIgnore)
{
	if (client < 1 || client > 256 || !m_players[client].bInMenu)
	{
		return false;
	}

	_CancelMenu(client, autoIgnore);

	return true;
}


void ValveMenuStyle::ClientPressedKey(int client, unsigned int key_press)
{
	CValveMenuPlayer *player = &m_players[client];

	/* First question: Are we in a menu? */
	if (!player->bInMenu)
	{
		return;
	}

	bool cancel = false;
	unsigned int item = 0;
	MenuCancelReason reason = MenuCancel_Exit;
	menu_states_t &states = player->states;

	assert(states.mh != NULL);

	if (states.menu == NULL)
	{
		item = key_press;
	} else if (key_press < 1 ||  key_press > 8) {
		cancel = true;
	} else {
		ItemSelection type = states.slots[key_press].type;

		/* For navigational items, we're going to redisplay */
		if (type == ItemSel_Back)
		{
			if (!RedoClientMenu(client, ItemOrder_Descending))
			{
				cancel = true;
				reason = MenuCancel_NoDisplay;
			} else {
				return;
			}
		} else if (type == ItemSel_Next) {
			if (!RedoClientMenu(client, ItemOrder_Ascending))
			{
				cancel = true;						/* I like Saltines. */
				reason = MenuCancel_NoDisplay;
			} else {
				return;
			}
		} else if (type == ItemSel_Exit || type == ItemSel_None) {
			cancel = true;
		} else {
			item = states.slots[key_press].item;
		}
	}

	/* Save variables */
	IMenuHandler *mh = states.mh;
	IBaseMenu *menu = states.menu;

	/* Clear states */
	player->bInMenu = false;
	if (player->menuHoldTime)
	{
		m_WatchList.remove(client);
	}

	if (cancel)
	{
		mh->OnMenuCancel(menu, client, reason);
	} else {
		mh->OnMenuSelect(menu, client, item);
	}

	mh->OnMenuEnd(menu);
}

bool ValveMenuStyle::DoClientMenu(int client, CValveMenuDisplay *menu, IMenuHandler *mh, unsigned int time)
{
	if (!g_pVSPHandle || !mh)
	{
		return false;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer || pPlayer->IsFakeClient() || !pPlayer->IsInGame())
	{
		return false;
	}

	CValveMenuPlayer *player = &m_players[client];
	if (player->bAutoIgnore)
	{
		return false;
	}

	/* For the duration of this, we are going to totally ignore whether
	 * the player is already in a menu or not (except to cancel the old one).
	 * Instead, we are simply going to ignore any further menu displays, so
	 * this display can't be interrupted.
	 */
	player->bAutoIgnore = true;

	/* Cancel any old menus */
	menu_states_t &states = player->states;
	if (player->bInMenu)
	{
		/* We need to cancel the old menu */
		if (player->menuHoldTime)
		{
			m_WatchList.remove(client);
		}
		states.mh->OnMenuCancel(states.menu, client, MenuCancel_Interrupt);
		states.mh->OnMenuEnd(states.menu);
	}

	states.firstItem = 0;
	states.lastItem = 0;
	states.menu = NULL;
	states.mh = mh;
	states.apiVers = SMINTERFACE_MENUMANAGER_VERSION;
	player->curPrioLevel--;
	player->bInMenu = true;
	player->menuStartTime = gpGlobals->curtime;
	player->menuHoldTime = time;

	if (time)
	{
		m_WatchList.push_back(client);
	}

	/* Draw the display */
	menu->SendRawDisplay(client, player->curPrioLevel, time);

	/* We can be interrupted again! */
	player->bAutoIgnore = false;

	return true;
}

bool ValveMenuStyle::DoClientMenu(int client, CValveMenu *menu, IMenuHandler *mh, unsigned int time)
{
	mh->OnMenuStart(menu);

	if (!g_pVSPHandle || !mh)
	{
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu);
		return false;
	}

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer || pPlayer->IsFakeClient() || !pPlayer->IsInGame())
	{
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu);
		return false;
	}

	CValveMenuPlayer *player = &m_players[client];
	if (player->bAutoIgnore)
	{
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu);
		return false;
	}

	/* For the duration of this, we are going to totally ignore whether
	 * the player is already in a menu or not (except to cancel the old one).
	 * Instead, we are simply going to ignore any further menu displays, so
	 * this display can't be interrupted.
	 */
	player->bAutoIgnore = true;

	/* Cancel any old menus */
	menu_states_t &states = player->states;
	if (player->bInMenu)
	{
		/* We need to cancel the old menu */
		if (player->menuHoldTime)
		{
			m_WatchList.remove(client);
		}
		states.mh->OnMenuCancel(states.menu, client, MenuCancel_Interrupt);
		states.mh->OnMenuEnd(states.menu);
	}

	states.firstItem = 0;
	states.lastItem = 0;
	states.menu = menu;
	states.mh = mh;
	states.apiVers = SMINTERFACE_MENUMANAGER_VERSION;

	IMenuDisplay *display = g_Menus.RenderMenu(client, states, ItemOrder_Ascending);
	if (!display)
	{
		player->bAutoIgnore = false;
		player->bInMenu = false;
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu);
		return false;
	}

	/* Finally, set our states */
	player->curPrioLevel--;
	player->bInMenu = true;
	player->menuStartTime = gpGlobals->curtime;
	player->menuHoldTime = time;

	if (time)
	{
		m_WatchList.push_back(client);
	}

	/* Draw the display */
	CValveMenuDisplay *vDisplay = (CValveMenuDisplay *)display;
	vDisplay->SendRawDisplay(client, player->curPrioLevel, time);

	/* Free the display pointer */
	delete display;

	/* We can be interrupted again! */
	player->bAutoIgnore = false;

	return true;
}

bool ValveMenuStyle::RedoClientMenu(int client, ItemOrder order)
{
	CValveMenuPlayer *player = &m_players[client];
	menu_states_t &states = player->states;

	player->bAutoIgnore = true;
	IMenuDisplay *display = g_Menus.RenderMenu(client, states, order);
	if (!display)
	{
		if (player->menuHoldTime)
		{
			m_WatchList.remove(client);
		}
		player->bAutoIgnore = false;
		return false;
	}

	CValveMenuDisplay *vDisplay = (CValveMenuDisplay *)display;
	vDisplay->SendRawDisplay(client, --player->curPrioLevel, player->menuHoldTime);

	delete display;

	player->bAutoIgnore = false;

	return true;
}

MenuSource ValveMenuStyle::GetClientMenu(int client, void **object)
{
	if (client < 1 || client > 256 || !m_players[client].bInMenu)
	{
		return MenuSource_None;
	}

	IBaseMenu *menu;
	if ((menu=m_players[client].states.menu) != NULL)
	{
		if (object)
		{
			*object = menu;
		}
		return MenuSource_BaseMenu;
	}

	return MenuSource_Display;
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
	UTIL_Format(buffer, sizeof(buffer), "%d. %s", m_NextPos, item.display);

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
	m_pKv->SetInt("time", time);

	SH_CALL(g_pSPHCC, &IServerPluginHelpers::CreateMessage)(
		engine->PEntityOfEntIndex(client),
		DIALOG_MENU,
		m_pKv,
		g_pVSPHandle);
}

bool CValveMenuDisplay::SendDisplay(int client, IMenuHandler *handler, unsigned int time)
{
	return g_ValveMenuStyle.DoClientMenu(client, this, handler, time);
}

CValveMenu::CValveMenu() : CBaseMenu(&g_ValveMenuStyle), 
	m_IntroColor(255, 0, 0, 255), m_bShouldDelete(false), m_bCancelling(false)
{
	strcpy(m_IntroMsg, "You have a menu, press ESC");
	m_Pagination = 5;
}

void CValveMenu::Cancel()
{
	if (m_bCancelling)
	{
		return;
	}

	m_bCancelling = true;
	g_ValveMenuStyle.CancelMenu(this);
	m_bCancelling = false;

	if (m_bShouldDelete)
	{
		delete this;
	}
}

void CValveMenu::Destroy()
{
	if (!m_bCancelling || m_bShouldDelete)
	{
		Cancel();
		delete this;
	} else {
		m_bShouldDelete = true;
	}
}

bool CValveMenu::SetPagination(unsigned int itemsPerPage)
{
	if (itemsPerPage < 1 || itemsPerPage > 5)
	{
		return false;
	}

	CBaseMenu::SetPagination(itemsPerPage);

	return true;
}

bool CValveMenu::SetExtOption(MenuOption option, const void *valuePtr)
{
	if (option == MenuOption_IntroMessage)
	{
		strncopy(m_IntroMsg, (const char *)valuePtr, sizeof(m_IntroMsg));
		return true;
	} else if (option == MenuOption_IntroColor) {
		unsigned int *array = (unsigned int *)valuePtr;
		m_IntroColor = Color(array[0], array[1], array[2], array[3]);
		return true;
	}

	return false;
}

bool CValveMenu::Display(int client, IMenuHandler *handler, unsigned int time)
{
	if (m_bCancelling)
	{
		return false;
	}

	return g_ValveMenuStyle.DoClientMenu(client, this, handler, time);
}

IMenuDisplay *CValveMenu::CreateDisplay()
{
	return new CValveMenuDisplay(this);
}

bool CValveMenu::GetExitButton()
{
	return true;
}

bool CValveMenu::SetExitButton(bool set)
{
	return false;
}

static const char *g_OptionNumTable[11] = 
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
};

static const char *g_OptionCmdTable[11] = 
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
