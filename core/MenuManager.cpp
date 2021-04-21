/**
 * vim: set ts=4 :
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

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include "MenuManager.h"
#include "MenuVoting.h"
#include "sm_stringutil.h"
#include "sourcemm_api.h"
#include "PlayerManager.h"
#include "MenuStyle_Valve.h"
#include "sourcemm_api.h"
#include "logic_bridge.h"

MenuManager g_Menus;
VoteMenuHandler s_VoteHandler;

ConVar sm_menu_sounds("sm_menu_sounds", "1", 0, "Sets whether SourceMod menus play trigger sounds");

MenuManager::MenuManager()
{
	m_Styles.push_back(&g_ValveMenuStyle);
	SetDefaultStyle(&g_ValveMenuStyle);
}

void MenuManager::OnSourceModAllInitialized()
{
	sharesys->AddInterface(NULL, this);

	HandleAccess access;
	handlesys->InitAccessDefaults(NULL, &access);

	/* Deny cloning to menus */
	access.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;
	m_MenuType = handlesys->CreateType("IBaseMenu", this, 0, NULL, &access, g_pCoreIdent, NULL);

	/* Also deny deletion to styles */
	access.access[HandleAccess_Delete] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;
	m_StyleType = handlesys->CreateType("IMenuStyle", this, 0, NULL, &access, g_pCoreIdent, NULL);
}

void MenuManager::OnSourceModAllShutdown()
{
	handlesys->RemoveType(m_MenuType, g_pCoreIdent);
	handlesys->RemoveType(m_StyleType, g_pCoreIdent);
}

void MenuManager::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == m_MenuType)
	{
		IBaseMenu *menu = (IBaseMenu *)object;
		menu->Destroy(false);
	}
	else if (type == m_StyleType)
	{
		/* Do nothing */
	}
}

bool MenuManager::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	if (type == m_MenuType)
	{
		*pSize = ((IBaseMenu *)object)->GetApproxMemUsage();
		return true;
	}
	else
	{
		*pSize = ((IMenuStyle *)object)->GetApproxMemUsage();
		return true;
	}

	return false;
}

Handle_t MenuManager::CreateMenuHandle(IBaseMenu *menu, IdentityToken_t *pOwner)
{
	if (m_MenuType == NO_HANDLE_TYPE)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(m_MenuType, menu, pOwner, g_pCoreIdent, NULL);
}

Handle_t MenuManager::CreateStyleHandle(IMenuStyle *style)
{
	if (m_StyleType == NO_HANDLE_TYPE)
	{
		return BAD_HANDLE;
	}

	return handlesys->CreateHandle(m_StyleType, style, g_pCoreIdent, g_pCoreIdent, NULL);
}

HandleError MenuManager::ReadMenuHandle(Handle_t handle, IBaseMenu **menu)
{
	HandleSecurity sec;

	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = NULL;

	return handlesys->ReadHandle(handle, m_MenuType, &sec, (void **)menu);
}

HandleError MenuManager::ReadStyleHandle(Handle_t handle, IMenuStyle **style)
{
	HandleSecurity sec;

	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = g_pCoreIdent;

	return handlesys->ReadHandle(handle, m_StyleType, &sec, (void **)style);
}

bool MenuManager::SetDefaultStyle(IMenuStyle *style)
{
	if (!style)
	{
		return false;
	}

	m_pDefaultStyle = style;

	return true;
}

IMenuStyle *MenuManager::GetStyle(unsigned int index)
{
	if (index >= GetStyleCount())
	{
		return NULL;
	}

	return m_Styles[index];
}

void MenuManager::AddStyle(IMenuStyle *style)
{
	m_Styles.push_back(style);
}

unsigned int MenuManager::GetStyleCount()
{
	return (unsigned int)m_Styles.size();
}

IMenuStyle *MenuManager::FindStyleByName(const char *name)
{
	unsigned int count = GetStyleCount();
	for (unsigned int i=0; i<count; i++)
	{
		IMenuStyle *ptr = GetStyle(i);
		if (strcasecmp(ptr->GetStyleName(), name) == 0)
		{
			return ptr;
		}
	}

	return NULL;
}

