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

#include "common_logic.h"
#include <sh_stack.h>
#include <IMenuManager.h>
#include <IPlayerHelpers.h>
#include "DebugReporter.h"
#if defined MENU_DEBUG
#include "Logger.h"
#endif
#include <ISourceMod.h>
#include <stdlib.h>
#include <bridge/include/IScriptManager.h>

#if defined CreateMenu
#undef CreateMenu
#endif

/**
 * And God said, "let there be menus," and behold, there were menus.  
 * God saw the menus and they were good.  And the evening and the morning
 * were the third day.
 */

enum MenuAction
{
	MenuAction_Start = (1<<0),		/**< A menu has been started (nothing passed) */
	MenuAction_Display = (1<<1),	/**< A menu is about to be displayed (param1=client, param2=MenuPanel Handle) */
	MenuAction_Select = (1<<2),		/**< An item was selected (param1=client, param2=item) */
	MenuAction_Cancel = (1<<3),		/**< The menu was cancelled (param1=client, param2=item) */
	MenuAction_End = (1<<4),		/**< A menu's display/selection cycle is complete (nothing passed). */
	MenuAction_VoteEnd = (1<<5),	/**< (VOTE ONLY): A vote sequence has ended (param1=chosen item) */
	MenuAction_VoteStart = (1<<6), 	/**< (VOTE ONLY): A vote sequence has started */
	MenuAction_VoteCancel = (1<<7),	/**< (VOTE ONLY): A vote sequence has been cancelled (nothing passed) */
	MenuAction_DrawItem = (1<<8),	/**< A style is being drawn; return the new style (param1=client, param2=item) */
	MenuAction_DisplayItem = (1<<9),	/**< the odd duck */
};

static HandleError ReadMenuHandle(Handle_t handle, IBaseMenu **menu)
{
	static HandleType_t menuType = NO_HANDLE_TYPE;
	if (menuType == NO_HANDLE_TYPE && !handlesys->FindHandleType("IBaseMenu", &menuType))
	{
		// This should never happen so exact error doesn't matter.
		return HandleError_Index;
	}

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = NULL;
	return handlesys->ReadHandle(handle, menuType, &sec, (void **)menu);
}

static HandleError ReadStyleHandle(Handle_t handle, IMenuStyle **style)
{
	static HandleType_t styleType = NO_HANDLE_TYPE;
	if (styleType == NO_HANDLE_TYPE && !handlesys->FindHandleType("IMenuStyle", &styleType))
	{
		// This should never happen so exact error doesn't matter.
		return HandleError_Index;
	}

	HandleSecurity sec;

	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = g_pCoreIdent;

	return handlesys->ReadHandle(handle, styleType, &sec, (void **)style);
}

static IMenuStyle &ValveMenuStyle()
{
	static IMenuStyle *valveMenuStyle = menus->FindStyleByName("valve");
	return *valveMenuStyle;
}

static IMenuStyle &RadioMenuStyle()
{
	static IMenuStyle *radioMenuStyle = menus->FindStyleByName("radio");
	return *radioMenuStyle;
}

class CPanelHandler : public IMenuHandler
{
	friend class MenuNativeHelpers;
public:
	CPanelHandler()
	{
	}
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
private:
	IPluginFunction *m_pFunc;
	IPlugin *m_pPlugin;
};

class CMenuHandler : public IMenuHandler
{
	friend class MenuNativeHelpers;
public:
	CMenuHandler(IPluginFunction *pBasic, int flags);
public:
	void OnMenuStart(IBaseMenu *menu);
	void OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display);
	void OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page);
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason);
	void OnMenuDestroy(IBaseMenu *menu);
	void OnMenuVoteStart(IBaseMenu *menu);
	void OnMenuVoteResults(IBaseMenu *menu, const menu_vote_result_t *results);
	void OnMenuVoteCancel(IBaseMenu *menu, VoteCancelReason reason);
	void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style);
	unsigned int OnMenuDisplayItem(IBaseMenu *menu, int client, IMenuPanel *panel, unsigned int item, const ItemDrawInfo &dr);
	bool OnSetHandlerOption(const char *option, const void *data);
private:
	cell_t DoAction(IBaseMenu *menu, MenuAction action, cell_t param1, cell_t param2, cell_t def_res=0);
private:
	IPluginFunction *m_pBasic;
	int m_Flags;
	IPluginFunction *m_pVoteResults;
	cell_t m_fnVoteResult;
};

/**
 * GLOBAL CLASS FOR HELPERS
 */

class MenuNativeHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener
{
public:
	virtual void OnSourceModAllInitialized()
	{
		m_PanelType = handlesys->CreateType("IMenuPanel", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		m_TempPanelType = handlesys->CreateType("TempIMenuPanel", this, m_PanelType, NULL, NULL, g_pCoreIdent, NULL);
		scripts->AddPluginsListener(this);
	}

	virtual void OnSourceModShutdown()
	{
		scripts->RemovePluginsListener(this);
		handlesys->RemoveType(m_TempPanelType, g_pCoreIdent);
		handlesys->RemoveType(m_PanelType, g_pCoreIdent);

		while (!m_FreePanelHandlers.empty())
		{
			delete m_FreePanelHandlers.front();
			m_FreePanelHandlers.pop();
		}

		while (!m_FreeMenuHandlers.empty())
		{
			delete m_FreeMenuHandlers.front();
			m_FreeMenuHandlers.pop();
		}
	}

	virtual void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == m_TempPanelType)
		{
			return;
		}

		IMenuPanel *panel = (IMenuPanel *)object;
		panel->DeleteThis();
	}

	virtual bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		*pSize = ((IMenuPanel *)object)->GetApproxMemUsage();
		return true;
	}

	/**
	 * It is extremely important that unloaded plugins don't crash.
	 * Thus, if a plugin unloads, we run through every handler we have.
	 * This means we do almost no runtime work for keeping track of
	 * our panel handlers (we don't have to store a list of the running 
	 * ones), but when push comes to shove, we have to scan them all
	 * in case any of them are active.
	 */
	virtual void OnPluginUnloaded(IPlugin *plugin)
	{
		for (size_t i = 0; i < m_PanelHandlers.size(); i++)
		{
			if (m_PanelHandlers[i]->m_pPlugin == plugin)
			{
				m_PanelHandlers[i]->m_pPlugin = NULL;
				m_PanelHandlers[i]->m_pFunc = NULL;
			}
		}
	}

	inline HandleType_t GetPanelType()
	{
		return m_PanelType;
	}

	inline HandleType_t GetTempPanelType()
	{
		return m_TempPanelType;
	}

	CPanelHandler *GetPanelHandler(IPluginFunction *pFunction)
	{
		CPanelHandler *handler;
		if (m_FreePanelHandlers.empty())
		{
			handler = new CPanelHandler;
			m_PanelHandlers.push_back(handler);
		} else {
			handler = m_FreePanelHandlers.front();
			m_FreePanelHandlers.pop();
		}
		handler->m_pFunc = pFunction;
		handler->m_pPlugin = scripts->FindPluginByContext(pFunction->GetParentContext()->GetContext());
		return handler;
	}

	void FreePanelHandler(CPanelHandler *handler)
	{
		handler->m_pFunc = NULL;
		handler->m_pPlugin = NULL;
		m_FreePanelHandlers.push(handler);
	}

	CMenuHandler *GetMenuHandler(IPluginFunction *pFunction, int flags)
	{
		CMenuHandler *handler;
		if (m_FreeMenuHandlers.empty())
		{
			handler = new CMenuHandler(pFunction, flags);
		} else {
			handler = m_FreeMenuHandlers.front();
			m_FreeMenuHandlers.pop();
			handler->m_pBasic = pFunction;
			handler->m_Flags = flags;
			handler->m_pVoteResults = NULL;
		}
		return handler;
	}

	void FreeMenuHandler(CMenuHandler *handler)
	{
		m_FreeMenuHandlers.push(handler);
	}

private:
	HandleType_t m_PanelType;
	HandleType_t m_TempPanelType;
	CStack<CPanelHandler *> m_FreePanelHandlers;
	CStack<CMenuHandler *> m_FreeMenuHandlers;
	CVector<CPanelHandler *> m_PanelHandlers;
} g_MenuHelpers;

/**
 * BASIC PANEL HANDLER WRAPPER
 */

void CPanelHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	if (m_pFunc)
	{
		m_pFunc->PushCell(BAD_HANDLE);
		m_pFunc->PushCell(MenuAction_Cancel);
		m_pFunc->PushCell(client);
		m_pFunc->PushCell(reason);
		m_pFunc->Execute(NULL);
	}
	g_MenuHelpers.FreePanelHandler(this);
}

void CPanelHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	if (m_pFunc)
	{
		unsigned int old_reply = playerhelpers->SetReplyTo(SM_REPLY_CHAT);
		m_pFunc->PushCell(BAD_HANDLE);
		m_pFunc->PushCell(MenuAction_Select);
		m_pFunc->PushCell(client);
		m_pFunc->PushCell(item);
		m_pFunc->Execute(NULL);
		playerhelpers->SetReplyTo(old_reply);
	}
	g_MenuHelpers.FreePanelHandler(this);
}

static IMenuPanel *s_pCurPanel = NULL;
static unsigned int s_CurPanelReturn = 0;
static const ItemDrawInfo *s_CurDrawInfo = NULL;
static unsigned int *s_CurSelectPosition = NULL;

/**
 * MENU HANDLER WRAPPER
 */
CMenuHandler::CMenuHandler(IPluginFunction *pBasic, int flags) : 
	m_pBasic(pBasic), m_Flags(flags), m_pVoteResults(NULL)
{
	/* :TODO: We can probably cache the handle ahead of time */
}

