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


#include <stdarg.h>
#include <time.h>
#include "MenuManager.h"
#include "sm_stringutil.h"
#include "sourcemm_api.h"
#include "PlayerManager.h"
#include "MenuStyle_Valve.h"

MenuManager g_Menus;

/*************************************
 *************************************
 **** BROADCAST HANDLING WRAPPERS ****
 *************************************
 *************************************/

BroadcastHandler::BroadcastHandler(IMenuHandler *handler) : m_pHandler(handler), numClients(0)
{
}

unsigned int BroadcastHandler::GetMenuAPIVersion2()
{
	return m_pHandler->GetMenuAPIVersion2();
}

void BroadcastHandler::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	m_pHandler->OnMenuCancel(menu, client, reason);
}

void BroadcastHandler::OnMenuDisplay(IBaseMenu *menu, int client, IMenuDisplay *display)
{
	numClients++;
	m_pHandler->OnMenuDisplay(menu, client, display);
}

void BroadcastHandler::OnBroadcastEnd(IBaseMenu *menu)
{
	g_Menus.FreeBroadcastHandler(this);
}

void BroadcastHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	m_pHandler->OnMenuSelect(menu, client, item);
}

void BroadcastHandler::OnMenuEnd(IBaseMenu *menu)
{
	assert(numClients > 0);

	/* Only fire if all clients have gotten a menu end */
	if (--numClients == 0)
	{
		IMenuHandler *pHandler = m_pHandler;
		OnBroadcastEnd(menu);
		pHandler->OnMenuEnd(menu);
	}
}

VoteHandler::VoteHandler(IMenuVoteHandler *handler) 
: BroadcastHandler(handler), m_pVoteHandler(handler)
{
}

void VoteHandler::Initialize(IBaseMenu *menu)
{
	unsigned int numItems = menu->GetItemCount();

	if (m_counts.size() >= numItems)
	{
		for (size_t i=0; i<numItems; i++)
		{
			m_counts[i] = 0;
		}
	} else {
		for (size_t i=0; i<m_counts.size(); i++)
		{
			m_counts[i] = 0;
		}
		m_counts.resize(numItems, 0);
	}
}

void VoteHandler::OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
{
	if (item < numItems)
	{
		m_counts[item]++;
	}

	BroadcastHandler::OnMenuSelect(menu, client, item);
}

void VoteHandler::OnBroadcastEnd(IBaseMenu *menu)
{
	m_ties.clear();

	size_t highest = 0;
	for (size_t i=1; i<numItems; i++)
	{
		if (m_counts[i] > m_counts[highest])
		{
			m_ties.clear();
			highest = i;
		} else if (m_counts[i] == m_counts[highest]) {
			m_ties.push_back(i);
		}
	}

	if (m_ties.size())
	{
		m_ties.push_back(highest);
		srand(static_cast<unsigned int>(time(NULL)));
		highest = m_ties[rand() % m_ties.size()];
	}

	m_pVoteHandler->OnMenuVoteEnd(menu, highest);

	g_Menus.FreeVoteHandler(this);
}

/*******************************
 *******************************
 ******** MENU MANAGER *********
 *******************************
 *******************************/

MenuManager::MenuManager()
{
	m_ShowMenu = -1;
	m_pDefaultStyle = NULL;
}

void MenuManager::OnSourceModAllInitialized()
{
	int num = g_SMAPI->GetUserMessageCount();
	if (num >= 1)
	{
		for (int i=0; i<num; i++)
		{
			if (strcmp(g_SMAPI->GetUserMessage(i, NULL), "ShowMenu") == 0)
			{
				m_ShowMenu = i;
				break;
			}
		}
	}

	/* :TODO: styles */
	m_Styles.push_back(&g_ValveMenuStyle);
	SetDefaultStyle(&g_ValveMenuStyle);
}

