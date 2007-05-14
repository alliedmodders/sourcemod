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

#ifndef _INCLUDE_MENUSTYLE_BASE_H
#define _INCLUDE_MENUSTYLE_BASE_H

#include <IMenuManager.h>
#include <IPlayerHelpers.h>
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_memtable.h"
#include "sm_fastlink.h"

using namespace SourceMod;
using namespace SourceHook;

class CItem
{
public:
	CItem()
	{
		infoString = -1;
		displayString = -1;
		style = 0;
	}
public:
	int infoString;
	int displayString;
	unsigned int style;
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
	virtual bool DoClientMenu(int client, CBaseMenu *menu, IMenuHandler *mh, unsigned int time);
	virtual bool DoClientMenu(int client, IMenuPanel *menu, IMenuHandler *mh, unsigned int time);
	virtual void AddClientToWatch(int client);
	virtual void RemoveClientFromWatch(int client);
	virtual void ProcessWatchList();
public: //helpers
	void CancelMenu(CBaseMenu *menu);
	void ClientPressedKey(int client, unsigned int key_press);
protected:
	void _CancelClientMenu(int client, bool bAutoIgnore=false, MenuCancelReason reason=MenuCancel_Interrupt);
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
	virtual const char *GetItemInfo(unsigned int position, ItemDrawInfo *draw=NULL);
	virtual unsigned int GetItemCount();
	virtual bool SetPagination(unsigned int itemsPerPage);
	virtual unsigned int GetPagination();
	virtual IMenuStyle *GetDrawStyle();
	virtual void SetDefaultTitle(const char *message);
	virtual const char *GetDefaultTitle();
	virtual bool GetExitButton();
	virtual bool SetExitButton(bool set);
	virtual void Cancel();
	virtual void Destroy(bool releaseHandle);
	virtual void Cancel_Finally() =0;
	virtual Handle_t GetHandle();
private:
	void InternalDelete();
protected:
	String m_Title;
	IMenuStyle *m_pStyle;
	BaseStringTable m_Strings;
	unsigned int m_Pagination;
	CVector<CItem> m_items;
	bool m_ExitButton;
	bool m_bShouldDelete;
	bool m_bCancelling;
	IdentityToken_t *m_pOwner;
	bool m_bDeleting;
	bool m_bWillFreeHandle;
	Handle_t m_hHandle;
	IMenuHandler *m_pHandler;
};

#endif //_INCLUDE_MENUSTYLE_BASE_H
