/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod TopMenus Extension
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

#include <stdlib.h>
#include <stdarg.h>
#include "TopMenu.h"

struct obj_by_name_t
{
	unsigned int obj_index;
	char name[TOPMENU_DISPLAY_BUFFER_SIZE];
};

int _SortObjectNamesDescending(const void *ptr1, const void *ptr2);
unsigned int strncopy(char *dest, const char *src, size_t count);
size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

TopMenu::TopMenu(ITopMenuObjectCallbacks *callbacks)
{
	m_clients = NULL;
	m_SerialNo = 1;
	m_pTitle = callbacks;
	m_max_clients = 0;
	m_bCacheTitles = true;

	if (playerhelpers->IsServerActivated())
	{
		CreatePlayers(playerhelpers->GetMaxClients());
	}
}

TopMenu::~TopMenu()
{
	/* Delete all categories */
	while (m_Categories.size())
	{
		RemoveFromMenu(m_Categories[0]->obj->object_id);
	}

	/* Remove all objects */
	for (size_t i = 0; i < m_Objects.size(); i++)
	{
		assert(m_Objects[i]->is_free == true);
		delete m_Objects[i];
	}

	m_pTitle->OnTopMenuObjectRemoved(this, 0);

	/* Delete all cached config entries */
	for (size_t i = 0; i < m_Config.cats.size(); i++)
	{
		delete m_Config.cats[i];
	}

	if (m_clients != NULL)
	{
		/* Sweep players */
		for (size_t i = 0; i <= (size_t)m_max_clients; i++)
		{
			TearDownClient(&m_clients[i]);
		}
		free(m_clients);
	}
}

unsigned int TopMenu::CalcMemUsage()
{
	unsigned int size = sizeof(TopMenu);

	size += m_Config.strings.GetMemTable()->GetMemUsage();
	size += (m_Config.cats.size() * sizeof(int));
	size += (sizeof(topmenu_player_t) * (SM_MAXPLAYERS + 1));
	size += (m_SortedCats.size() * sizeof(unsigned int));
	size += (m_UnsortedCats.size() * sizeof(unsigned int));
	size += (m_Categories.size() * (sizeof(topmenu_category_t *) + sizeof(topmenu_category_t)));
	size += (m_Objects.size() * (sizeof(topmenu_object_t *) + sizeof(topmenu_object_t)));
	size += m_ObjLookup.mem_usage();

	for (size_t i = 0; i < m_Categories.size(); i++)
	{
		size += m_Categories[i]->obj_list.size() * sizeof(topmenu_object_t *);
		size += m_Categories[i]->sorted.size() * sizeof(topmenu_object_t *);
		size += m_Categories[i]->unsorted.size() * sizeof(topmenu_object_t *);
	}

	return size;
}

void TopMenu::OnClientConnected(int client)
{
	if (m_clients == NULL)
	{
		return;
	}

	topmenu_player_t *player = &m_clients[client];
	TearDownClient(player);
}

void TopMenu::OnClientDisconnected(int client)
{
	if (m_clients == NULL)
	{
		return;
	}

	topmenu_player_t *player = &m_clients[client];
	TearDownClient(player);
}

void TopMenu::OnServerActivated(int max_clients)
{
	if (m_clients == NULL)
	{
		CreatePlayers(max_clients);
	}
}

unsigned int TopMenu::AddToMenu(const char *name,
								TopMenuObjectType type,
								ITopMenuObjectCallbacks *callbacks,
								IdentityToken_t *owner,
								const char *cmdname,
								FlagBits flags,
								unsigned int parent)
{
	return AddToMenu2(name, type, callbacks, owner, cmdname, flags, parent, NULL);
}

