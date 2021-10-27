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

#include <stdarg.h>
#include "sm_stringutil.h"
#include "MenuStyle_Base.h"
#include "PlayerManager.h"
#include "MenuManager.h"
#include "CellRecipientFilter.h"
#if defined MENU_DEBUG
#include <bridge/include/ILogger.h>
#endif
#include "logic_bridge.h"
#include "AutoHandleRooter.h"

BaseMenuStyle::BaseMenuStyle() : m_WatchList(256), m_hHandle(BAD_HANDLE)
{
}

Handle_t BaseMenuStyle::GetHandle()
{
	/* Don't create the handle until we need it */
	if (m_hHandle == BAD_HANDLE)
	{
		m_hHandle = g_Menus.CreateStyleHandle(this);
	}

	return m_hHandle;
}

void BaseMenuStyle::AddClientToWatch(int client)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] AddClientToWatch(%d)", client);
#endif
	m_WatchList.push_back(client);
}

void BaseMenuStyle::RemoveClientFromWatch(int client)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] RemoveClientFromWatch(%d)", client);
#endif
	m_WatchList.remove(client);
}

void BaseMenuStyle::_CancelClientMenu(int client, MenuCancelReason reason, bool bAutoIgnore/* =false */)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] _CancelClientMenu() (client %d) (bAutoIgnore %d) (reason %d)", client, bAutoIgnore, reason);
#endif
	CBaseMenuPlayer *player = GetMenuPlayer(client);
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
		RemoveClientFromWatch(client);
	}

	/* Fire callbacks */
	mh->OnMenuCancel(menu, client, reason);
	
	/* Only fire end if there's a valid menu */
	if (menu)
	{
		mh->OnMenuEnd(menu, MenuEnd_Cancelled);
	}

	if (bAutoIgnore)
	{
		player->bAutoIgnore = bOldIgnore;
	}
}

void BaseMenuStyle::CancelMenu(CBaseMenu *menu)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] CancelMenu() (menu %p)", menu);
#endif
	int maxClients = g_Players.GetMaxClients();
	for (int i=1; i<=maxClients; i++)
	{
		CBaseMenuPlayer *player = GetMenuPlayer(i);
		if (player->bInMenu)
		{
			menu_states_t &states = player->states;
			if (states.menu == menu)
			{
				_CancelClientMenu(i, MenuCancel_Interrupted);
			}
		}
	}
}

bool BaseMenuStyle::CancelClientMenu(int client, bool autoIgnore)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] CancelClientMenu() (client %d) (bAutoIgnore %d)", client, autoIgnore);
#endif
	if (client < 1 || client > g_Players.MaxClients())
	{
		return false;
	}

	CBaseMenuPlayer *player = GetMenuPlayer(client);
	if (!player->bInMenu)
	{
		return false;
	}

	_CancelClientMenu(client, MenuCancel_Interrupted, autoIgnore);

	return true;
}

MenuSource BaseMenuStyle::GetClientMenu(int client, void **object)
{
	if (client < 1 || client > g_Players.GetMaxClients())
	{
		return MenuSource_None;
	}

	CBaseMenuPlayer *player = GetMenuPlayer(client);

	if (player->bInMenu)
	{
		IBaseMenu *menu;
		if ((menu=player->states.menu) != NULL)
		{
			if (object)
			{
				*object = menu;
			}
			return MenuSource_BaseMenu;
		}

		return MenuSource_Display;
	} else if (player->bInExternMenu) {
		if (player->menuHoldTime != 0
			&& (gpGlobals->curtime > player->menuStartTime + player->menuHoldTime))
		{
			player->bInExternMenu = false;
			return MenuSource_None;
		}
		return MenuSource_External;
	}

	return MenuSource_None;
}

void BaseMenuStyle::OnClientDisconnected(int client)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] OnClientDisconnected(%d)", client);
#endif
	CBaseMenuPlayer *player = GetMenuPlayer(client);
	if (!player->bInMenu)
	{
		return;
	}

	_CancelClientMenu(client, MenuCancel_Disconnected, true);

	player->bInMenu = false;
	player->bInExternMenu = false;
}