void CMenuHandler::OnMenuStart(IBaseMenu *menu)
{
	if ((m_Flags & (int)MenuAction_Start) == (int)MenuAction_Start)
	{
		DoAction(menu, MenuAction_Start, 0, 0);
	}
}

void CMenuHandler::OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *panel)
{
	if ((m_Flags & (int)MenuAction_Display) == (int)MenuAction_Display)
	{
		HandleSecurity sec;
		sec.pIdentity = g_pCoreIdent;
		sec.pOwner = m_pBasic->GetParentContext()->GetIdentity();
	
		HandleAccess access;
		handlesys->InitAccessDefaults(NULL, &access);
		access.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

		Handle_t hndl = handlesys->CreateHandleEx(g_MenuHelpers.GetTempPanelType(), panel, &sec, &access, NULL);

		DoAction(menu, MenuAction_Display, client, hndl);

		handlesys->FreeHandle(hndl, &sec);
	}
}

void CMenuHandler::OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page)
{
	/* Save old position first. */
	unsigned int first_item = item_on_page;
	unsigned int *old_pos = s_CurSelectPosition;

	s_CurSelectPosition = &first_item;

	unsigned int old_reply = playerhelpers->SetReplyTo(SM_REPLY_CHAT);
	DoAction(menu, MenuAction_Select, client, item);
	playerhelpers->SetReplyTo(old_reply);

	s_CurSelectPosition = old_pos;
}

void CMenuHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	unsigned int old_reply = playerhelpers->SetReplyTo(SM_REPLY_CHAT);
	DoAction(menu, MenuAction_Cancel, client, (cell_t)reason);
	playerhelpers->SetReplyTo(old_reply);
}

void CMenuHandler::OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
{
	DoAction(menu, MenuAction_End, reason, 0);
}

void CMenuHandler::OnMenuDestroy(IBaseMenu *menu)
{
	g_MenuHelpers.FreeMenuHandler(this);
}

void CMenuHandler::OnMenuVoteStart(IBaseMenu *menu)
{
	DoAction(menu, MenuAction_VoteStart, 0, 0);
}

void CMenuHandler::OnMenuVoteCancel(IBaseMenu *menu, VoteCancelReason reason)
{
	DoAction(menu, MenuAction_VoteCancel, reason, 0);
}

void CMenuHandler::OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style)
{
	if ((m_Flags & (int)MenuAction_DrawItem) == (int)MenuAction_DrawItem)
	{
		cell_t result = DoAction(menu, MenuAction_DrawItem, client, item, style);
		style = (unsigned int)result;
	}
}

unsigned int CMenuHandler::OnMenuDisplayItem(IBaseMenu *menu,
									 int client,
									 IMenuPanel *panel,
									 unsigned int item,
									 const ItemDrawInfo &dr)
{
	if ((m_Flags & (int)MenuAction_DisplayItem) == (int)MenuAction_DisplayItem)
	{
		IMenuPanel *oldpanel = s_pCurPanel;
		unsigned int oldret = s_CurPanelReturn;
		const ItemDrawInfo *oldinfo = s_CurDrawInfo;
		s_pCurPanel = panel;
		s_CurPanelReturn = 0;
		s_CurDrawInfo = &dr;

		cell_t res = DoAction(menu, MenuAction_DisplayItem, client, item, 0);

		if (!res)
		{
			res = s_CurPanelReturn;
		}

		s_pCurPanel = oldpanel;
		s_CurPanelReturn = oldret;
		s_CurDrawInfo = oldinfo;

		return res;
	}

	return 0;
}

cell_t CMenuHandler::DoAction(IBaseMenu *menu, MenuAction action, cell_t param1, cell_t param2, cell_t def_res)
{
#if defined MENU_DEBUG
	g_Logger.LogMessage("[SM_MENU] CMenuHandler::DoAction() (menu %p/%08x) (action %d) (param1 %d) (param2 %d)",
		menu,
		menu->GetHandle(),
		action,
		param1,
		param2);
#endif
	cell_t res = def_res;
	m_pBasic->PushCell(menu->GetHandle());
	m_pBasic->PushCell((cell_t)action);
	m_pBasic->PushCell(param1);
	m_pBasic->PushCell(param2);
	m_pBasic->Execute(&res);
	return res;
}