unsigned int TopMenu::AddToMenu2(const char *name,
								 TopMenuObjectType type,
								 ITopMenuObjectCallbacks *callbacks,
								 IdentityToken_t *owner,
								 const char *cmdname,
								 FlagBits flags,
								 unsigned int parent,
								 const char *info_string)
{
	/* Sanity checks */
	if (type == TopMenuObject_Category && parent != 0)
	{
		return 0;
	}
	else if (type == TopMenuObject_Item && parent == 0)
	{
		return 0;
	}
	else if (m_ObjLookup.contains(name))
	{
		return 0;
	}
	else if (type != TopMenuObject_Item && type != TopMenuObject_Category)
	{
		return 0;
	}

	/* If we're adding an item, make sure the parent is valid, 
	 * and that the parent is a category.
	 */
	topmenu_object_t *parent_obj = NULL;
	topmenu_category_t *parent_cat = NULL;
	if (type == TopMenuObject_Item)
	{
		size_t category_id;
		if (!FindCategoryByObject(parent, &category_id))
		{
			return 0;
		}

		parent_obj = m_Objects[parent - 1];
		parent_cat = m_Categories[category_id];
	}

	/* Re-use an old object pointer if we can. */
	topmenu_object_t *obj = NULL;
	for (size_t i = 0; i < m_Objects.size(); i++)
	{
		if (m_Objects[i]->is_free == true)
		{
			obj = m_Objects[i];
			break;
		}
	}

	/* Otherwise, allocate a new one. */
	if (obj == NULL)
	{
		obj = new topmenu_object_t;
		obj->object_id = ((unsigned int)m_Objects.size()) + 1;
		m_Objects.push_back(obj);
	}

	/* Initialize the object's properties. */
	obj->callbacks = callbacks;
	obj->flags = flags;
	obj->owner = owner;
	obj->type = type;
	obj->is_free = false;
	obj->parent = parent_obj;
	strncopy(obj->name, name, sizeof(obj->name));
	strncopy(obj->cmdname, cmdname ? cmdname : "", sizeof(obj->cmdname));
	strncopy(obj->info, info_string ? info_string : "", sizeof(obj->info));

	if (obj->type == TopMenuObject_Category)
	{
		/* Create a new category entry */
		topmenu_category_t *cat = new topmenu_category_t;
		cat->obj = obj;
		cat->reorder = false;
		cat->serial = 1;

		/* Add it, then update our serial change number. */
		obj->cat_id = m_Categories.size();
		m_Categories.push_back(cat);
		m_SerialNo++;

		/* Updating sorting info */
		m_bCatsNeedResort = true;
	}
	else if (obj->type == TopMenuObject_Item)
	{
		/* Update the category, mark it as needing changes */
		obj->cat_id = 0;
		parent_cat->obj_list.push_back(obj);
		parent_cat->reorder = true;
		parent_cat->serial++;

		/* If the category just went from 0 to 1 items, mark it as 
		 * changed, so clients get the category drawn.
		 */
		if (parent_cat->obj_list.size() == 1)
		{
			m_SerialNo++;
		}
	}

	m_ObjLookup.insert(name, obj);

	return obj->object_id;
}

const char *TopMenu::GetObjectInfoString(unsigned int object_id)
{
	if (object_id == 0 
		|| object_id > m_Objects.size() 
		|| m_Objects[object_id - 1]->is_free)
	{
		return NULL;
	}

	topmenu_object_t *obj = m_Objects[object_id - 1];

	return obj->info;
}

const char *TopMenu::GetObjectName(unsigned int object_id)
{
	if (object_id == 0 
		|| object_id > m_Objects.size() 
		|| m_Objects[object_id - 1]->is_free)
	{
		return NULL;
	}

	topmenu_object_t *obj = m_Objects[object_id - 1];

	return obj->name;
}