void MenuManager::OnSourceModAllShutdown()
{
	while (!m_BroadcastHandlers.empty())
	{
		delete m_BroadcastHandlers.front();
		m_BroadcastHandlers.pop();
	}

	while (!m_VoteHandlers.empty())
	{
		delete m_VoteHandlers.front();
		m_VoteHandlers.pop();
	}
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

inline bool IsSlotItem(IMenuDisplay *display,
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

IMenuDisplay *MenuManager::RenderMenu(int client, menu_states_t &md, ItemOrder order)
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
	IMenuDisplay *display = menu->CreateDisplay();
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
			while (lastItem-- != 0)
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
			menu->GetItemInfo(drawItems[i].position, &dr);
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
			menu->GetItemInfo(drawItems[i].position, &dr);
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
		bool canDrawDisabled = display->CanDrawItem(ITEMDRAW_DISABLED);
		bool exitButton = menu->GetExitButton();
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

		/**
		 * We allow next/prev to be undrawn if neither exists.
		 * Thus, we only need padding if one of them will be drawn,
		 * or the exit button will be drawn.
		 */
		ItemDrawInfo padItem(NULL, ITEMDRAW_SPACER);
		if (exitButton || (displayNext || displayPrev))
		{
			/* Add spacers so we can pad to the end */
			unsigned int null_pos = 0;
			for (unsigned int i=0; i<padding; i++)
			{
				position = display->DrawItem(padItem);
				slots[position].type = ItemSel_None;
			}
		}

		/* Put a fake spacer before control stuff, if possible */
		{
			ItemDrawInfo draw("", ITEMDRAW_RAWLINE|ITEMDRAW_SPACER);
			display->DrawItem(draw);
		}

		/* PREVIOUS */
		ItemDrawInfo dr(text, 0);
		if (displayPrev || canDrawDisabled)
		{
			CorePlayerTranslate(client, text, sizeof(text), "Back", NULL);
			dr.style = displayPrev ? 0 : ITEMDRAW_DISABLED;
			position = display->DrawItem(dr);
			slots[position].type = ItemSel_Back;
		} else if ((displayNext || canDrawDisabled) || exitButton) {
			/* If we can't display this, 
			 * but there is a "next" or "exit" button, we need to pad!
			 */
			position = display->DrawItem(padItem);
			slots[position].type = ItemSel_None;
		}

		/* NEXT */
		if (displayNext || canDrawDisabled)
		{
			CorePlayerTranslate(client, text, sizeof(text), "Next", NULL);
			dr.style = displayNext ? 0 : ITEMDRAW_DISABLED;
			position = display->DrawItem(dr);
			slots[position].type = ItemSel_Next;
		} else if (exitButton) {
			/* If we can't display this,
			 * but there is an exit button, we need to pad!
			 */
			position = display->DrawItem(padItem);
			slots[position].type = ItemSel_None;
		}

		/* EXIT */
		if (exitButton)
		{
			CorePlayerTranslate(client, text, sizeof(text), "Exit", NULL);
			dr.style = 0;
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

unsigned int MenuManager::BroadcastMenu(IBaseMenu *menu, 
										IMenuHandler *handler, 
										int clients[], 
										unsigned int numClients, 
										unsigned int time)
{
	BroadcastHandler *bh;

	if (m_BroadcastHandlers.empty())
	{
		bh = new BroadcastHandler(handler);
	} else {
		bh = m_BroadcastHandlers.front();
		m_BroadcastHandlers.pop();
		bh->m_pHandler = handler;
		bh->numClients = 0;
	}

	handler->OnMenuStart(menu);

	unsigned int total = 0;
	for (unsigned int i=0; i<numClients; i++)
	{
		/* Only continue if displaying works */
		if (!menu->Display(clients[i], bh, time))
		{
			continue;
		}

		/* :TODO: Allow sourcetv only, not all bots */
		CPlayer *player = g_Players.GetPlayerByIndex(clients[i]);
		if (player->IsFakeClient())
		{
			continue;
		}

		total++;
	}

	if (!total)
	{
		/* End the broadcast here */
		handler->OnMenuEnd(menu);
		FreeBroadcastHandler(bh);
	}

	return total;
}

unsigned int MenuManager::VoteMenu(IBaseMenu *menu,
								   IMenuVoteHandler *handler,
								   int clients[],
								   unsigned int numClients,
								   unsigned int time)
{
	VoteHandler *vh;

	if (m_VoteHandlers.empty())
	{
		vh = new VoteHandler(handler);
	} else {
		vh = m_VoteHandlers.front();
		m_VoteHandlers.pop();
		vh->m_pHandler = handler;
		vh->numClients = 0;
	}

	vh->Initialize(menu);
	handler->OnMenuStart(menu);

	unsigned int total = 0;
	for (unsigned int i=0; i<numClients; i++)
	{
		/* Only continue if displaying works */
		if (!menu->Display(clients[i], vh, time))
		{
			continue;
		}

		/* :TODO: Allow sourcetv only, not all bots */
		CPlayer *player = g_Players.GetPlayerByIndex(clients[i]);
		if (player->IsFakeClient())
		{
			continue;
		}

		total++;
	}

	if (!total)
	{
		/* End the broadcast here */
		handler->OnMenuEnd(menu);
		FreeVoteHandler(vh);
	}

	return total;
}

void MenuManager::FreeBroadcastHandler(BroadcastHandler *bh)
{
	m_BroadcastHandlers.push(bh);
}

void MenuManager::FreeVoteHandler(VoteHandler *vh)
{
	m_VoteHandlers.push(vh);
}

IMenuStyle *MenuManager::GetDefaultStyle()
{
	return m_pDefaultStyle;
}

