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

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include "MenuManager.h"
#include "sm_stringutil.h"
#include "sourcemm_api.h"
#include "PlayerManager.h"
#include "MenuStyle_Valve.h"
#include "ShareSys.h"
#include "HandleSys.h"

MenuManager g_Menus;

/*******************************
 *******************************
 ******** VOTE HANDLER *********
 *******************************
 *******************************/

unsigned int VoteMenuHandler::GetMenuAPIVersion2()
{
	return m_pHandler->GetMenuAPIVersion2();
}

bool VoteMenuHandler::IsVoteInProgress()
{
	return (m_pCurMenu != NULL);
}

void VoteMenuHandler::InitializeVoting(IBaseMenu *menu)
{
	m_Items = menu->GetItemCount();

	if (m_Votes.size() < (size_t)m_Items)
	{
		/* Only clear the items we need to... */
		size_t size = m_Votes.size();
		for (size_t i=0; i<size; i++)
		{
			m_Votes[i] = 0;
		}
		m_Votes.resize(m_Items, 0);
	} else {
		for (unsigned int i=0; i<m_Items; i++)
		{
			m_Votes[i] = 0;
		}
	}

	m_pCurMenu = menu;

	m_pHandler->OnMenuStart(m_pCurMenu);
}

void VoteMenuHandler::StartVoting()
{
	m_bStarted = true;

	m_pHandler->OnMenuVoteStart(m_pCurMenu);

	/* By now we know how many clients were set.  
	 * If there are none, we should end IMMEDIATELY.
	 */
	if (m_Clients == 0)
	{
		EndVoting();
	}
}

void VoteMenuHandler::DecrementPlayerCount()
{
	assert(m_Clients > 0);

	m_Clients--;

	if (m_bStarted && m_Clients == 0)
	{
		EndVoting();
	}
}

void VoteMenuHandler::EndVoting()
{
	if (m_bCancelled)
	{
		/* If we were cancelled, don't bother tabulating anything.
		 * Reset just in case someone tries to redraw, which means
		 * we need to save our states.
		 */
		IBaseMenu *menu = m_pCurMenu;
		InternalReset();
		m_pHandler->OnMenuVoteCancel(menu);
		m_pHandler->OnMenuEnd(menu, MenuEnd_VotingCancelled);
		return;
	}

	unsigned int chosen = 0;
	unsigned int highest = 0;
	unsigned int dup_count = 0;
	unsigned int total = m_Votes.size() ? m_Votes[0] : 0;

	/* If we got zero votes, take a shortcut. */
	if (m_NumVotes == 0)
	{
		/* Pick a random item and then jump far, far away. */
		srand((unsigned int)(time(NULL)));
		chosen = (unsigned int)rand() % m_Items;
		goto picked_item;
	}

	/* We can't have more dups than this!
	 * This is the max number of players.
	 */
	unsigned int dup_array[256];

	for (size_t i=1; i<m_Items; i++)
	{
		if (m_Votes[i] > m_Votes[highest])
		{
			/* If we have a new highest count, mark it and trash the duplicate
			 * list by setting the total to 0.
			 */
			highest = i;
			dup_count = 0;
		} else if (m_Votes[i] == m_Votes[highest]) {
			/* If they're equal, mark it in the duplicate list.
			 * We'll add in the original later.
			 */
			dup_array[dup_count++] = i;
		}
		total += m_Votes[i];
	}

	/* Check if we need to pick from the duplicate list */
	if (dup_count)
	{
		/* Re-add the original to the list because it's not in there. */
		dup_array[dup_count++] = highest;

		/* Pick a random slot. */
		srand((unsigned int)(time(NULL)));
		unsigned int r = (unsigned int)rand() % dup_count;

		/* Pick the item. */
		chosen = dup_array[r];
	} else {
		chosen = highest;
	}

picked_item:
	m_pHandler->OnMenuVoteEnd(m_pCurMenu, chosen, m_Votes[highest], total);
	m_pHandler->OnMenuEnd(m_pCurMenu, MenuEnd_VotingDone);
	InternalReset();
}