void TopMenu::RemoveFromMenu(unsigned int object_id)
{
	if (object_id == 0 
		|| object_id > m_Objects.size() 
		|| m_Objects[object_id - 1]->is_free)
	{
		return;
	}

	topmenu_object_t *obj = m_Objects[object_id - 1];

	m_ObjLookup.remove(obj->name);

	if (obj->type == TopMenuObject_Category)
	{
		/* Find it in the category list. */
		for (size_t i = 0; i < m_Categories.size(); i++)
		{
			if (m_Categories[i]->obj == obj)
			{
				/* Mark all children as removed + free.  Note we could
				 * call into RemoveMenuItem() for this, but it'd be very 
				 * inefficient!
				 */
				topmenu_category_t *cat = m_Categories[i];
				for (size_t j = 0; j < m_Categories[i]->obj_list.size(); j++)
				{
					m_ObjLookup.remove(cat->obj_list[j]->name);
					cat->obj_list[j]->callbacks->OnTopMenuObjectRemoved(this, cat->obj_list[j]->object_id);
					cat->obj_list[j]->is_free = true;
				}
				
				/* Remove the category from the list, then delete it. */
				m_Categories.erase(m_Categories.iterAt(i));
				delete cat;
				break;
			}
		}

		/* Update the root as changed. */
		m_SerialNo++;
		m_bCatsNeedResort = true;
	}
	else if (obj->type == TopMenuObject_Item)
	{
		/* Find the category this item is in. */
		topmenu_category_t *parent_cat = NULL;
		for (size_t i = 0; i < m_Categories.size(); i++)
		{
			if (m_Categories[i]->obj == obj->parent)
			{
				parent_cat = m_Categories[i];
				break;
			}
		}

		/* Erase it from the category's lists. */
		if (parent_cat)
		{
			for (size_t i = 0; i < parent_cat->obj_list.size(); i++)
			{
				if (parent_cat->obj_list[i] == obj)
				{
					parent_cat->obj_list.erase(parent_cat->obj_list.iterAt(i));

					/* If this category now has no items, mark root as changed 
					 * so clients won't get the category drawn anymore.
					 */
					if (parent_cat->obj_list.size() == 0)
					{
						m_SerialNo++;
					}
					break;
				}
			}

			/* Update the category as changed. */
			parent_cat->reorder = true;
			parent_cat->serial++;
		}
	}

	/* The callbacks pointer is still valid, so fire away! */
	obj->callbacks->OnTopMenuObjectRemoved(this, object_id);

	/* Finally, mark the object as free. */
	obj->is_free = true;
}

bool TopMenu::DisplayMenu(int client, unsigned int hold_time, TopMenuPosition position)
{
	if (m_clients == NULL)
	{
		return false;
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer->IsInGame())
	{
		return false;
	}

	UpdateClientRoot(client, pPlayer);

	/* This is unfortunate but it's the best solution. */
	for (size_t i = 0; i < m_Categories.size(); i++)
	{
		UpdateClientCategory(client, i, true);
	}

	topmenu_player_t *pClient = &m_clients[client];
	if (pClient->root == NULL)
	{
		return false;
	}

	if (!m_bCacheTitles)
	{
		char renderbuf[128];
		m_pTitle->OnTopMenuDisplayTitle(this, client, 0, renderbuf, sizeof(renderbuf));
		pClient->root->SetDefaultTitle(renderbuf);
	}

	bool return_value = false;

	if (position == TopMenuPosition_LastCategory && 
		pClient->last_category < m_Categories.size())
	{
		return_value = DisplayCategory(client, pClient->last_category, hold_time, true);
		if (!return_value)
		{
			return_value = pClient->root->DisplayAtItem(client, hold_time, pClient->last_root_pos);
		}
	}
	else if (position == TopMenuPosition_LastRoot)
	{
		pClient->root->DisplayAtItem(client, hold_time, pClient->last_root_pos);
	}
	else if (position == TopMenuPosition_Start)
	{
		pClient->last_position = 0;
		pClient->last_category = 0;
		return_value = pClient->root->Display(client, hold_time);
	}

	return return_value;
}

bool TopMenu::DisplayMenuAtCategory(int client, unsigned int object_id)
{
	if (m_clients == NULL)
	{
		return false;
	}

	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	if (!pPlayer->IsInGame())
	{
		return false;
	}

	// Get the category this object is in.
	size_t category_id;
	if (!FindCategoryByObject(object_id, &category_id))
	{
		return false;
	}

	topmenu_category_t *category = m_Categories[category_id];

	UpdateClientRoot(client, pPlayer);

	topmenu_player_t *pClient = &m_clients[client];
	if (pClient->root == NULL)
	{
		return false;
	}

	if (!m_bCacheTitles)
	{
		char renderbuf[128];
		m_pTitle->OnTopMenuDisplayTitle(this, client, 0, renderbuf, sizeof(renderbuf));
		pClient->root->SetDefaultTitle(renderbuf);
	}

	bool return_value = false;

	return_value = DisplayCategory(client, category_id, 0, true);
	if (!return_value)
	{
		return_value = pClient->root->DisplayAtItem(client, 0, pClient->last_root_pos);
	}

	return return_value;
}

