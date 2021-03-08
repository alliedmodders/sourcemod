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

#ifndef _INCLUDE_MENUSTYLE_BASE_H
#define _INCLUDE_MENUSTYLE_BASE_H

#include <memory>
#include <utility>

#include <IMenuManager.h>
#include <IPlayerHelpers.h>
#include <am-string.h>
#include <am-vector.h>
#include "sm_fastlink.h"

using namespace SourceMod;

class CItem
{
public:
	CItem(unsigned int index)
	{
		this->index = index;
		style = 0;
		access = 0;
	}
	CItem(CItem &&other)
	: info(std::move(other.info)),
	  display(std::move(other.display))
	{
		index = other.index;
		style = other.style;
		access = other.access;
	}
	CItem & operator =(CItem &&other)
	{
		index = other.index;
		info = std::move(other.info);
		display = std::move(other.display);
		style = other.style;
		access = other.access;
		return *this;
	}

public:
	unsigned int index;
	std::string info;
	std::unique_ptr<std::string> display;
	unsigned int style;
	unsigned int access;

private:
	CItem(const CItem &other) = delete;
	CItem &operator =(const CItem &other) = delete;
};

class CBaseMenuPlayer
{
public:
	CBaseMenuPlayer() : bInMenu(false), bAutoIgnore(false), bInExternMenu(false)
	{
	}
	menu_states_t states;
	bool bInMenu;
	bool bAutoIgnore;
	float menuStartTime;
	unsigned int menuHoldTime;
	bool bInExternMenu;
};

class CBaseMenu;

class BaseMenuStyle : 
	public IMenuStyle,
	public IClientListener
{
public:
	BaseMenuStyle();
public: //IMenuStyle
	bool CancelClientMenu(int client, bool autoIgnore/* =false */);
	MenuSource GetClientMenu(int client, void **object);
	Handle_t GetHandle();
public: //IClientListener
	void OnClientDisconnected(int client);
public: //what derived must implement
	virtual CBaseMenuPlayer *GetMenuPlayer(int client) =0;
	virtual void SendDisplay(int client, IMenuPanel *display) =0;
public: //what derived may implement 
	virtual bool DoClientMenu(int client, 
		CBaseMenu *menu, 
		unsigned int first_item,
		IMenuHandler *mh, 
		unsigned int time);
	virtual bool DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time);
	virtual void AddClientToWatch(int client);
	virtual void RemoveClientFromWatch(int client);
	virtual void ProcessWatchList();
public: //helpers
	void CancelMenu(CBaseMenu *menu);
	void ClientPressedKey(int client, unsigned int key_press);
protected:
	void _CancelClientMenu(int client, MenuCancelReason reason, bool bAutoIgnore=false);
	bool RedoClientMenu(int client, ItemOrder order);
protected:
	FastLink<int> m_WatchList;
	Handle_t m_hHandle;
};

class CBaseMenu : public IBaseMenu
{
public:
	CBaseMenu(IMenuHandler *pHandler, IMenuStyle *pStyle, IdentityToken_t *pOwner);
	virtual ~CBaseMenu();
public:
	virtual bool AppendItem(const char *info, const ItemDrawInfo &draw);
	virtual bool InsertItem(unsigned int position, const char *info, const ItemDrawInfo &draw);
	virtual bool RemoveItem(unsigned int position);
	virtual void RemoveAllItems();
	virtual const char *GetItemInfo(unsigned int position, ItemDrawInfo *draw=NULL, int client=0);
	virtual unsigned int GetItemCount();
	virtual bool SetPagination(unsigned int itemsPerPage);
	virtual unsigned int GetPagination();
	virtual IMenuStyle *GetDrawStyle();
	virtual void SetDefaultTitle(const char *message);
	virtual const char *GetDefaultTitle();
	virtual void Cancel();
	virtual void Destroy(bool releaseHandle);
	virtual void Cancel_Finally() =0;
	virtual Handle_t GetHandle();
	virtual unsigned int GetMenuOptionFlags();
	virtual void SetMenuOptionFlags(unsigned int flags);
	virtual IMenuHandler *GetHandler();
	virtual void ShufflePerClient(int start, int stop);
	virtual void SetClientMapping(int client, int *array, int length);
	virtual bool IsPerClientShuffled();
	virtual unsigned int GetRealItemIndex(int client, unsigned int position);
	unsigned int GetBaseMemUsage();
private:
	void InternalDelete();
protected:
	std::string m_Title;
	IMenuStyle *m_pStyle;
	unsigned int m_Pagination;
	std::vector<CItem> m_items;
	bool m_bShouldDelete;
	bool m_bCancelling;
	IdentityToken_t *m_pOwner;
	bool m_bDeleting;
	bool m_bWillFreeHandle;
	Handle_t m_hHandle;
	IMenuHandler *m_pHandler;
	unsigned int m_nFlags;
	std::vector<uint8_t> m_RandomMaps[SM_MAXPLAYERS+1];
};

#endif //_INCLUDE_MENUSTYLE_BASE_H