void CMenuHandler::OnMenuVoteResults(IBaseMenu *menu, const menu_vote_result_t *results)
{
	if (!m_pVoteResults)
	{
		/* Call MenuAction_VoteEnd instead.  See if there are any extra winners. */
		unsigned int num_items = 1;
		for (unsigned int i=1; i<results->num_items; i++)
		{
			if (results->item_list[i].count != results->item_list[0].count)
			{
				break;
			}
			num_items++;
		}

		/* See if we need to pick a random winner. */
		unsigned int winning_item;
		if (num_items > 1)
		{
			/* Yes, we do. */
			winning_item = rand() % num_items;
			winning_item = results->item_list[winning_item].item;
		} else {
			/* No, take the first. */
			winning_item = results->item_list[0].item;
		}

		unsigned int total_votes = results->num_votes;
		unsigned int winning_votes = results->item_list[0].count;

		DoAction(menu, MenuAction_VoteEnd, winning_item, (total_votes << 16) | (winning_votes & 0xFFFF));
		return;
	}

	IPluginContext *pContext = m_pVoteResults->GetParentContext();
	AutoEnterHeapScope heap_scope(pContext);

	/* First array */
	cell_t client_array_address = -1;
	if (cell_t client_array_size = results->num_clients * 2) {
		auto init = std::make_unique<cell_t[]>(client_array_size);
		for (unsigned int i = 0; i < results->num_clients; i++) {
			init[i * 2] = results->client_list[i].client;
			init[i * 2 + 1] = results->client_list[i].item;
		}

		if (!pContext->HeapAlloc2dArray(results->num_clients, 2, &client_array_address, init.get())) {
			g_DbgReporter.GenerateError(pContext, m_fnVoteResult, -1,
										"Menu callback could not allocate cells for client list.");
			return;
		}
	}

	/* Second array */
	cell_t item_array_address = -1;
	if (cell_t item_array_size = results->num_items * 2) {
		auto init = std::make_unique<cell_t[]>(item_array_size);
		for (unsigned int i = 0; i < results->num_items; i++) {
			init[i * 2] = results->item_list[i].item;
			init[i * 2 + 1] = results->item_list[i].count;
		}
		if (!pContext->HeapAlloc2dArray(results->num_items, 2, &item_array_address, init.get())) {
			g_DbgReporter.GenerateError(pContext, m_fnVoteResult, -1,
										"Menu callback could not allocate %d cells for item list.",
										item_array_size);
			return;
		}
	}

	m_pVoteResults->PushCell(menu->GetHandle());
	m_pVoteResults->PushCell(results->num_votes);
	m_pVoteResults->PushCell(results->num_clients);
	m_pVoteResults->PushCell(client_array_address);
	m_pVoteResults->PushCell(results->num_items);
	m_pVoteResults->PushCell(item_array_address);
	m_pVoteResults->Execute(NULL);
}

bool CMenuHandler::OnSetHandlerOption(const char *option, const void *data)
{
	if (strcmp(option, "set_vote_results_handler") == 0)
	{
		void **array = (void **)data;
		m_pVoteResults = (IPluginFunction *)array[0];
		m_fnVoteResult = *(cell_t *)((cell_t *)array[1]);
		return true;
	}

	return false;
}

/**
 * INLINE FUNCTIONS FOR NATIVES
 */

inline Handle_t MakePanelHandle(IMenuPanel *panel, IPluginContext *pContext)
{
	return handlesys->CreateHandle(g_MenuHelpers.GetPanelType(), panel, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

inline HandleError ReadPanelHandle(Handle_t hndl, IMenuPanel **panel)
{
	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = NULL;
	return handlesys->ReadHandle(hndl, g_MenuHelpers.GetPanelType(), &sec, (void **)panel);
}

inline IMenuStyle *GetStyleFromCell(cell_t cell)
{
	enum MenuStyle
	{
		MenuStyle_Default = 0,		/**< The "default" menu style for the mod */
		MenuStyle_Valve = 1,		/**< The Valve provided menu style (Used on HL2DM) */
		MenuStyle_Radio = 2,		/**< The simpler menu style commonly used on CS:S */
	};

	if (cell == MenuStyle_Valve)
	{
		return &ValveMenuStyle();
	} else if (cell == MenuStyle_Radio
			   && RadioMenuStyle().IsSupported())
	{
		return &RadioMenuStyle();
	}

	return menus->GetDefaultStyle();
}

/***********************************
 **** NATIVE DEFINITIONS ***********
 ***********************************/

static cell_t CreateMenu(IPluginContext *pContext, const cell_t *params)
{
	IMenuStyle *style = menus->GetDefaultStyle();
	IPluginFunction *pFunction;

	if ((pFunction=pContext->GetFunctionById(params[1])) == NULL)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[1]);
	}

	CMenuHandler *handler = g_MenuHelpers.GetMenuHandler(pFunction, params[2]);
	IBaseMenu *menu = style->CreateMenu(handler, pContext->GetIdentity());

	Handle_t hndl = menu->GetHandle();
	if (!hndl)
	{
		menu->Destroy();
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t DisplayMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->Display(params[2], params[3]) ? 1 : 0;
}

static cell_t DisplayMenuAtItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->DisplayAtItem(params[2], params[4], params[3]) ? 1 : 0;
}