bool TopMenu::FindCategoryByObject(unsigned int obj_id, size_t *index)
{
	if (obj_id == 0 
		|| obj_id > m_Objects.size() 
		|| m_Objects[obj_id - 1]->is_free)
	{
		return false;
	}

	topmenu_object_t *obj = m_Objects[obj_id - 1];
	if (obj->type != TopMenuObject_Category)
	{
		return false;
	}

	/* Find an equivalent pointer in the category array. */
	for (size_t i = 0; i < m_Categories.size(); i++)
	{
		if (m_Categories[i]->obj == obj)
		{
			*index = i;
			return true;
		}
	}

	return false;
}

bool TopMenu::DisplayCategory(int client, unsigned int category, unsigned int hold_time, bool last_position)
{
	UpdateClientCategory(client, category);

	topmenu_player_t *pClient = &m_clients[client];
	if (category >= pClient->cat_count || pClient->cats[category].menu == NULL)
	{
		return false;
	}

	bool return_value = false;

	topmenu_player_category_t *player_cat = &(pClient->cats[category]);

	// Refresh the title if the topmenu wants that.
	if (!m_bCacheTitles)
	{
		char renderbuf[128];
		m_Categories[category]->obj->callbacks->OnTopMenuDisplayTitle(this, 
			client, 
			m_Categories[category]->obj->object_id, 
			renderbuf, 
			sizeof(renderbuf));
		player_cat->menu->SetDefaultTitle(renderbuf);
	}

	pClient->last_category = category;
	if (last_position)
	{
		return_value = player_cat->menu->DisplayAtItem(client, hold_time, pClient->last_position);
	}
	else
	{
		return_value = player_cat->menu->Display(client, hold_time);
	}

	return return_value;
}

void TopMenu::SetTitleCaching(bool cache_titles)
{
	m_bCacheTitles = cache_titles;
}

void TopMenu::OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page)
{
	const char *item_name = menu->GetItemInfo(item, NULL);
	if (!item_name)
	{
		return;
	}

	topmenu_object_t *obj;
	topmenu_player_t *pClient = &m_clients[client];
	if (!m_ObjLookup.retrieve(item_name, &obj))
		return;

	/* We now have the object... what do we do with it? */
	if (obj->type == TopMenuObject_Category)
	{
		/* If it's a category, the user wants to view it.. */
		assert(obj->cat_id < m_Categories.size());
		assert(m_Categories[obj->cat_id]->obj == obj);
		pClient->last_root_pos = item_on_page;
		if (!DisplayCategory(client, obj->cat_id, MENU_TIME_FOREVER, false))
		{
			/* If we can't display the category, re-display the root menu.
			 * This code should be dead as of bug 3256, which disables categories 
			 * that cannot be displayed.
			 */
			DisplayMenu(client, MENU_TIME_FOREVER, TopMenuPosition_LastRoot);
		}
	}
	else
	{
		pClient->last_position = item_on_page;
		
		/* Re-check access in case this user had their credentials revoked */
		if (obj->cmdname[0] != '\0' && !adminsys->CheckAccess(client, obj->cmdname, obj->flags, false))
		{
			DisplayMenu(client, 0, TopMenuPosition_LastCategory);
			return;
		}
		
		/* Pass the information on to the callback */
		obj->callbacks->OnTopMenuSelectOption(this, client, obj->object_id);
	}
}

void TopMenu::OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style)
{
	const char *item_name = menu->GetItemInfo(item, NULL);
	if (!item_name)
	{
		return;
	}

	topmenu_object_t *obj;
	if (!m_ObjLookup.retrieve(item_name, &obj))
		return;

	/* If the category has nothing to display, disable it. */
	if (obj->type == TopMenuObject_Category)
	{
		assert(obj->cat_id < m_Categories.size());
		assert(m_Categories[obj->cat_id]->obj == obj);
		topmenu_player_t *pClient = &m_clients[client];
		if (obj->cat_id >= pClient->cat_count || pClient->cats[obj->cat_id].menu == NULL)
		{
			style = ITEMDRAW_IGNORE;
			return;
		}
	}

	style = obj->callbacks->OnTopMenuDrawOption(this, client, obj->object_id);
	if (style != ITEMDRAW_DEFAULT)
	{
		return;
	}

	if (obj->cmdname[0] == '\0')
	{
		return;
	}

	if (!adminsys->CheckAccess(client, obj->cmdname, obj->flags, false))
	{
		style = ITEMDRAW_IGNORE;
	}
}