static int do_lookup[256];
void BaseMenuStyle::ProcessWatchList()
{
	if (!m_WatchList.size())
	{
		return;
	}

#if defined MENU_DEBUG
	logger->LogMessage("BaseMenuStyle::ProcessWatchList(%d,%d,%d,%d,%d,%p)", 
		m_WatchList.m_Size,
		m_WatchList.m_FirstLink,
		m_WatchList.m_FreeNodes,
		m_WatchList.m_LastLink,
		m_WatchList.m_MaxSize,
		m_WatchList.m_Nodes);
#endif

	unsigned int total = 0;
	for (FastLink<int>::iterator iter=m_WatchList.begin(); iter!=m_WatchList.end(); ++iter)
	{
		do_lookup[total++] = (*iter);
	}

#if defined MENU_DEBUG
	if (total)
	{
		logger->LogMessage("[SM_MENU] ProcessWatchList() found %d clients", total);
	}
#endif

	int client;
	CBaseMenuPlayer *player;
	float curTime = gpGlobals->curtime;
	for (unsigned int i=0; i<total; i++)
	{
		client = do_lookup[i];
		player = GetMenuPlayer(client);
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] ProcessWatchList() (client %d) (bInMenu %d) (menuHoldTime %d) (curTime %f) (menuStartTime %f)",
			client,
			player->bInMenu,
			player->menuHoldTime,
			curTime,
			player->menuStartTime);
#endif
		if (!player->bInMenu || !player->menuHoldTime)
		{
#if defined MENU_DEBUG
			logger->LogMessage("[SM_MENU] ProcessWatchList() removing client %d", client);
#endif
			m_WatchList.remove(client);
			continue;
		}
		if (curTime > player->menuStartTime + player->menuHoldTime)
		{
			_CancelClientMenu(client, MenuCancel_Timeout, false);
		}
	}
}

void BaseMenuStyle::ClientPressedKey(int client, unsigned int key_press)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] ClientPressedKey() (client %d) (key_press %d)", client, key_press);
#endif
	CBaseMenuPlayer *player = GetMenuPlayer(client);

	/* First question: Are we in a menu? */
	if (!player->bInMenu)
	{
		return;
	}

	bool cancel = false;
	unsigned int item = 0;
	MenuCancelReason reason = MenuCancel_Exit;
	MenuEndReason end_reason = MenuEnd_Selected;
	menu_states_t &states = player->states;

	/* Save variables */
	IMenuHandler *mh = states.mh;
	IBaseMenu *menu = states.menu;

	unsigned int item_on_page = states.item_on_page;

	assert(mh != NULL);

	if (menu == NULL)
	{
		item = key_press;
	} else if (key_press < 1 || key_press > GetMaxPageItems()) {
		cancel = true;
	} else {
		ItemSelection type = states.slots[key_press].type;

		/* Check if we should play a sound about the type */
		if (g_Menus.MenuSoundsEnabled() && 
			(!menu || (menu->GetMenuOptionFlags() & MENUFLAG_NO_SOUND) != MENUFLAG_NO_SOUND))
		{
			CellRecipientFilter filter;
			cell_t clients[1];

			clients[0] = client;
			filter.Initialize(clients, 1);

			const char *sound = g_Menus.GetMenuSound(type);

			if (sound != NULL)
			{
				edict_t *pEdict = PEntityOfEntIndex(client);
				if (pEdict)
				{
					ICollideable *pCollideable = pEdict->GetCollideable();

					if (pCollideable)
					{
						const Vector & pos = pCollideable->GetCollisionOrigin();
						enginesound->EmitSound(filter, 
							client, 
							CHAN_AUTO, 
#if SOURCE_ENGINE >= SE_PORTAL2
							sound, 
							-1, 
#endif
							sound, 
							VOL_NORM, 
							ATTN_NORM, 
#if SOURCE_ENGINE >= SE_PORTAL2
							0, 
#endif
							0, 
							PITCH_NORM, 
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_TF2
							0,
#endif
							&pos);
					}
				}
			}
		}

		/* For navigational items, we're going to redisplay */
		if (type == ItemSel_Back)
		{
			if (!RedoClientMenu(client, ItemOrder_Descending))
			{
				cancel = true;
				reason = MenuCancel_NoDisplay;
				end_reason = MenuEnd_Cancelled;
			} else {
				return;
			}
		} else if (type == ItemSel_Next) {
			if (!RedoClientMenu(client, ItemOrder_Ascending))
			{
				cancel = true;						/* I like Saltines. */
				reason = MenuCancel_NoDisplay;
				end_reason = MenuEnd_Cancelled;
			} else {
				return;
			}
		} else if (type == ItemSel_Exit || type == ItemSel_None) {
			cancel = true;
			reason = MenuCancel_Exit;
			end_reason = MenuEnd_Exit;
		} else if (type == ItemSel_ExitBack) {
			cancel = true;
			reason = MenuCancel_ExitBack;
			end_reason = MenuEnd_ExitBack;
		} else {
			item = states.slots[key_press].item;
		}
	}

	/* Clear states */
	player->bInMenu = false;
	if (player->menuHoldTime)
	{
		RemoveClientFromWatch(client);
	}

	Handle_t hndl = menu ? menu->GetHandle() : BAD_HANDLE;
	AutoHandleRooter ahr(hndl);

	if (cancel)
	{
		mh->OnMenuCancel(menu, client, reason);
	} else {
		mh->OnMenuSelect(menu, client, item);
		if (mh->GetMenuAPIVersion2() >= 13)
		{
			mh->OnMenuSelect2(menu, client, item, item_on_page);
		}
	}

	/* Only fire end for valid menus */
	if (menu)
	{
		mh->OnMenuEnd(menu, end_reason);
	}
}