void VoteMenuHandler::OnMenuStart(IBaseMenu *menu)
{
	m_Clients++;
}

void VoteMenuHandler::OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
{
	DecrementPlayerCount();
}

void VoteMenuHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	m_pHandler->OnMenuCancel(menu, client, reason);
}

void VoteMenuHandler::OnMenuDisplay(IBaseMenu *menu, int client, IMenuPanel *display)
{
	m_pHandler->OnMenuDisplay(menu, client, display);
}

void VoteMenuHandler::OnMenuDisplayItem(IBaseMenu *menu, int client, unsigned int item, const char **display)
{
	m_pHandler->OnMenuDisplayItem(menu, client, item, display);
}

void VoteMenuHandler::OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style)
{
	m_pHandler->OnMenuDrawItem(menu, client, item, style);
}

void VoteMenuHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	/* Check by our item count, NOT the vote array size */
	if (item < m_Items)
	{
		m_Votes[item]++;
		m_NumVotes++;
	}

	m_pHandler->OnMenuSelect(menu, client, item);
}

void VoteMenuHandler::Reset(IMenuHandler *mh)
{
	m_pHandler = mh;
	InternalReset();
}

void VoteMenuHandler::InternalReset()
{
	m_Clients = 0;
	m_Items = 0;
	m_bStarted = false;
	m_pCurMenu = NULL;
	m_NumVotes = 0;
	m_bCancelled = false;
}

void VoteMenuHandler::CancelVoting()
{
	m_bCancelled = true;
}

/*******************************
 *******************************
 ******** MENU MANAGER *********
 *******************************
 *******************************/

MenuManager::MenuManager()
{
	m_Styles.push_back(&g_ValveMenuStyle);
	SetDefaultStyle(&g_ValveMenuStyle);
}

void MenuManager::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);

	HandleAccess access;
	g_HandleSys.InitAccessDefaults(NULL, &access);

	/* Deny cloning to menus */
	access.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;
	m_MenuType = g_HandleSys.CreateType("IBaseMenu", this, 0, NULL, &access, g_pCoreIdent, NULL);

	/* Also deny deletion to styles */
	access.access[HandleAccess_Delete] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;
	m_StyleType = g_HandleSys.CreateType("IMenuStyle", this, 0, NULL, &access, g_pCoreIdent, NULL);
}

void MenuManager::OnSourceModAllShutdown()
{
	g_HandleSys.RemoveType(m_MenuType, g_pCoreIdent);
	g_HandleSys.RemoveType(m_StyleType, g_pCoreIdent);

	while (!m_VoteHandlers.empty())
	{
		delete m_VoteHandlers.front();
		m_VoteHandlers.pop();
	}
}

void MenuManager::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == m_MenuType)
	{
		IBaseMenu *menu = (IBaseMenu *)object;
		menu->Destroy(false);
	} else if (type == m_StyleType) {
		/* Do nothing */
	}
}

Handle_t MenuManager::CreateMenuHandle(IBaseMenu *menu, IdentityToken_t *pOwner)
{
	if (m_MenuType == NO_HANDLE_TYPE)
	{
		return BAD_HANDLE;
	}

	return g_HandleSys.CreateHandle(m_MenuType, menu, pOwner, g_pCoreIdent, NULL);
}

Handle_t MenuManager::CreateStyleHandle(IMenuStyle *style)
{
	if (m_StyleType == NO_HANDLE_TYPE)
	{
		return BAD_HANDLE;
	}

	return g_HandleSys.CreateHandle(m_StyleType, style, g_pCoreIdent, g_pCoreIdent, NULL);
}

HandleError MenuManager::ReadMenuHandle(Handle_t handle, IBaseMenu **menu)
{
	HandleSecurity sec;

	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = NULL;

	return g_HandleSys.ReadHandle(handle, m_MenuType, &sec, (void **)menu);
}