unsigned int TopMenu::OnMenuDisplayItem(IBaseMenu *menu,
								int client,
								IMenuPanel *panel,
								unsigned int item,
								const ItemDrawInfo &dr)
{
	const char *item_name = menu->GetItemInfo(item, NULL);
	if (!item_name)
	{
		return 0;
	}

	topmenu_object_t *obj;
	if (!m_ObjLookup.retrieve(item_name, &obj))
		return 0;

	/* Ask the object to render the text for this client */
	char renderbuf[TOPMENU_DISPLAY_BUFFER_SIZE];
	obj->callbacks->OnTopMenuDisplayOption(this, client, obj->object_id, renderbuf, sizeof(renderbuf));

	/* Build the new draw info */
	ItemDrawInfo new_dr = dr;
	new_dr.display = renderbuf;

	/* Man I love the menu API.  Ask the panel to draw the item and give the position 
	 * back to Core's renderer.  This way we don't have to worry about keeping the 
	 * render buffer static!
	 */
	return panel->DrawItem(new_dr);
}

void TopMenu::OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
{
	if (reason == MenuCancel_ExitBack)
	{
		/* If the client chose exit back, they were on a category menu, and we can
		 * now display the root menu from the last known position.
		 */
		DisplayMenu(client, 0, TopMenuPosition_LastRoot);
	}
}

void TopMenu::UpdateClientRoot(int client, IGamePlayer *pGamePlayer)
{
	topmenu_player_t *pClient = &m_clients[client];
	IGamePlayer *pPlayer = pGamePlayer ? pGamePlayer : playerhelpers->GetGamePlayer(client);

	/* Determine if an update is necessary */
	bool is_update_needed = false;
	if (pClient->menu_serial != m_SerialNo)
	{
		is_update_needed = true;
	}
	else if (pPlayer->GetUserId() != pClient->user_id)
	{
		is_update_needed = true;
	}

	/* If no update is needed at the root level, just leave now */
	if (!is_update_needed)
	{
		return;
	}

	/* First we need to flush the cache... */
	TearDownClient(pClient);

	/* Now, rebuild the category list, but don't create menus */
	if (m_Categories.size() == 0)
	{
		pClient->cat_count = 0;
		pClient->cats = NULL;
	}
	else
	{
		pClient->cat_count = (unsigned int)m_Categories.size();
		pClient->cats = new topmenu_player_category_t[pClient->cat_count];
		memset(pClient->cats, 0, sizeof(topmenu_player_category_t) * pClient->cat_count);
	}

	/* Re-sort the root categories if needed */
	SortCategoriesIfNeeded();

	/* Build the root menu */
	IBaseMenu *root_menu = menus->GetDefaultStyle()->CreateMenu(this, myself->GetIdentity());

	/* Add the sorted items */
	for (size_t i = 0; i < m_SortedCats.size(); i++)
	{
		if (m_Categories[m_SortedCats[i]]->obj_list.size() == 0)
		{
			continue;
		}
		root_menu->AppendItem(m_Categories[m_SortedCats[i]]->obj->name, ItemDrawInfo(""));
	}

	/* Now we need to handle un-sorted items.  This is slightly trickier, as we need to 
	 * pre-render each category name, and cache those names.  Phew!
	 */
	if (m_UnsortedCats.size())
	{
		obj_by_name_t *item_list = new obj_by_name_t[m_UnsortedCats.size()];
		for (size_t i = 0; i < m_UnsortedCats.size(); i++)
		{
			obj_by_name_t *temp_obj = &item_list[i];
			topmenu_object_t *obj = m_Categories[m_UnsortedCats[i]]->obj;
			obj->callbacks->OnTopMenuDisplayOption(this,
				client, 
				obj->object_id,
				temp_obj->name,
				sizeof(temp_obj->name));
			temp_obj->obj_index = m_UnsortedCats[i];
		}

		/* Sort our temp list */
		qsort(item_list, m_UnsortedCats.size(), sizeof(obj_by_name_t), _SortObjectNamesDescending);

		/* Add the new sorted categories */
		for (size_t i = 0; i < m_UnsortedCats.size(); i++)
		{
			if (m_Categories[item_list[i].obj_index]->obj_list.size() == 0)
			{
				continue;
			}
			root_menu->AppendItem(m_Categories[item_list[i].obj_index]->obj->name, ItemDrawInfo(""));
		}

		delete [] item_list;
	}

	/* Set the menu's title */
	if (m_bCacheTitles)
	{
		char renderbuf[128];
		m_pTitle->OnTopMenuDisplayTitle(this, client, 0, renderbuf, sizeof(renderbuf));
		root_menu->SetDefaultTitle(renderbuf);
	}

	/* The client is now fully updated */
	pClient->root = root_menu;
	pClient->user_id = pPlayer->GetUserId();
	pClient->menu_serial = m_SerialNo;
	pClient->last_position = 0;
	pClient->last_category = 0;
	pClient->last_root_pos = 0;
}