bool BaseMenuStyle::DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] DoClientMenu() (client %d) (panel %p) (mh %p) (time %d)",
		client,
		menu,
		mh,
		time);
#endif
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer || pPlayer->IsFakeClient() || !pPlayer->IsInGame())
	{
		return false;
	}

	CBaseMenuPlayer *player = GetMenuPlayer(client);
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
		_CancelClientMenu(client, MenuCancel_Interrupted, true);
	}

	states.firstItem = 0;
	states.lastItem = 0;
	states.menu = NULL;
	states.mh = mh;
	states.apiVers = SMINTERFACE_MENUMANAGER_VERSION;
	player->bInMenu = true;
	player->bInExternMenu = false;
	player->menuStartTime = gpGlobals->curtime;
	player->menuHoldTime = time;

	if (time)
	{
		AddClientToWatch(client);
	}

	/* Draw the display */
	SendDisplay(client, menu);

	/* We can be interrupted again! */
	player->bAutoIgnore = false;

	return true;
}

bool BaseMenuStyle::DoClientMenu(int client,
								 CBaseMenu *menu,
								 unsigned int first_item,
								 IMenuHandler *mh,
								 unsigned int time)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] DoClientMenu() (client %d) (menu %p) (mh %p) (time %d)",
		client,
		menu,
		mh,
		time);
#endif
	mh->OnMenuStart(menu);

	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer || pPlayer->IsFakeClient() || !pPlayer->IsInGame())
	{
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] DoClientMenu(): Failed to display to client %d", client);
#endif
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu, MenuEnd_Cancelled);
		return false;
	}

	CBaseMenuPlayer *player = GetMenuPlayer(client);
	if (player->bAutoIgnore)
	{
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] DoClientMenu(): Client %d is autoIgnoring", client);
#endif
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu, MenuEnd_Cancelled);
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
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] DoClientMenu(): Cancelling old menu to client %d", client);
#endif
		_CancelClientMenu(client, MenuCancel_Interrupted, true);
	}

	states.firstItem = 0;
	states.lastItem = first_item;
	states.menu = menu;
	states.mh = mh;
	states.apiVers = SMINTERFACE_MENUMANAGER_VERSION;

	IMenuPanel *display = g_Menus.RenderMenu(client, states, ItemOrder_Ascending);
	if (!display)
	{
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] DoClientMenu(): Failed to render to client %d", client);
#endif
		player->bAutoIgnore = false;
		player->bInMenu = false;
		mh->OnMenuCancel(menu, client, MenuCancel_NoDisplay);
		mh->OnMenuEnd(menu, MenuEnd_Cancelled);
		return false;
	}

	/* Finally, set our states */
	player->bInMenu = true;
	player->bInExternMenu = false;
	player->menuStartTime = gpGlobals->curtime;
	player->menuHoldTime = time;

	if (time)
	{
		AddClientToWatch(client);
	}

	/* Draw the display */
	SendDisplay(client, display);

	/* Free the display pointer */
	display->DeleteThis();

	/* We can be interrupted again! */
	player->bAutoIgnore = false;

