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

#include "sm_globals.h"
#include <sh_stack.h>
#include "MenuManager.h"
#include "MenuStyle_Valve.h"
#include "MenuStyle_Radio.h"
#include "HandleSys.h"
#include "PluginSys.h"

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
};

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
	void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item);
	void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
	void OnMenuEnd(IBaseMenu *menu);
	void OnMenuDestroy(IBaseMenu *menu);
#if 0
	void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style);
	void OnMenuDisplayItem(IBaseMenu *menu, int client, unsigned int item, const char **display);
#endif
private:
	IPluginFunction *m_pBasic;
	int m_Flags;
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
		m_PanelType = g_HandleSys.CreateType("IMenuPanel", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_PluginSys.AddPluginsListener(this);
	}

	virtual void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(m_PanelType, g_pCoreIdent);

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
		IMenuPanel *panel = (IMenuPanel *)object;
		panel->DeleteThis();
	}

	/**
	 *   It is extremely important that unloaded plugins don't crash.
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
		handler->m_pPlugin = g_PluginSys.GetPluginByCtx(pFunction->GetParentContext()->GetContext());
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
		}
		return handler;
	}

	void FreeMenuHandler(CMenuHandler *handler)
	{
		m_FreeMenuHandlers.push(handler);
	}

private:
	HandleType_t m_PanelType;
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
		m_pFunc->PushCell(BAD_HANDLE);
		m_pFunc->PushCell(MenuAction_Select);
		m_pFunc->PushCell(client);
		m_pFunc->PushCell(item);
		m_pFunc->Execute(NULL);
	}
	g_MenuHelpers.FreePanelHandler(this);
}

/**
 * MENU HANDLER WRAPPER
 */
CMenuHandler::CMenuHandler(IPluginFunction *pBasic, int flags) : 
	m_pBasic(pBasic), m_Flags(flags)
{
	/* :TODO: We can probably cache the handle ahead of time */
}

void CMenuHandler::OnMenuStart(IBaseMenu *menu)
{
	if ((m_Flags & (int)MenuAction_Start) == (int)MenuAction_Start)
	{
		m_pBasic->PushCell(menu->GetHandle());
		m_pBasic->PushCell(MenuAction_Start);
		m_pBasic->PushCell(0);
		m_pBasic->PushCell(0);
		m_pBasic->Execute(NULL);
	}
}

void CMenuHandler::OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *panel)
{
	if ((m_Flags & (int)MenuAction_Display) == (int)MenuAction_Display)
	{
		HandleSecurity sec;
		sec.pIdentity = m_pBasic->GetParentContext()->GetIdentity();
		sec.pOwner = g_pCoreIdent;
	
		HandleAccess access;
		g_HandleSys.InitAccessDefaults(NULL, &access);
		access.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

		Handle_t hndl =  g_HandleSys.CreateHandleEx(g_MenuHelpers.GetPanelType(), panel, &sec, &access, NULL);

		m_pBasic->PushCell(menu->GetHandle());
		m_pBasic->PushCell(MenuAction_Display);
		m_pBasic->PushCell(client);
		m_pBasic->PushCell(hndl);
		m_pBasic->Execute(NULL);

		g_HandleSys.FreeHandle(hndl, &sec);
	}
}

void CMenuHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	m_pBasic->PushCell(menu->GetHandle());
	m_pBasic->PushCell(MenuAction_Select);
	m_pBasic->PushCell(client);
	m_pBasic->PushCell(item);
	m_pBasic->Execute(NULL);
}

void CMenuHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	m_pBasic->PushCell(menu->GetHandle());
	m_pBasic->PushCell(MenuAction_Cancel);
	m_pBasic->PushCell(client);
	m_pBasic->PushCell(reason);
	m_pBasic->Execute(NULL);
}

void CMenuHandler::OnMenuEnd(IBaseMenu *menu)
{
	m_pBasic->PushCell(menu->GetHandle());
	m_pBasic->PushCell(MenuAction_End);
	m_pBasic->PushCell(0);
	m_pBasic->PushCell(0);
	m_pBasic->Execute(NULL);
}

void CMenuHandler::OnMenuDestroy(IBaseMenu *menu)
{
	g_MenuHelpers.FreeMenuHandler(this);
}

/**
 * INLINE FUNCTIONS FOR NATIVES
 */