void TopMenu::UpdateClientCategory(int client, unsigned int category, bool bSkipRootCheck)
{
	bool has_access = false;

	/* Update the client's root menu just in case it needs it.  This 
	 * will validate that we have both a valid client and a valid
	 * category structure for that client.
	 */
	if (!bSkipRootCheck)
	{
		UpdateClientRoot(client);
	}

	/* Now it's guaranteed that our category tables will be usable */
	topmenu_player_t *pClient = &m_clients[client];
	topmenu_category_t *cat = m_Categories[category];
	topmenu_player_category_t *player_cat = &(pClient->cats[category]);

	/* Does the category actually need updating? */
	if (player_cat->serial == cat->serial)
	{
		return;
	}

	/* Destroy any existing menu */
	if (player_cat->menu)
	{
		player_cat->menu->Destroy();
		player_cat->menu = NULL;
	}

	if (pClient->last_category == category)
	{
		pClient->last_position = 0;
	}

	IBaseMenu *cat_menu = menus->GetDefaultStyle()->CreateMenu(this, myself->GetIdentity());
	
	/* Categories get an "exit back" button */
	cat_menu->SetMenuOptionFlags(cat_menu->GetMenuOptionFlags() | MENUFLAG_BUTTON_EXITBACK);

	/* Re-sort the category if needed */
	SortCategoryIfNeeded(category);

	/* Build the menu with the sorted items first */
	for (size_t i = 0; i < cat->sorted.size(); i++)
	{
		cat_menu->AppendItem(cat->sorted[i]->name, ItemDrawInfo(""));
		if (!has_access && adminsys->CheckAccess(client,
												 cat->sorted[i]->cmdname,
												 cat->sorted[i]->flags,
												 false))
		{
			has_access = true;
		}
	}

	/* Now handle unsorted items */
	if (cat->unsorted.size())
	{
		/* Build a list of the item names */
		obj_by_name_t *item_list = new obj_by_name_t[cat->unsorted.size()];
		for (size_t i = 0; i < cat->unsorted.size(); i++)
		{
			obj_by_name_t *item = &item_list[i];
			topmenu_object_t *obj = cat->unsorted[i];
			obj->callbacks->OnTopMenuDisplayOption(this,
				client,
				obj->object_id,
				item->name,
				sizeof(item->name));
			item->obj_index = (unsigned int)i;
			if (!has_access && adminsys->CheckAccess(client, obj->cmdname, obj->flags, false))
			{
				has_access = true;
			}
		}

		if (has_access)
		{
			/* Sort the names */
			qsort(item_list,
				cat->unsorted.size(),
				sizeof(obj_by_name_t),
				_SortObjectNamesDescending);

			/* Add to the menu */
			for (size_t i = 0; i < cat->unsorted.size(); i++)
			{
				cat_menu->AppendItem(cat->unsorted[item_list[i].obj_index]->name, ItemDrawInfo(""));
			}
		}

		delete [] item_list;
	}

	/* If the player has access to no items, don't draw a menu. */
	if (!has_access)
	{
		cat_menu->Destroy();
		player_cat->menu = NULL;
		player_cat->serial = cat->serial;
		return;
	}

	/* Set the menu's title */
	if (m_bCacheTitles)
	{
		char renderbuf[128];
		cat->obj->callbacks->OnTopMenuDisplayTitle(this, 
			client, 
			cat->obj->object_id, 
			renderbuf, 
			sizeof(renderbuf));
		cat_menu->SetDefaultTitle(renderbuf);
	}

	/* We're done! */
	player_cat->menu = cat_menu;
	player_cat->serial = cat->serial;
}