#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] DoClientMenu() finished successfully (client %d)", client);
#endif

	return true;
}

bool BaseMenuStyle::RedoClientMenu(int client, ItemOrder order)
{
#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] RedoClientMenu() (client %d) (order %d)", client, order);
#endif
	CBaseMenuPlayer *player = GetMenuPlayer(client);
	menu_states_t &states = player->states;

	player->bAutoIgnore = true;
	IMenuPanel *display = g_Menus.RenderMenu(client, states, order);
	if (!display)
	{
#if defined MENU_DEBUG
		logger->LogMessage("[SM_MENU] RedoClientMenu(): Failed to render menu");
#endif
		if (player->menuHoldTime)
		{
			RemoveClientFromWatch(client);
		}
		player->bAutoIgnore = false;
		return false;
	}

	SendDisplay(client, display);

	display->DeleteThis();

	player->bAutoIgnore = false;

#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] RedoClientMenu(): Succeeded to client %d", client);
#endif

	return true;
}

CBaseMenu::CBaseMenu(IMenuHandler *pHandler, IMenuStyle *pStyle, IdentityToken_t *pOwner) : 
m_pStyle(pStyle), m_Pagination(7), m_bShouldDelete(false), m_bCancelling(false), 
m_pOwner(pOwner ? pOwner : g_pCoreIdent), m_bDeleting(false), m_bWillFreeHandle(false), 
m_hHandle(BAD_HANDLE), m_pHandler(pHandler), m_nFlags(MENUFLAG_BUTTON_EXIT)
{
}

CBaseMenu::~CBaseMenu()
{
}

Handle_t CBaseMenu::GetHandle()
{
	if (!m_hHandle)
	{
		m_hHandle = g_Menus.CreateMenuHandle(this, m_pOwner);
	}

	return m_hHandle;
}

bool CBaseMenu::AppendItem(const char *info, const ItemDrawInfo &draw)
{
	if (m_Pagination == (unsigned)MENU_NO_PAGINATION
		&& m_items.size() >= m_pStyle->GetMaxPageItems())
	{
		return false;
	}

	CItem item(m_items.size());

	item.info = info;
	if (draw.display)
		item.display = std::make_unique<std::string>(draw.display);
	item.style = draw.style;

	m_items.push_back(std::move(item));
	return true;
}

bool CBaseMenu::InsertItem(unsigned int position, const char *info, const ItemDrawInfo &draw)
{
	if (m_Pagination == (unsigned)MENU_NO_PAGINATION &&
	    m_items.size() >= m_pStyle->GetMaxPageItems())
	{
		return false;
	}

	if (position >= m_items.size())
		return false;

	CItem item(position);
	item.info = info;
	if (draw.display)
		item.display = std::make_unique<std::string>(draw.display);
	item.style = draw.style;

	m_items.emplace(m_items.begin() + position, std::move(item));
	return true;
}

bool CBaseMenu::RemoveItem(unsigned int position)
{
	if (position >= m_items.size())
		return false;

	m_items.erase(m_items.begin() + position);
	return true;
}

void CBaseMenu::RemoveAllItems()
{
	m_items.clear();
}

const char *CBaseMenu::GetItemInfo(unsigned int position, ItemDrawInfo *draw/* =NULL */, int client/* =0 */)
{
	if (position >= m_items.size())
		return NULL;

	if (client > 0 && position < m_RandomMaps[client].size())
	{
		position = m_RandomMaps[client][position];
	}

	if (draw)
	{
		draw->display = m_items[position].display->c_str();
		draw->style = m_items[position].style;
	}

	return m_items[position].info.c_str();
}

