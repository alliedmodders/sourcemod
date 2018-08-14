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

#ifndef _INCLUDE_SOURCEMOD_TOP_MENU_H_
#define _INCLUDE_SOURCEMOD_TOP_MENU_H_

#include <sh_list.h>
#include <sh_vector.h>
#include <ITopMenus.h>
#include "smsdk_ext.h"
#include "sm_memtable.h"
#include <sm_namehashset.h>

using namespace SourceHook;
using namespace SourceMod;

#define TOPMENU_DISPLAY_BUFFER_SIZE 128

struct config_category_t
{
	int name;
	CVector<int> commands;
};

struct config_root_t
{
	config_root_t() : strings(1024)
	{
	}
	BaseStringTable strings;
	CVector<config_category_t *> cats;
};

struct topmenu_object_t
{
	char name[64];						/** Name */
	char cmdname[64];					/** Command name */ 
	FlagBits flags;						/** Admin flags */
	ITopMenuObjectCallbacks *callbacks;	/** Callbacks */
	IdentityToken_t *owner;				/** Owner */
	unsigned int object_id;				/** Object ID */
	topmenu_object_t *parent;			/** Parent, if any */
	TopMenuObjectType type;				/** Object Type */
	bool is_free;						/** Free or not? */
	char info[255];						/** Info string */
	unsigned int cat_id;				/** Set if a category */

	static inline bool matches(const char *name, const topmenu_object_t *topmenu)
	{
		return strcmp(name, topmenu->name) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

struct topmenu_category_t
{
	CVector<topmenu_object_t *> obj_list;	/** Full object list */
	CVector<topmenu_object_t *> sorted;		/** Sorted items */
	CVector<topmenu_object_t *> unsorted;	/** Unsorted items */
	topmenu_object_t *obj;					/** Bound object */
	unsigned int serial;					/** Serial number */
	bool reorder;							/** Whether ordering needs updating */
};

struct topmenu_player_category_t
{
	IBaseMenu *menu;					/** menu pointer */
	unsigned int serial;				/** last known serial */
};

struct topmenu_player_t
{
	int user_id;						/** userid on server */
	unsigned int menu_serial;			/** menu serial no */
	IBaseMenu *root;					/** root menu display */
	topmenu_player_category_t *cats;	/** category display */
	unsigned int cat_count;				/** number of categories */
	unsigned int last_category;			/** last category they selected */
	unsigned int last_position;			/** last position in that category */
	unsigned int last_root_pos;			/** last page in the root menu */
};

class TopMenu : 
	public ITopMenu,
	public IMenuHandler,
	public ITextListener_SMC
{
	friend class TopMenuManager;
public:
	TopMenu(ITopMenuObjectCallbacks *callbacks);
	~TopMenu();
public: //ITopMenu
	virtual unsigned int AddToMenu(const char *name,
		TopMenuObjectType type,
		ITopMenuObjectCallbacks *callbacks,
		IdentityToken_t *owner,
		const char *cmdname,
		FlagBits flags,
		unsigned int parent);
	unsigned int AddToMenu2(const char *name,
		TopMenuObjectType type,
		ITopMenuObjectCallbacks *callbacks,
		IdentityToken_t *owner,
		const char *cmdname,
		FlagBits flags,
		unsigned int parent,
		const char *info_string);
	virtual void RemoveFromMenu(unsigned int object_id);
	virtual bool DisplayMenu(int client, 
		unsigned int hold_time, 
		TopMenuPosition position);
	virtual bool LoadConfiguration(const char *file, char *error, size_t maxlength);
	virtual unsigned int FindCategory(const char *name);
	const char *GetObjectInfoString(unsigned int object_id);
	const char *GetObjectName(unsigned int object_id);
public: //IMenuHandler
	virtual void OnMenuSelect2(IBaseMenu *menu, int client, unsigned int item, unsigned int item_on_page);
	virtual void OnMenuDrawItem(IBaseMenu *menu, int client, unsigned int item, unsigned int &style);
	virtual unsigned int OnMenuDisplayItem(IBaseMenu *menu, 
		int client, 
		IMenuPanel *panel,
		unsigned int item, 
		const ItemDrawInfo &dr);
	virtual void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason);
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public:
	unsigned int CalcMemUsage();
	void SetTitleCaching(bool cache_titles);
	bool DisplayMenuAtCategory(int client, unsigned int object_id);
private:
	void SortCategoriesIfNeeded();
	void SortCategoryIfNeeded(unsigned int category);
private:
	bool DisplayCategory(int client, unsigned int category, unsigned int hold_time, bool last_position);
	void CreatePlayers(int max_clients);
	void UpdateClientRoot(int client, IGamePlayer *pGamePlayer=NULL);
	void UpdateClientCategory(int client, unsigned int category, bool bSkipRootCheck=false);
	void TearDownClient(topmenu_player_t *player);
	bool FindCategoryByObject(unsigned int obj_id, size_t *index);
private:
	void OnClientConnected(int client);
	void OnClientDisconnected(int client);
	void OnServerActivated(int max_clients);
	bool OnIdentityRemoval(IdentityToken_t *owner);
	void OnMaxPlayersChanged(int newvalue);
private:
	config_root_t m_Config;					/* Configuration from file */
	topmenu_player_t *m_clients;			/* Client array */
	CVector<unsigned int> m_SortedCats;		/* Sorted categories */
	CVector<unsigned int> m_UnsortedCats;	/* Un-sorted categories */
	CVector<topmenu_category_t *> m_Categories; /* Category array */
	CVector<topmenu_object_t *> m_Objects;	/* Object array */
	NameHashSet<topmenu_object_t *> m_ObjLookup; /* Object lookup trie */
	unsigned int m_SerialNo;				/* Serial number for updating */
	ITopMenuObjectCallbacks *m_pTitle;		/* Title callbacks */
	int m_max_clients;						/* Maximum number of clients */
	bool m_bCatsNeedResort;					/* True if categories need a resort */
	bool m_bCacheTitles;						/* True if the categorie titles should be cached */
};

unsigned int strncopy(char *dest, const char *src, size_t count);

#endif //_INCLUDE_SOURCEMOD_TOP_MENU_H_