void TopMenu::SortCategoryIfNeeded(unsigned int category)
{
	topmenu_category_t *cat = m_Categories[category];
	if (!cat->reorder)
	{
		return;
	}

	cat->sorted.clear();
	cat->unsorted.clear();

	if (cat->obj_list.size() == 0)
	{
		cat->reorder = false;
		return;
	}

	CVector<unsigned int> to_sort;
	for (size_t i = 0; i < cat->obj_list.size(); i++)
	{
		to_sort.push_back(i);
	}

	/* Find a matching category in the configs */
	config_category_t *config_cat = NULL; 
	for (size_t i = 0; i < m_Config.cats.size(); i++)
	{
		if (strcmp(m_Config.strings.GetString(m_Config.cats[i]->name), cat->obj->name) == 0)
		{
			config_cat = m_Config.cats[i];
			break;
		}
	}

	/* If there is a matching category, build a pre-sorted item list */
	if (config_cat != NULL)
	{
		/* Find matching objects in this category */
		for (size_t i = 0; i < config_cat->commands.size(); i++)
		{
			const char *config_name = m_Config.strings.GetString(config_cat->commands[i]);
			for (size_t j = 0; j < to_sort.size(); j++)
			{
				if (strcmp(config_name, cat->obj_list[to_sort[j]]->name) == 0)
				{
					/* Place in the final list, then remove from the temporary list */
					cat->sorted.push_back(cat->obj_list[to_sort[j]]);
					to_sort.erase(to_sort.iterAt(j));
					break;
				}
			}
		}
	}

	/* Push any remaining items onto the unsorted list */
	for (size_t i = 0; i < to_sort.size(); i++)
	{
		cat->unsorted.push_back(cat->obj_list[to_sort[i]]);
	}

	cat->reorder = false;
}

void TopMenu::SortCategoriesIfNeeded()
{
	if (!m_bCatsNeedResort)
	{
		return;
	}

	/* Clear sort results */
	m_SortedCats.clear();
	m_UnsortedCats.clear();

	if (m_Categories.size() == 0)
	{
		m_bCatsNeedResort = false;
		return;
	}

	CVector<unsigned int> to_sort;
	for (unsigned int i = 0; i < (unsigned int)m_Categories.size(); i++)
	{
		to_sort.push_back(i);
	}

	/* If we have any predefined categories, add them in as they appear. */
	for (size_t i= 0; i < m_Config.cats.size(); i++)
	{
		/* Find this category and map it in if we can */
		for (size_t j = 0; j < to_sort.size(); j++)
		{
			if (strcmp(m_Config.strings.GetString(m_Config.cats[i]->name), 
					   m_Categories[to_sort[j]]->obj->name) == 0)
			{
				/* Add to the real list and remove from the temporary */
				m_SortedCats.push_back(to_sort[j]);
				to_sort.erase(to_sort.iterAt(j));
				break;
			}
		}
	}

	/* Push any remaining items onto the unsorted list */
	for (size_t i = 0; i < to_sort.size(); i++)
	{
		m_UnsortedCats.push_back(to_sort[i]);
	}

	m_bCatsNeedResort = false;
}

void TopMenu::CreatePlayers(int max_clients)
{
	m_max_clients = max_clients;
	m_clients = (topmenu_player_t *)malloc(sizeof(topmenu_player_t) * (SM_MAXPLAYERS + 1));
	memset(m_clients, 0, sizeof(topmenu_player_t) * (SM_MAXPLAYERS + 1));
}

void TopMenu::TearDownClient(topmenu_player_t *player)
{
	if (player->cats != NULL)
	{
		for (unsigned int i = 0; i < player->cat_count; i++)
		{
			topmenu_player_category_t *player_cat = &(player->cats[i]);
			if (player_cat->menu != NULL)
			{
				player_cat->menu->Destroy();
			}
		}
		delete [] player->cats;
	}

	if (player->root != NULL)
	{
		player->root->Destroy();
	}

	memset(player, 0, sizeof(topmenu_player_t));
}

bool TopMenu::LoadConfiguration(const char *file, char *error, size_t maxlength)
{
	SMCError err;
	SMCStates states;

	if ((err = textparsers->ParseFile_SMC(file, this, &states))
		!= SMCError_Okay)
	{
		const char *err_string = textparsers->GetSMCErrorString(err);
		if (!err_string)
		{
			err_string = "Unknown";
		}

		UTIL_Format(error, maxlength, "%s", err_string);

		return false;
	}

	m_SerialNo++;
	m_bCatsNeedResort = true;

	return true;
}