inline bool IsSlotItem(IMenuPanel *display,
					   unsigned int style)
{
	if (!display->CanDrawItem(style))
	{
	return false;
	}
	if ((style & ITEMDRAW_IGNORE) == ITEMDRAW_IGNORE)
	{
		return false;
	}
	if (style & ITEMDRAW_RAWLINE)
	{
		return false;
	}

	return true;
}

IMenuPanel *MenuManager::RenderMenu(int client, menu_states_t &md, ItemOrder order)
{
	IBaseMenu *menu = md.menu;

	if (!menu)
	{
		return NULL;
	}

	struct
	{
		unsigned int position;
		ItemDrawInfo draw;
	} drawItems[10];

	/* Figure out how many items to draw */
	IMenuStyle *style = menu->GetDrawStyle();
	unsigned int pgn = menu->GetPagination();
	unsigned int maxItems = style->GetMaxPageItems();
	bool exitButton = (menu->GetMenuOptionFlags() & MENUFLAG_BUTTON_EXIT) == MENUFLAG_BUTTON_EXIT;
	bool novoteButton = (menu->GetMenuOptionFlags() & MENUFLAG_BUTTON_NOVOTE) == MENUFLAG_BUTTON_NOVOTE;

	if (pgn != MENU_NO_PAGINATION)
	{
		maxItems = pgn;
	}
	else if (exitButton)
	{
		maxItems--;
	}

	if (novoteButton)
	{
		maxItems--;
	}

	/* This is very not allowed! */
	if (maxItems < 2)
	{
		return NULL;
	}

	unsigned int totalItems = menu->GetItemCount();
	unsigned int startItem = 0;

	/* For pagination, find the starting point. */
	if (pgn != MENU_NO_PAGINATION)
	{
		if (order == ItemOrder_Ascending)
		{
			startItem = md.lastItem;
			/* This shouldn't happen with well-coded menus.
			 * If the item is out of bounds, switch the order to
			 * Items_Descending and make us start from the top.
			 */
			if (startItem >= totalItems)
			{
				startItem = totalItems - 1;
				order = ItemOrder_Descending;
			}
		}
		else if (order == ItemOrder_Descending)
		{
			startItem = md.firstItem;
			/* This shouldn't happen with well-coded menus.
			 * If searching backwards doesn't give us enough room,
			 * start from the beginning and change to ascending.
			 */
			if (startItem <= maxItems)
			{
				startItem = 0;
				order = ItemOrder_Ascending;
			}
		}
	}

	/* Get our Display pointer and initialize some crap */
	IMenuPanel *panel = menu->CreatePanel();
	IMenuHandler *mh = md.mh;
	bool foundExtra = false;
	unsigned int extraItem = 0;

	if (panel == NULL)
	{
		return NULL;
	}

	/**
	 * We keep searching until:
	 * 1) There are no more items
	 * 2) We reach one OVER the maximum number of slot items
	 * 3) We have reached maxItems and pagination is MENU_NO_PAGINATION
	 */
	unsigned int i = startItem;
	unsigned int foundItems = 0;
	while (totalItems)
	{
		ItemDrawInfo &dr = drawItems[foundItems].draw;
		/* Is the item valid? */
		if (menu->GetItemInfo(i, &dr, client) != NULL)
		{
			/* Ask the user to change the style, if necessary */
			mh->OnMenuDrawItem(menu, client, i, dr.style);
			/* Check if it's renderable */
			if (IsSlotItem(panel, dr.style))
			{
				/* If we've already found the max number of items,
				 * This means we should just cancel out and log our
				 * "last item."
				 */
				if (foundItems >= maxItems)
				{
					foundExtra = true;
					extraItem = i;
					break;
				}
				drawItems[foundItems++].position = i;
			}
		}
		/* If there's no pagination, stop once the menu is full. */
		if (pgn == MENU_NO_PAGINATION)
		{
			/* If we've filled up, then stop */
			if (foundItems >= maxItems)
			{
				break;
			}
		}
		/* If we're descending and this is the first item, stop */
		if (order == ItemOrder_Descending)
		{
			if (i == 0)
			{
				break;
			}
			i--;
		} 
		/* If we're ascending and this is the last item, stop */
		else if (order == ItemOrder_Ascending)
		{
			if (i >= totalItems - 1)
			{
				break;
			}
			i++;
		}
	}

	/* There were no items to draw! */
	if (!foundItems)
	{
		panel->DeleteThis();
		return NULL;
	}

	bool displayPrev = false;
	bool displayNext = false;

	/* This is an annoying process.
	 * Skip it for non-paginated menus, which get special treatment.
	 */
	if (pgn != MENU_NO_PAGINATION)
	{
		if (foundExtra)
		{
			if (order == ItemOrder_Descending)
			{
				displayPrev = true;
				md.firstItem = extraItem;
			}
			else if (order == ItemOrder_Ascending)
			{
				displayNext = true;
				md.lastItem = extraItem;
			}
		}

		unsigned int lastItem = 0;
		ItemDrawInfo dr;
		/* Find the last feasible item to search from. */
		if (order == ItemOrder_Descending)
		{
			lastItem = drawItems[0].position;
			if (lastItem >= totalItems - 1)
			{
				goto skip_search;
			}
			while (++lastItem < totalItems)
			{
				if (menu->GetItemInfo(lastItem, &dr, client) != NULL)
				{
					mh->OnMenuDrawItem(menu, client, lastItem, dr.style);
					if (IsSlotItem(panel, dr.style))
					{
						displayNext = true;
						md.lastItem = lastItem;
						break;
					}
				}
			}
		}
		else if (order == ItemOrder_Ascending)
		{
			lastItem = drawItems[0].position;
			if (lastItem == 0)
			{
				goto skip_search;
			}
			lastItem--;
			while (lastItem != 0)
			{
				if (menu->GetItemInfo(lastItem, &dr, client) != NULL)
				{
					mh->OnMenuDrawItem(menu, client, lastItem, dr.style);
					if (IsSlotItem(panel, dr.style))
					{
						displayPrev = true;
						md.firstItem = lastItem;
						break;
					}
				}
				lastItem--;
			}
		}
	}

skip_search:

	/* Draw the item according to the order */
	menu_slots_t *slots = md.slots;
	unsigned int position = 0;			/* Keep track of the last position */

	if (novoteButton)
	{
		char text[50];
		if (!logicore.CoreTranslate(text, sizeof(text), "%T", 2, NULL, "No Vote", &client))
		{
			ke::SafeStrcpy(text, sizeof(text), "No Vote");
		}
		ItemDrawInfo dr(text, 0);
		position = panel->DrawItem(dr);
		slots[position].type = ItemSel_Exit;
		position++;
	}

	if (order == ItemOrder_Ascending)
	{
		md.item_on_page = drawItems[0].position;
		for (unsigned int i = 0; i < foundItems; i++)
		{
			ItemDrawInfo &dr = drawItems[i].draw;
			if ((position = mh->OnMenuDisplayItem(menu, client, panel, drawItems[i].position, dr)) == 0)
			{
				position = panel->DrawItem(dr);
			}
			if (position != 0)
			{
				slots[position].item = drawItems[i].position;
				if ((dr.style & ITEMDRAW_DISABLED) == ITEMDRAW_DISABLED)
				{
					slots[position].type = ItemSel_None;
				}
				else
				{
					slots[position].type = ItemSel_Item;
				}
			}
		}
	}
	else if (order == ItemOrder_Descending)
	{
		unsigned int i = foundItems;
		/* NOTE: There will always be at least one item because
		 * of the check earlier.
		 */
		md.item_on_page = drawItems[foundItems - 1].position;
		while (i--)
		{
			ItemDrawInfo &dr = drawItems[i].draw;
			if ((position = mh->OnMenuDisplayItem(menu, client, panel, drawItems[i].position, dr)) == 0)
			{
				position = panel->DrawItem(dr);
			}
			if (position != 0)
			{
				slots[position].item = drawItems[i].position;
				if ((dr.style & ITEMDRAW_DISABLED) == ITEMDRAW_DISABLED)
				{
					slots[position].type = ItemSel_None;
				}
				else
				{
					slots[position].type = ItemSel_Item;
				}
			}
		}
	}

	/* Now, we need to check if we need to add anything extra */
	if (pgn != MENU_NO_PAGINATION || exitButton)
	{
		bool canDrawDisabled = panel->CanDrawItem(ITEMDRAW_DISABLED|ITEMDRAW_CONTROL);
		bool exitBackButton = false;
		char text[50];

		if (pgn != MENU_NO_PAGINATION
			&& (menu->GetMenuOptionFlags() & MENUFLAG_BUTTON_EXITBACK) == MENUFLAG_BUTTON_EXITBACK)
		{
			exitBackButton = true;
		}

		/* Calculate how many items we are allowed for control stuff */
		unsigned int padding = style->GetMaxPageItems() - maxItems;
		
		/* Add the number of available slots */
		padding += (maxItems - foundItems);

		/* Someday, if we are able to re-enable this, we will be very lucky men. */
#if 0
		if (!style->FeatureExists(MenuStyleFeature_ImplicitExit))
		{
#endif
		/* Even if we don't draw an exit button, we invalidate the slot. */
		padding--;
#if 0
		} else {
			/* Otherwise, we don't draw anything and leave the slot available */
			exitButton = false;
		}
#endif

		if (pgn != MENU_NO_PAGINATION)
		{
			/* Subtract two slots for the displayNext/displayPrev padding */
			padding -= 2;
		}

		/* If we have an "Exit Back" button and the space to draw it, do so. */
		if (exitBackButton)
		{
			if (!displayPrev)
			{
				displayPrev = true;
			}
			else
			{
				exitBackButton = false;
			}
		}

		/**
		 * We allow next/prev to be undrawn if neither exists.
		 * Thus, we only need padding if one of them will be drawn,
		 * or the exit button will be drawn.
		 */
		ItemDrawInfo padItem(NULL, ITEMDRAW_SPACER);
		if (exitButton || (displayNext || displayPrev))
		{
			/* If there are no control options,
			 * Instead just pad with invisible slots.
			 */
			if (!displayNext && !displayPrev)
			{
				padItem.style = ITEMDRAW_NOTEXT;
			}
			/* Add spacers so we can pad to the end */
			for (unsigned int i=0; i<padding; i++)
			{
				position = panel->DrawItem(padItem);
				slots[position].type = ItemSel_None;
			}
		}

		/* Put a fake spacer before control stuff, if possible */
		if ((displayPrev || displayNext) || exitButton)
		{
			ItemDrawInfo draw("", ITEMDRAW_RAWLINE|ITEMDRAW_SPACER);
			panel->DrawItem(draw);
		}

		ItemDrawInfo dr(text, 0);

		/**
		 * If we have one or the other, we need to have spacers for both.
		 */
		if (pgn != MENU_NO_PAGINATION)
		{
			if (displayPrev || displayNext)
			{
				/* PREVIOUS */
				ItemDrawInfo padCtrlItem(NULL, ITEMDRAW_SPACER|ITEMDRAW_CONTROL);
				if (displayPrev || canDrawDisabled)
				{
					if (exitBackButton)
					{
						if (!logicore.CoreTranslate(text, sizeof(text), "%T", 2, NULL, "Back", &client))
						{
							ke::SafeStrcpy(text, sizeof(text), "Back");
						}
						dr.style = ITEMDRAW_CONTROL;
						position = panel->DrawItem(dr);
						slots[position].type = ItemSel_ExitBack;
					}
					else
					{
						if (!logicore.CoreTranslate(text, sizeof(text), "%T", 2, NULL, "Previous", &client))
						{
							ke::SafeStrcpy(text, sizeof(text), "Previous");
						}
						dr.style = (displayPrev ? 0 : ITEMDRAW_DISABLED)|ITEMDRAW_CONTROL;
						position = panel->DrawItem(dr);
						slots[position].type = ItemSel_Back;
					}
				}
				else if (displayNext || exitButton)
				{
					/* If we can't display this, and there is an exit button,
					 * we need to pad!
					 */
					position = panel->DrawItem(padCtrlItem);
					slots[position].type = ItemSel_None;
				}

				/* NEXT */
				if (displayNext || canDrawDisabled)
				{
					if (!logicore.CoreTranslate(text, sizeof(text), "%T", 2, NULL, "Next", &client))
					{
						ke::SafeStrcpy(text, sizeof(text), "Next");
					}
					dr.style = (displayNext ? 0 : ITEMDRAW_DISABLED)|ITEMDRAW_CONTROL;
					position = panel->DrawItem(dr);
					slots[position].type = ItemSel_Next;
				}
				else if (exitButton)
				{
					/* If we can't display this,
					 * but there is an "exit" button, we need to pad!
					 */
					position = panel->DrawItem(padCtrlItem);
					slots[position].type = ItemSel_None;
				}
			}
			else
			{
				/* Otherwise, bump to two slots! */
				ItemDrawInfo numBump(NULL, ITEMDRAW_NOTEXT);
				position = panel->DrawItem(numBump);
				slots[position].type = ItemSel_None;
				position = panel->DrawItem(numBump);
				slots[position].type = ItemSel_None;
			}
		}

		/* EXIT */
		if (exitButton)
		{
			if (!logicore.CoreTranslate(text, sizeof(text), "%T", 2, NULL, "Exit", &client))
			{
				ke::SafeStrcpy(text, sizeof(text), "Exit");
			}
			dr.style = ITEMDRAW_CONTROL;
			position = panel->DrawItem(dr);
			slots[position].type = ItemSel_Exit;
		}
	}

	/* Lastly, fill in any slots we could have missed */
	for (unsigned int i = position + 1; i < 10; i++)
	{
		slots[i].type = ItemSel_None;
	}

	/* Do title stuff */
	mh->OnMenuDisplay(menu, client, panel);
	panel->DrawTitle(menu->GetDefaultTitle(), true);

	return panel;
}

