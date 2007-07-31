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
#include <sh_string.h>
#include "sm_memtable.h"
#include "sm_globals.h"

using namespace SourceMod;
using namespace SourceHook;

class VoteMenuHandler : public IVoteMenuHandler
{
public: //IMenuHandler
	unsigned int GetMenuAPIVersion2();
	void OnMenuStart(IBaseMenu *menu);
	void OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display);
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason);
	void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style);
	unsigned int OnMenuDisplayItem(IBaseMenu *menu, int client, IMenuPanel *panel, unsigned int item, const ItemDrawInfo &dr);
public: //IVoteMenuHandler
	bool IsVoteInProgress();
	void InitializeVoting(IBaseMenu *menu);
	void StartVoting();
	void CancelVoting();
public:
	void Reset(IMenuHandler *mh);
private:
	void DecrementPlayerCount();
	void EndVoting();
	void InternalReset();
private:
	IMenuHandler *m_pHandler;
	unsigned int m_Clients;
	unsigned int m_Items;
	CVector<unsigned int> m_Votes;
	IBaseMenu *m_pCurMenu;
	bool m_bStarted;
	bool m_bCancelled;
	unsigned int m_NumVotes;
};

class MenuManager : 
	public IMenuManager,
	public SMGlobalClass,
	public IHandleTypeDispatch
{
	friend class BroadcastHandler;
	friend class VoteHandler;
	friend class CBaseMenu;
	friend class BaseMenuStyle;
public:
	MenuManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
	ConfigResult OnSourceModConfigChanged(const char *key,
		const char *value,
		ConfigSource source,
		char *error,
		size_t maxlength);
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
	IMenuStyle *GetDefaultStyle();
	void AddStyle(IMenuStyle *style);
	bool SetDefaultStyle(IMenuStyle *style);
	IMenuPanel *RenderMenu(int client, menu_states_t &states, ItemOrder order);
	IVoteMenuHandler *CreateVoteWrapper(IMenuHandler *mh);
	void ReleaseVoteWrapper(IVoteMenuHandler *mh);
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public:
	HandleError ReadMenuHandle(Handle_t handle, IBaseMenu **menu);
	HandleError ReadStyleHandle(Handle_t handle, IMenuStyle **style);
public:
	bool MenuSoundsEnabled();
	const char *GetMenuSound(ItemSelection sel);
protected:
	Handle_t CreateMenuHandle(IBaseMenu *menu, IdentityToken_t *pOwner);
	Handle_t CreateStyleHandle(IMenuStyle *style);
private:
	int m_ShowMenu;
	IMenuStyle *m_pDefaultStyle;
	CStack<VoteMenuHandler *> m_VoteHandlers;
	CVector<IMenuStyle *> m_Styles;
	HandleType_t m_StyleType;
	HandleType_t m_MenuType;
	String m_SelectSound;
	String m_ExitBackSound;
};

extern MenuManager g_Menus;

#endif //_INCLUDE_SOURCEMOD_MENUMANAGER_H_