inline Handle_t MakePanelHandle(IMenuPanel *panel, IPluginContext *pContext)
{
	return g_HandleSys.CreateHandle(g_MenuHelpers.GetPanelType(), panel, pContext->GetIdentity(), g_pCoreIdent, NULL);
}

inline HandleError ReadPanelHandle(Handle_t hndl, IMenuPanel **panel)
{
	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = NULL;
	return g_HandleSys.ReadHandle(hndl, g_MenuHelpers.GetPanelType(), &sec, (void **)panel);
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
		return &g_ValveMenuStyle;
	} else if (cell == MenuStyle_Radio
			   && g_RadioMenuStyle.IsSupported())
	{
		return &g_RadioMenuStyle;
	}

	return g_Menus.GetDefaultStyle();
}

/***********************************
 **** NATIVE DEFINITIONS ***********
 ***********************************/

static cell_t CreateMenu(IPluginContext *pContext, const cell_t *params)
{
	IMenuStyle *style = g_Menus.GetDefaultStyle();
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->Display(params[2], params[3]) ? 1 : 0;
}

static cell_t AddMenuItem(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	ItemDrawInfo dr;
	const char *info;

	if ((info=menu->GetItemInfo(params[2], &dr)) == NULL)
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

static cell_t SetMenuPagination(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	char buffer[1024];
	g_SourceMod.FormatString(buffer, sizeof(buffer), pContext, params, 2);

	menu->SetDefaultTitle(buffer);

	return 1;
}

static cell_t CreatePanelFromMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->GetExitButton() ? 1 : 0;
}

static cell_t SetMenuExitButton(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	return menu->SetExitButton(params[2] ? true : false) ? 1 : 0;
}

static cell_t CancelMenu(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Menu handle %x is invalid (error %d)", hndl, err);
	}

	menu->Cancel();

	return 1;
}

static cell_t GetMenuStyle(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IBaseMenu *menu;

	if ((err=g_Menus.ReadMenuHandle(params[1], &menu)) != HandleError_None)
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
		if ((err=g_Menus.ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = g_Menus.GetDefaultStyle();
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
		if ((err=g_Menus.ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = g_Menus.GetDefaultStyle();
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
		if ((err=g_Menus.ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = g_Menus.GetDefaultStyle();
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
		if ((err=g_Menus.ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = g_Menus.GetDefaultStyle();
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
		if ((err=g_Menus.ReadStyleHandle(params[1], &style)) != HandleError_None)
		{
			return pContext->ThrowNativeError("MenuStyle handle %x is invalid (error %d)", hndl, err);
		}
	} else {
		style = g_Menus.GetDefaultStyle();
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

REGISTER_NATIVES(menuNatives)
{
	{"AddMenuItem",				AddMenuItem},
	{"CanPanelDrawFlags",		CanPanelDrawFlags},
	{"CancelClientMenu",		CancelClientMenu},
	{"CancelMenu",				CancelMenu},
	{"CreateMenu",				CreateMenu},
	{"CreateMenuEx",			CreateMenuEx},
	{"CreatePanel",				CreatePanel},
	{"CreatePanelFromMenu",		CreatePanelFromMenu},
	{"DisplayMenu",				DisplayMenu},
	{"DrawPanelItem",			DrawPanelItem},
	{"DrawPanelText",			DrawPanelText},
	{"GetClientMenu",			GetClientMenu},
	{"GetMaxPageItems",			GetMaxPageItems},
	{"GetMenuExitButton",		GetMenuExitButton},
	{"GetMenuItem",				GetMenuItem},
	{"GetMenuItemCount",		GetMenuItemCount},
	{"GetMenuPagination",		GetMenuPagination},
	{"GetMenuStyle",			GetMenuStyle},
	{"GetMenuStyleHandle",		GetMenuStyleHandle},
	{"GetPanelCurrentKey",		GetPanelCurrentKey},
	{"GetPanelStyle",			GetPanelStyle},
	{"InsertMenuItem",			InsertMenuItem},
	{"RemoveAllMenuItems",		RemoveAllMenuItems},
	{"RemoveMenuItem",			RemoveMenuItem},
	{"SendPanelToClient",		SendPanelToClient},
	{"SetMenuExitButton",		SetMenuExitButton},
	{"SetMenuPagination",		SetMenuPagination},
	{"SetMenuTitle",			SetMenuTitle},
	{"SetPanelCurrentKey",		SetPanelCurrentKey},
	{"SetPanelTitle",			SetPanelTitle},
	{"SetPanelKeys",			SetPanelKeys},
	{NULL,						NULL},
};