bool TopMenu::OnIdentityRemoval(IdentityToken_t *owner)
{
	/* First sweep the categories owned by us */
	CVector<unsigned int> obj_list;
	for (size_t i = 0; i < m_Categories.size(); i++)
	{
		if (m_Categories[i]->obj->owner == owner)
		{
			obj_list.push_back(m_Categories[i]->obj->object_id);
		}
	}

	for (size_t i = 0; i < obj_list.size(); i++)
	{
		RemoveFromMenu(obj_list[i]);
	}

	/* Now we can look for actual items */
	for (size_t i = 0; i < m_Objects.size(); i++)
	{
		if (m_Objects[i]->is_free)
		{
			continue;
		}
		if (m_Objects[i]->owner == owner)
		{
			assert(m_Objects[i]->type != TopMenuObject_Category);
			RemoveFromMenu(m_Objects[i]->object_id);
		}
	}

	return true;
}

#define PARSE_STATE_NONE		0
#define PARSE_STATE_MAIN		1
#define PARSE_STATE_CATEGORY	2
unsigned int ignore_parse_level = 0;
unsigned int current_parse_state = 0;
config_category_t *cur_cat = NULL;

void TopMenu::ReadSMC_ParseStart()
{
	current_parse_state = PARSE_STATE_NONE;
	ignore_parse_level = 0;
	cur_cat = NULL;

	/* Reset the old config */
	m_Config.strings.Reset();
	for (size_t i = 0; i < m_Config.cats.size(); i++)
	{
		delete m_Config.cats[i];
	}
	m_Config.cats.clear();
}

SMCResult TopMenu::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (ignore_parse_level)
	{
		ignore_parse_level++;
	}
	else
	{
		if (current_parse_state == PARSE_STATE_NONE)
		{
			if (strcmp(name, "Menu") == 0)
			{
				current_parse_state = PARSE_STATE_MAIN;
			}
			else
			{
				ignore_parse_level = 1;
			}
		}
		else if (current_parse_state == PARSE_STATE_MAIN)
		{
			cur_cat = new config_category_t;
			cur_cat->name = m_Config.strings.AddString(name);
			m_Config.cats.push_back(cur_cat);
			current_parse_state = PARSE_STATE_CATEGORY;

			// This category needs reordering now that the sorting file is defining something new for it.
			for (unsigned int i = 0; i < (unsigned int)m_Categories.size(); i++)
			{
				if (strcmp(name, m_Categories[i]->obj->name) == 0)
				{
					m_Categories[i]->reorder = true;
					m_Categories[i]->serial++;
					break;
				}
			}
		}
		else
		{
			ignore_parse_level = 1;
		}
	}

	return SMCResult_Continue;
}

SMCResult TopMenu::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	if (ignore_parse_level > 0
		|| current_parse_state != PARSE_STATE_CATEGORY
		|| cur_cat == NULL)
	{
		return SMCResult_Continue;
	}

	if (strcmp(key, "item") == 0)
	{
		cur_cat->commands.push_back(m_Config.strings.AddString(value));
	}

	return SMCResult_Continue;
}

SMCResult TopMenu::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (ignore_parse_level)
	{
		ignore_parse_level--;
	}
	else
	{
		if (current_parse_state == PARSE_STATE_CATEGORY)
		{
			cur_cat = NULL;
			current_parse_state = PARSE_STATE_MAIN;
		}
		else if (current_parse_state == PARSE_STATE_MAIN)
		{
			current_parse_state = PARSE_STATE_NONE;
		}
	}

	return SMCResult_Continue;
}

unsigned int TopMenu::FindCategory(const char *name)
{
	topmenu_object_t *obj;
	if (!m_ObjLookup.retrieve(name, &obj))
		return 0;

	if (obj->type != TopMenuObject_Category)
		return 0;

	return obj->object_id;
}

void TopMenu::OnMaxPlayersChanged( int newvalue )
{
	m_max_clients = newvalue;
}

int _SortObjectNamesDescending(const void *ptr1, const void *ptr2)
{
	obj_by_name_t *obj1 = (obj_by_name_t *)ptr1;
	obj_by_name_t *obj2 = (obj_by_name_t *)ptr2;
	return strcmp(obj1->name, obj2->name);
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}