static cell_t VoteMenu(IPluginContext *pContext, const cell_t *params)
{
	if (menus->IsVoteInProgress())
	{
		return pContext->ThrowNativeError("A vote is already in progress");
	}

	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	cell_t flags = 0;
	if (params[0] >= 5)
	{
		flags = params[5];
	}

	if (!menus->StartVote(menu, params[3], addr, params[4], flags))
	{
		return 0;
	}

	return 1;
}

static cell_t AddMenuItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char *info;
	ItemDrawInfo dr;

	pContext->LocalToString(params[2], &info);
	pContext->LocalToString(params[3], (char **)&dr.display);
	dr.style = params[4];

	return menu->AppendItem(info, dr) ? 1 : 0;
}

static cell_t InsertMenuItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char *info;
	ItemDrawInfo dr;

	pContext->LocalToString(params[3], &info);
	pContext->LocalToString(params[4], (char **)&dr.display);
	dr.style = params[5];

	return menu->InsertItem(params[2], info, dr) ? 1 : 0;
}

static cell_t RemoveMenuItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->RemoveItem(params[2]) ? 1 : 0;
}

static cell_t RemoveAllMenuItems(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	menu->RemoveAllItems();

	return 1;
}

static cell_t GetMenuItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	ItemDrawInfo dr;
	const char *info;
	cell_t client = (params[0] >= 8) ? params[8] : 0;
	if(!client && menu->IsPerClientShuffled())
	{
		return pContext->ThrowNativeError("This menu has been per-client random shuffled. "
										  "You have to call GetMenuItem with a client index!");
	}

	if ((info=menu->GetItemInfo(params[2], &dr, client)) == NULL)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[3], params[4], info, NULL);
	pContext->StringToLocalUTF8(params[6], params[7], dr.display ? dr.display : "", NULL);

	cell_t *addr;
	pContext->LocalToPhysAddr(params[5], &addr);
	*addr = dr.style;

	return 1;
}

static cell_t MenuShufflePerClient(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	int start = params[2];
	int stop = params[3];

	if (stop > 0 && !(stop >= start))
	{
		return pContext->ThrowNativeError("Stop must be -1 or >= start!");
	}

	menu->ShufflePerClient(start, stop);

	return 1;
}

static cell_t MenuSetClientMapping(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	int client = params[2];
	if (client < 1 || client > SM_MAXPLAYERS)
	{
		return pContext->ThrowNativeError("Invalid client index!");
	}

	cell_t *array;
	pContext->LocalToPhysAddr(params[3], &array);

	int length = params[4];

	menu->SetClientMapping(client, array, length);

	return 1;
}

static cell_t SetMenuPagination(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->SetPagination(params[2]) ? 1 : 0;
}

static cell_t GetMenuPagination(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->GetPagination();
}

static cell_t GetMenuItemCount(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->GetItemCount();
}

static cell_t SetMenuTitle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char buffer[1024];
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);

	menu->SetDefaultTitle(buffer);

	return 1;
}

static cell_t GetMenuTitle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	size_t written;
	const char *title = menu->GetDefaultTitle();
	pContext->StringToLocalUTF8(params[2], params[3], title, &written);

	return (cell_t)written;
}


static cell_t CreatePanelFromMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	IMenuPanel *panel = menu->CreatePanel();
	hndl = MakePanelHandle(panel, pContext);
	if (!hndl)
	{
		panel->DeleteThis();
	}

	return hndl;
}

static cell_t GetMenuExitButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return ((menu->GetMenuOptionFlags() & MENUFLAG_BUTTON_EXIT) == MENUFLAG_BUTTON_EXIT) ? 1 : 0;
}

static cell_t GetMenuExitBackButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return ((menu->GetMenuOptionFlags() & MENUFLAG_BUTTON_EXITBACK) == MENUFLAG_BUTTON_EXITBACK) ? 1 : 0;
}

static cell_t SetMenuExitButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	unsigned int flags = menu->GetMenuOptionFlags();

	if (params[2])
	{
		flags |= MENUFLAG_BUTTON_EXIT;
	} else {
		flags &= ~MENUFLAG_BUTTON_EXIT;
	}

	menu->SetMenuOptionFlags(flags);
	unsigned int new_flags = menu->GetMenuOptionFlags();

	return (flags == new_flags);
}

static cell_t SetMenuNoVoteButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	unsigned int flags = menu->GetMenuOptionFlags();

	if (params[2])
	{
		flags |= MENUFLAG_BUTTON_NOVOTE;
	} else {
		flags &= ~MENUFLAG_BUTTON_NOVOTE;
	}

	menu->SetMenuOptionFlags(flags);
	unsigned int new_flags = menu->GetMenuOptionFlags();

	return (flags == new_flags);
}

static cell_t SetMenuExitBackButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	unsigned int flags = menu->GetMenuOptionFlags();

	if (params[2])
	{
		flags |= MENUFLAG_BUTTON_EXITBACK;
	} else {
		flags &= ~MENUFLAG_BUTTON_EXITBACK;
	}

	menu->SetMenuOptionFlags(flags);

	return 1;
}

