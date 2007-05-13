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

#ifndef _INCLUDE_SOURCEMOD_MENUMANAGER_H_
#define _INCLUDE_SOURCEMOD_MENUMANAGER_H_

#include <IMenuManager.h>
#include <sh_vector.h>
#include <sh_stack.h>
#include <sh_list.h>
#include "sm_memtable.h"
#include "sm_globals.h"

using namespace SourceMod;
using namespace SourceHook;

class BroadcastHandler : public IMenuHandler
{
public:
	BroadcastHandler(IMenuHandler *handler);
public: //IMenuHandler
	void OnMenuDisplay(IBaseMenu *menu, int client, IMenuDisplay *display);
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnMenuEnd(IBaseMenu *menu);
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	unsigned int GetMenuAPIVersion2();
public:
	virtual void OnBroadcastEnd(IBaseMenu *menu);
public:
	IMenuHandler *m_pHandler;
	unsigned int numClients;
};

class VoteHandler : public BroadcastHandler
{
public:
	VoteHandler(IMenuVoteHandler *handler);
public:
	void Initialize(IBaseMenu *menu);
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnBroadcastEnd(IBaseMenu *menu);
private:
	CVector<unsigned int> m_counts;
	CVector<unsigned int> m_ties;
	unsigned int numItems;
	IMenuVoteHandler *m_pVoteHandler;
};

class MenuManager : 
	public IMenuManager,
	public SMGlobalClass
{
	friend class BroadcastHandler;
	friend class VoteHandler;
public:
	MenuManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
public: //IMenuManager
	virtual const char *GetInterfaceName()
	{
		return SMINTERFACE_MENUMANAGER_NAME;
	}
	virtual unsigned int GetInterfaceVersion()
	{
		return SMINTERFACE_MENUMANAGER_VERSION;
	}
public:
	unsigned int GetStyleCount();
	IMenuStyle *GetStyle(unsigned int index);
	IMenuStyle *FindStyleByName(const char *name);
	unsigned int BroadcastMenu(IBaseMenu *menu, 
		IMenuHandler *handler,
		int clients[], 
		unsigned int numClients,
		unsigned int time);
	unsigned int VoteMenu(IBaseMenu *menu, 
		IMenuVoteHandler *handler,
		int clients[], 
		unsigned int numClients,
		unsigned int time);
	IMenuStyle *GetDefaultStyle();
	void AddStyle(IMenuStyle *style);
	bool SetDefaultStyle(IMenuStyle *style);
	IMenuDisplay *RenderMenu(int client, menu_states_t &states, ItemOrder order);
protected:
	void FreeBroadcastHandler(BroadcastHandler *bh);
	void FreeVoteHandler(VoteHandler *vh);
private:
	int m_ShowMenu;
	IMenuStyle *m_pDefaultStyle;
	CStack<BroadcastHandler *> m_BroadcastHandlers;
	CStack<VoteHandler *> m_VoteHandlers;
	CVector<IMenuStyle *> m_Styles;
};

extern MenuManager g_Menus;

#endif //_INCLUDE_SOURCEMOD_MENUMANAGER_H_