IMenuStyle *MenuManager::GetDefaultStyle()
{
	return m_pDefaultStyle;
}

bool MenuManager::MenuSoundsEnabled()
{
	return (sm_menu_sounds.GetInt() != 0);
}

ConfigResult MenuManager::OnSourceModConfigChanged(const char *key,
												   const char *value,
												   ConfigSource source,
												   char *error,
												   size_t maxlength)
{
	if (strcmp(key, "MenuItemSound") == 0)
	{
		m_SelectSound.assign(value);
		return ConfigResult_Accept;
	} else if (strcmp(key, "MenuExitBackSound") == 0) {
		m_ExitBackSound.assign(value);
		return ConfigResult_Accept;
	} else if (strcmp(key, "MenuExitSound") == 0) {
		m_ExitSound.assign(value);
		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

const char *MenuManager::GetMenuSound(ItemSelection sel)
{
	const char *sound = NULL;

	switch (sel)
	{
	case ItemSel_Back:
	case ItemSel_Next:
	case ItemSel_Item:
		{
			if (m_SelectSound.size() > 0)
			{
				sound = m_SelectSound.c_str();
			}
			break;
		}
	case ItemSel_ExitBack:
		{
			if (m_ExitBackSound.size() > 0)
			{
				sound = m_ExitBackSound.c_str();
			}
			break;
		}
	case ItemSel_Exit:
		{
			if (m_ExitSound.size() > 0)
			{
				sound = m_ExitSound.c_str();
			}
			break;
		}
	default:
		{
			break;
		}
	}

	return sound;
}

void MenuManager::OnSourceModLevelChange(const char *mapName)
{
	if (m_SelectSound.size() > 0)
	{
		enginesound->PrecacheSound(m_SelectSound.c_str(), true);
	}
	if (m_ExitBackSound.size() > 0)
	{
		enginesound->PrecacheSound(m_ExitBackSound.c_str(), true);
	}
	if (m_ExitSound.size() > 0)
	{
		enginesound->PrecacheSound(m_ExitSound.c_str(), true);
	}
}

void MenuManager::CancelMenu(IBaseMenu *menu)
{
	if (s_VoteHandler.GetCurrentMenu() == menu
		&& !s_VoteHandler.IsCancelling())
	{
		s_VoteHandler.CancelVoting();
		return;
	}

	menu->Cancel();
}

bool MenuManager::StartVote(IBaseMenu *menu, unsigned int num_clients, int clients[], unsigned int max_time, unsigned int flags)
{
	return s_VoteHandler.StartVote(menu, num_clients, clients, max_time, flags);
}

bool MenuManager::IsVoteInProgress()
{
	return s_VoteHandler.IsVoteInProgress();
}

void MenuManager::CancelVoting()
{
	s_VoteHandler.CancelVoting();
}

unsigned int MenuManager::GetRemainingVoteDelay()
{
	return s_VoteHandler.GetRemainingVoteDelay();
}

bool MenuManager::IsClientInVotePool(int client)
{
	return s_VoteHandler.IsClientInVotePool(client);
}

bool MenuManager::RedrawClientVoteMenu(int client)
{
	return RedrawClientVoteMenu2(client, true);
}

bool MenuManager::RedrawClientVoteMenu2(int client, bool revote)
{
	return s_VoteHandler.RedrawToClient(client, revote);
}