static cell_t CancelMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	menus->CancelMenu(menu);

	return 1;
}

static cell_t IsVoteInProgress(IPluginContext *pContext, const cell_t *params)
{
	return menus->IsVoteInProgress() ? 1 : 0;
}

static cell_t GetMenuStyle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->GetDrawStyle()->GetHandle();
}

static cell_t GetMenuStyleHandle(IPluginContext *pContext, const cell_t *params)
{
	IMenuStyle *style = GetStyleFromCell(params[1]);
	if (!style)
	{
		return BAD_HANDLE;
	}

	return style->GetHandle();
}

static cell_t CreatePanel(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuStyle *style;

	if (hndl != 0)
	{
		if ((err = ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = menus->GetDefaultStyle();
	}

	IMenuPanel *panel = style->CreatePanel();

	hndl = MakePanelHandle(panel, pContext);
	if (!hndl)
	{
		panel->DeleteThis();
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t CreateMenuEx(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuStyle *style;

	if (hndl != 0)
	{
		if ((err = ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = menus->GetDefaultStyle();
	}

	IPluginFunction *pFunction;
	if ((pFunction=pContext->GetFunctionById(params[2])) == NULL)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[2]);
	}

	CMenuHandler *handler = g_MenuHelpers.GetMenuHandler(pFunction, params[3]);

	IBaseMenu *pMenu = style->CreateMenu(handler, pContext->GetIdentity());
	hndl = pMenu->GetHandle();
	if (!hndl)
	{
		pMenu->Destroy();
		return BAD_HANDLE;
	}

	return hndl;
}

static cell_t GetClientMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[2];
	HandleError err;
	IMenuStyle *style;

	if (hndl != 0)
	{
		if ((err = ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = menus->GetDefaultStyle();
	}

	return style->GetClientMenu(params[1], NULL);
}

static cell_t CancelClientMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[3];
	HandleError err;
	IMenuStyle *style;

	if (hndl != 0)
	{
		if ((err = ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = menus->GetDefaultStyle();
	}

	return style->CancelClientMenu(params[1], params[2] ? true : false) ? 1 : 0;
}

static cell_t GetMaxPageItems(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuStyle *style;

	if (hndl != 0)
	{
		if ((err = ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = menus->GetDefaultStyle();
	}

	return style->GetMaxPageItems();
}

static cell_t GetPanelStyle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->GetParentStyle()->GetHandle();
}

static cell_t SetPanelTitle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char *text;
	pContext->LocalToString(params[2], &text);

	panel->DrawTitle(text, params[3] ? true : false);

	return 1;
}

static cell_t DrawPanelItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	ItemDrawInfo dr;
	pContext->LocalToString(params[2], (char **)&dr.display);
	dr.style = params[3];

	return panel->DrawItem(dr);
}

static cell_t DrawPanelText(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char *text;
	pContext->LocalToString(params[2], &text);

	return panel->DrawRawLine(text) ? 1 : 0;
}

static cell_t CanPanelDrawFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->CanDrawItem(params[2]);
}

static cell_t SendPanelToClient(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	IPluginFunction *pFunction;
	if ((pFunction=pContext->GetFunctionById(params[3])) == NULL)
	{
		return pContext->ThrowNativeError("Function id %x is invalid", params[3]);
	}

	CPanelHandler *handler = g_MenuHelpers.GetPanelHandler(pFunction);
	if (!panel->SendDisplay(params[2], handler, params[4]))
	{
		g_MenuHelpers.FreePanelHandler(handler);
	}

	return 1;
}

static cell_t SetPanelKeys(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->SetSelectableKeys(params[2]);
}

static cell_t GetPanelCurrentKey(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->GetCurrentKey();
}

static cell_t GetPanelTextRemaining(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->GetAmountRemaining();
}

static cell_t SetPanelCurrentKey(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMenuPanel *panel;

	if ((err=ReadPanelHandle(hndl, &panel)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return panel->SetCurrentKey(params[2]) ? 1 : 0;
}

static cell_t RedrawMenuItem(IPluginContext *pContext, const cell_t *params)
{
	if (!s_pCurPanel)
	{
		return pContext->ThrowNativeError("You can only call this once from a MenuAction_DisplayItem callback");
	}

	char *str;
	pContext->LocalToString(params[1], &str);

	ItemDrawInfo dr = *s_CurDrawInfo;
	dr.display = str;

	if ((s_CurPanelReturn = s_pCurPanel->DrawItem(dr)) != 0)
	{
		s_pCurPanel = NULL;
	}

	return s_CurPanelReturn;
}

static cell_t GetMenuOptionFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->GetMenuOptionFlags();
}

static cell_t SetMenuOptionFlags(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	menu->SetMenuOptionFlags(params[2]);

	return 1;
}

static cell_t CancelVote(IPluginContext *pContxt, const cell_t *params)
{
	if (!menus->IsVoteInProgress())
	{
		return pContxt->ThrowNativeError("No vote is in progress");
	}

	menus->CancelVoting();

	return 1;
}

static cell_t SetVoteResultCallback(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err = ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	IPluginFunction *pFunction = pContext->GetFunctionById(params[2]);
	if (!pFunction)
	{
		return pContext->ThrowNativeError("Invalid function %x", params[2]);
	}

	void *array[2];
	array[0] = pFunction;
	array[1] = (void *)&params[2];

	IMenuHandler *pHandler = menu->GetHandler();
	if (!pHandler->OnSetHandlerOption("set_vote_results_handler", (const void *)array))
	{
		return pContext->ThrowNativeError("The given menu does not support this option");
	}

	return 1;
}

static cell_t CheckVoteDelay(IPluginContext *pContext, const cell_t *params)
{
	return menus->GetRemainingVoteDelay();
}

static cell_t GetMenuSelectionPosition(IPluginContext *pContext, const cell_t *params)
{
	if (!s_CurSelectPosition)
	{
		return pContext->ThrowNativeError("Can only be called from inside a MenuAction_Select callback");
	}

	return *s_CurSelectPosition;
}

static cell_t IsClientInVotePool(IPluginContext *pContext, const cell_t *params)
{
	int client;
	IGamePlayer *pPlayer;

	client = params[1];
	if ((pPlayer = playerhelpers->GetGamePlayer(client)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	if (!menus->IsVoteInProgress())
	{
		return pContext->ThrowNativeError("No vote is in progress");
	}

	return menus->IsClientInVotePool(client) ? 1 : 0;
}

static cell_t RedrawClientVoteMenu(IPluginContext *pContext, const cell_t *params)
{
	int client;
	IGamePlayer *pPlayer;

	client = params[1];
	if ((pPlayer = playerhelpers->GetGamePlayer(client)) == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	if (!menus->IsVoteInProgress())
	{
		return pContext->ThrowNativeError("No vote is in progress");
	}

	if (!menus->IsClientInVotePool(client))
	{
		return pContext->ThrowNativeError("Client is not in the voting pool");
	}

	bool revote = true;
	if (params[0] >= 2 && !params[2])
	{
		revote = false;
	}

	return menus->RedrawClientVoteMenu2(client, revote) ? 1 : 0;
}

class EmptyMenuHandler : public IMenuHandler
{
public:
} s_EmptyMenuHandler;

static cell_t InternalShowMenu(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);

	if (pPlayer == NULL)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	else if (!pPlayer->IsInGame())
	{
		return pContext->ThrowNativeError("Client %d is not in game", client);
	}

	if (!RadioMenuStyle().IsSupported())
	{
		return pContext->ThrowNativeError("Radio menus are not supported on this mod");
	}

	char *str;
	pContext->LocalToString(params[2], &str);

	IMenuPanel *pPanel = RadioMenuStyle().CreatePanel();

	if (pPanel == NULL)
	{
		return 0;
	}

	pPanel->DirectSet(str);
	pPanel->SetSelectableKeys(params[4]);

	IMenuHandler *pHandler;
	CPanelHandler *pActualHandler = NULL;
	if (params[5] != -1)
	{
		IPluginFunction *pFunction = pContext->GetFunctionById(params[5]);
		if (pFunction == NULL)
		{
			return pContext->ThrowNativeError("Invalid function index %x", params[5]);
		}
		pActualHandler = g_MenuHelpers.GetPanelHandler(pFunction);
	}

	if (pActualHandler == NULL)
	{
		pHandler = &s_EmptyMenuHandler;
	}
	else
	{
		pHandler = pActualHandler;
	}

	bool bSuccess = pPanel->SendDisplay(client, pHandler, params[3]);

	pPanel->DeleteThis();

	if (!bSuccess && pActualHandler != NULL)
	{
		g_MenuHelpers.FreePanelHandler(pActualHandler);
	}

	return bSuccess ? 1 : 0;
}

REGISTER_NATIVES(menuNatives)
{
	{"AddMenuItem",				AddMenuItem},
	{"CanPanelDrawFlags",		CanPanelDrawFlags},
	{"CancelClientMenu",		CancelClientMenu},
	{"CancelMenu",				CancelMenu},
	{"CancelVote",				CancelVote},
	{"CheckVoteDelay",			CheckVoteDelay},
	{"CreateMenu",				CreateMenu},
	{"CreateMenuEx",			CreateMenuEx},
	{"CreatePanel",				CreatePanel},
	{"CreatePanelFromMenu",		CreatePanelFromMenu},
	{"DisplayMenu",				DisplayMenu},
	{"DisplayMenuAtItem",		DisplayMenuAtItem},
	{"DrawPanelItem",			DrawPanelItem},
	{"DrawPanelText",			DrawPanelText},
	{"GetClientMenu",			GetClientMenu},
	{"GetMaxPageItems",			GetMaxPageItems},
	{"GetMenuExitBackButton",	GetMenuExitBackButton},
	{"GetMenuExitButton",		GetMenuExitButton},
	{"GetMenuItem",				GetMenuItem},
	{"GetMenuItemCount",		GetMenuItemCount},
	{"GetMenuOptionFlags",		GetMenuOptionFlags},
	{"GetMenuPagination",		GetMenuPagination},
	{"GetMenuSelectionPosition",GetMenuSelectionPosition},
	{"GetMenuStyle",			GetMenuStyle},
	{"GetMenuStyleHandle",		GetMenuStyleHandle},
	{"GetMenuTitle",			GetMenuTitle},
	{"GetPanelTextRemaining",	GetPanelTextRemaining},
	{"GetPanelCurrentKey",		GetPanelCurrentKey},
	{"GetPanelStyle",			GetPanelStyle},
	{"InsertMenuItem",			InsertMenuItem},
	{"InternalShowMenu",		InternalShowMenu},
	{"IsClientInVotePool",		IsClientInVotePool},
	{"IsVoteInProgress",		IsVoteInProgress},
	{"RedrawClientVoteMenu",	RedrawClientVoteMenu},
	{"RedrawMenuItem",			RedrawMenuItem},
	{"RemoveAllMenuItems",		RemoveAllMenuItems},
	{"RemoveMenuItem",			RemoveMenuItem},
	{"SendPanelToClient",		SendPanelToClient},
	{"SetMenuExitBackButton",	SetMenuExitBackButton},
	{"SetMenuExitButton",		SetMenuExitButton},
	{"SetMenuOptionFlags",		SetMenuOptionFlags},
	{"SetMenuPagination",		SetMenuPagination},
	{"SetMenuTitle",			SetMenuTitle},
	{"SetPanelCurrentKey",		SetPanelCurrentKey},
	{"SetPanelTitle",			SetPanelTitle},
	{"SetPanelKeys",			SetPanelKeys},
	{"SetVoteResultCallback",	SetVoteResultCallback},
	{"VoteMenu",				VoteMenu},
	{"MenuShufflePerClient",	MenuShufflePerClient},
	{"MenuSetClientMapping",	MenuSetClientMapping},
	{"SetMenuNoVoteButton",		SetMenuNoVoteButton},

	// Transitional syntax support.
	{"Panel.Panel",				CreatePanel},
	{"Panel.TextRemaining.get",	GetPanelTextRemaining},
	{"Panel.CurrentKey.get",	GetPanelCurrentKey},
	{"Panel.CurrentKey.set",	SetPanelCurrentKey},
	{"Panel.Style.get",			GetPanelStyle},
	{"Panel.CanDrawFlags",		CanPanelDrawFlags},
	{"Panel.SetTitle",			SetPanelTitle},
	{"Panel.SetKeys",			SetPanelKeys},
	{"Panel.Send",				SendPanelToClient},
	{"Panel.DrawItem",			DrawPanelItem},
	{"Panel.DrawText",			DrawPanelText},

	{"Menu.Menu",				CreateMenu},
	{"Menu.Display",			DisplayMenu},
	{"Menu.DisplayAt",			DisplayMenuAtItem},
	{"Menu.AddItem",			AddMenuItem},
	{"Menu.InsertItem",			InsertMenuItem},
	{"Menu.RemoveItem",			RemoveMenuItem},
	{"Menu.RemoveAllItems",		RemoveAllMenuItems},
	{"Menu.GetItem",			GetMenuItem},
	{"Menu.GetTitle",			GetMenuTitle},
	{"Menu.SetTitle",			SetMenuTitle},
	{"Menu.ToPanel",			CreatePanelFromMenu},
	{"Menu.Cancel",				CancelMenu},
	{"Menu.DisplayVote",		VoteMenu},
	{"Menu.ShufflePerClient",	MenuShufflePerClient},
	{"Menu.SetClientMapping",	MenuSetClientMapping},
	{"Menu.Pagination.get",		GetMenuPagination},
	{"Menu.Pagination.set",		SetMenuPagination},
	{"Menu.OptionFlags.get",	GetMenuOptionFlags},
	{"Menu.OptionFlags.set",	SetMenuOptionFlags},
	{"Menu.ExitButton.get",		GetMenuExitButton},
	{"Menu.ExitButton.set",		SetMenuExitButton},
	{"Menu.ExitBackButton.get",	GetMenuExitBackButton},
	{"Menu.ExitBackButton.set",	SetMenuExitBackButton},
	{"Menu.NoVoteButton.set",	SetMenuNoVoteButton},
	{"Menu.VoteResultCallback.set", SetVoteResultCallback},
	{"Menu.ItemCount.get",		GetMenuItemCount},
	{"Menu.Style.get",			GetMenuStyle},
	{"Menu.Selection.get",		GetMenuSelectionPosition},

	{NULL,						NULL},
};