HandleError MenuManager::ReadStyleHandle(Handle_t handle, IMenuStyle **style)
{
	HandleSecurity sec;

	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = g_pCoreIdent;

	return g_HandleSys.ReadHandle(handle, m_StyleType, &sec, (void **)style);
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
	if (pgn != MENU_NO_PAGINATION)
	{
		maxItems = pgn;
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
		} else if (order == ItemOrder_Descending) {
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
	IMenuPanel *display = menu->CreatePanel();
	IMenuHandler *mh = md.mh;
	bool foundExtra = false;
	unsigned int extraItem = 0;

	/**
	 * We keep searching until:
	 * 1) There are no more items
	 * 2) We reach one OVER the maximum number of slot items
	 * 3) We have reached maxItems and pagination is MENU_NO_PAGINATION
	 */
	unsigned int i = startItem;
	unsigned int foundItems = 0;
	while (true)
	{
		ItemDrawInfo &dr = drawItems[foundItems].draw;
		/* Is the item valid? */
		if (menu->GetItemInfo(i, &dr) != NULL)
		{
			/* Ask the user to change the style, if necessary */
			mh->OnMenuDrawItem(menu, client, i, dr.style);
			/* Check if it's renderable */
			if (IsSlotItem(display, dr.style))
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
		display->DeleteThis();
		return NULL;
	}

	/* Check initial buttons */
	bool displayPrev = false;
	bool displayNext = false;
	if (foundExtra)
	{
		if (order == ItemOrder_Descending)
		{
			displayPrev = true;
			md.firstItem = extraItem;
		} else if (order == ItemOrder_Ascending) {
			displayNext = true;
			md.lastItem = extraItem;
		}
	}

	/**
	 * If we're paginated, we have to find if there is another page.
	 * Sadly, the only way to do this is to try drawing more items!
	 */
	if (pgn != MENU_NO_PAGINATION)
	{
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
				if (menu->GetItemInfo(lastItem, &dr) != NULL)
				{
					mh->OnMenuDrawItem(menu, client, lastItem, dr.style);
					if (IsSlotItem(display, dr.style))
					{
						displayNext = true;
						md.lastItem = lastItem;
						break;
					}
				}
			}
		} else if (order == ItemOrder_Ascending) {
			lastItem = drawItems[0].position;
			if (lastItem == 0)
			{
				goto skip_search;
			}
			lastItem--;
			while (lastItem != 0)
			{
				if (menu->GetItemInfo(lastItem, &dr) != NULL)
				{
					mh->OnMenuDrawItem(menu, client, lastItem, dr.style);
					if (IsSlotItem(display, dr.style))
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
	if (order == ItemOrder_Ascending)
	{
		for (unsigned int i=0; i<foundItems; i++)
		{
			ItemDrawInfo &dr = drawItems[i].draw;
			mh->OnMenuDisplayItem(menu, client, drawItems[i].position, &(dr.display));
			if ((position = display->DrawItem(dr)) != 0)
			{
				slots[position].item = drawItems[i].position;
				slots[position].type = ItemSel_Item;
			}
		}
	} else if (order == ItemOrder_Descending) {
		unsigned int i = foundItems;
		/* NOTE: There will always be at least one item because
		 * of the check earlier.
		 */
		while (i--)
		{
			ItemDrawInfo &dr = drawItems[i].draw;
			mh->OnMenuDisplayItem(menu, client, drawItems[i].position, &(dr.display));
			if ((position = display->DrawItem(dr)) != 0)
			{
				slots[position].item = drawItems[i].position;
				slots[position].type = ItemSel_Item;
			}
		}
	}

	/* Now, we need to check if we need to add anything extra */
	if (pgn != MENU_NO_PAGINATION)
	{
		bool canDrawDisabled = display->CanDrawItem(ITEMDRAW_DISABLED|ITEMDRAW_CONTROL);
		bool exitButton = menu->GetExitButton();
		bool exitBackButton = menu->GetExitBackButton();
		char text[50];

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

		/* Subtract two slots for the displayNext/displayPrev padding */
		padding -= 2;

		/* If we have an "Exit Back" button and the space to draw it, do so. */
		if (exitBackButton)
		{
			if (!displayPrev)
			{
				displayPrev = true;
			} else {
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
			if (!displayPrev && !displayPrev)
			{
				padItem.style = ITEMDRAW_NOTEXT;
			}
			/* Add spacers so we can pad to the end */
			for (unsigned int i=0; i<padding; i++)
			{
				position = display->DrawItem(padItem);
				slots[position].type = ItemSel_None;
			}
		}

		/* Put a fake spacer before control stuff, if possible */
		if ((displayPrev || displayNext) || exitButton)
		{
			ItemDrawInfo draw("", ITEMDRAW_RAWLINE|ITEMDRAW_SPACER);
			display->DrawItem(draw);
		}

		ItemDrawInfo dr(text, 0);
		/**
		 * If we have one or the other, we need to have spacers for both.
		 */
		if (displayPrev || displayNext)
		{
			/* PREVIOUS */
			ItemDrawInfo padCtrlItem(NULL, ITEMDRAW_SPACER|ITEMDRAW_CONTROL);
			if (displayPrev || canDrawDisabled)
			{
				if (exitBackButton)
				{
					CorePlayerTranslate(client, text, sizeof(text), "Back", NULL);
					dr.style = ITEMDRAW_CONTROL;
					position = display->DrawItem(dr);
					slots[position].type = ItemSel_ExitBack;
				} else {
					CorePlayerTranslate(client, text, sizeof(text), "Previous", NULL);
					dr.style = (displayPrev ? 0 : ITEMDRAW_DISABLED)|ITEMDRAW_CONTROL;
					position = display->DrawItem(dr);
					slots[position].type = ItemSel_Back;
				}
			} else if (displayNext || exitButton) {
				/* If we can't display this, and there is an exit button,
				 * we need to pad!
				 */
				position = display->DrawItem(padCtrlItem);
				slots[position].type = ItemSel_None;
			}

			/* NEXT */
			if (displayNext || canDrawDisabled)
			{
				CorePlayerTranslate(client, text, sizeof(text), "Next", NULL);
				dr.style = (displayNext ? 0 : ITEMDRAW_DISABLED)|ITEMDRAW_CONTROL;
				position = display->DrawItem(dr);
				slots[position].type = ItemSel_Next;
			} else if (exitButton) {
				/* If we can't display this,
				 * but there is an "exit" button, we need to pad!
				 */
				position = display->DrawItem(padCtrlItem);
				slots[position].type = ItemSel_None;
			}
		} else {
			/* Otherwise, bump to two slots! */
			ItemDrawInfo numBump(NULL, ITEMDRAW_NOTEXT);
			position = display->DrawItem(numBump);
			slots[position].type = ItemSel_None;
			position = display->DrawItem(numBump);
			slots[position].type = ItemSel_None;
		}

		/* EXIT */
		if (exitButton)
		{
			CorePlayerTranslate(client, text, sizeof(text), "Exit", NULL);
			dr.style = ITEMDRAW_CONTROL;
			position = display->DrawItem(dr);
			slots[position].type = ItemSel_Exit;
		}
	}

	/* Lastly, fill in any slots we could have missed */
	for (unsigned int i = position + 1; i < 10; i++)
	{
		slots[i].type = ItemSel_None;
	}

	/* Do title stuff */
	mh->OnMenuDisplay(menu, client, display);
	display->DrawTitle(menu->GetDefaultTitle(), true);

	return display;
}

IMenuStyle *MenuManager::GetDefaultStyle()
{
	return m_pDefaultStyle;
}

IVoteMenuHandler *MenuManager::CreateVoteWrapper(IMenuHandler *mh)
{
	VoteMenuHandler *vh = NULL;

	if (m_VoteHandlers.empty())
	{
		vh = new VoteMenuHandler;
	} else {
		vh = m_VoteHandlers.front();
		m_VoteHandlers.pop();
	}

	vh->Reset(mh);

	return vh;
}

void MenuManager::ReleaseVoteWrapper(IVoteMenuHandler *mh)
{
	if (mh == NULL)
	{
		return;
	}

	m_VoteHandlers.push((VoteMenuHandler *)mh);
}