void CBaseMenu::ShufflePerClient(int start, int stop)
{
	// limit map len to 255 items since it's using uint8
	int length = MIN(GetItemCount(), 255);
	if (stop >= 0)
		length = MIN(length, stop);

	for (int i = 1; i <= SM_MAXPLAYERS; i++)
	{
		// populate per-client map ...
		m_RandomMaps[i].resize(length);
		for (int j = 0; j < length; j++)
			m_RandomMaps[i][j] = j;

		// ... and random shuffle it
		for (int j = length - 1; j > start; j--)
		{
			int x = rand() % (j - start + 1) + start;
			uint8_t tmp = m_RandomMaps[i][x];
			m_RandomMaps[i][x] = m_RandomMaps[i][j];
			m_RandomMaps[i][j] = tmp;
		}
	}
}

void CBaseMenu::SetClientMapping(int client, int *array, int length)
{
	length = MIN(length, 255);
	m_RandomMaps[client].resize(length);
	for (int i = 0; i < length; i++)
	{
		m_RandomMaps[client][i] = array[i];
	}
}

bool CBaseMenu::IsPerClientShuffled()
{
	for (int i = 1; i <= SM_MAXPLAYERS; i++)
	{
		if(m_RandomMaps[i].size() > 0)
			return true;
	}
	return false;
}

unsigned int CBaseMenu::GetRealItemIndex(int client, unsigned int position)
{
	if (client > 0 && position < m_RandomMaps[client].size())
	{
		position = m_RandomMaps[client][position];
		return m_items[position].index;
	}

	return position;
}

unsigned int CBaseMenu::GetItemCount()
{
	return m_items.size();
}

bool CBaseMenu::SetPagination(unsigned int itemsPerPage)
{
	if (itemsPerPage > 7 || itemsPerPage == 1)
	{
		return false;
	}

	if (itemsPerPage == MENU_NO_PAGINATION
		&& m_Pagination != MENU_NO_PAGINATION)
	{
		m_nFlags &= ~MENUFLAG_BUTTON_EXIT;
	}

	m_Pagination = itemsPerPage;

	return true;
}

unsigned int CBaseMenu::GetPagination()
{
	return m_Pagination;
}

IMenuStyle *CBaseMenu::GetDrawStyle()
{
	return m_pStyle;
}

void CBaseMenu::SetDefaultTitle(const char *message)
{
	m_Title = message;
}

const char *CBaseMenu::GetDefaultTitle()
{
	return m_Title.c_str();
}

void CBaseMenu::Cancel()
{
	if (m_bCancelling)
	{
		return;
	}

#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] CBaseMenu::Cancel(%p) (m_bShouldDelete %d)",
		this,
		m_bShouldDelete);
#endif

	m_bCancelling = true;
	Cancel_Finally();
	m_bCancelling = false;

	if (m_bShouldDelete)
	{
		InternalDelete();
	}
}

void CBaseMenu::Destroy(bool releaseHandle)
{
	/* Check if we shouldn't be here */
	if (m_bDeleting)
	{
		return;
	}

#if defined MENU_DEBUG
	logger->LogMessage("[SM_MENU] CBaseMenu::Destroy(%p) (release %d) (m_bCancelling %d) (m_bShouldDelete %d)",
		this,
		releaseHandle,
		m_bCancelling,
		m_bShouldDelete);
#endif

	/* Save the destruction hint about our handle */
	m_bWillFreeHandle = releaseHandle;

	/* Now actually do stuff */
	if (!m_bCancelling || m_bShouldDelete)
	{
		Cancel();
		InternalDelete();
	} else {
		m_bShouldDelete = true;
	}
}

void CBaseMenu::InternalDelete()
{
	if (m_bWillFreeHandle && m_hHandle != BAD_HANDLE)
	{
		Handle_t hndl = m_hHandle;
		HandleSecurity sec;

		sec.pOwner = m_pOwner;
		sec.pIdentity = g_pCoreIdent;

		m_hHandle = BAD_HANDLE;
		m_bDeleting = true;
		handlesys->FreeHandle(hndl, &sec);
	}

	m_pHandler->OnMenuDestroy(this);

	delete this;
}

unsigned int CBaseMenu::GetMenuOptionFlags()
{
	return m_nFlags;
}

void CBaseMenu::SetMenuOptionFlags(unsigned int flags)
{
	m_nFlags = flags;
}

IMenuHandler *CBaseMenu::GetHandler()
{
	return m_pHandler;
}

unsigned int CBaseMenu::GetBaseMemUsage()
{
	return m_Title.size() + (m_items.size() * sizeof(CItem));
}
