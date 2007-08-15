/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_CHALFLIFE2_H_
#define _INCLUDE_SOURCEMOD_CHALFLIFE2_H_

#include <sh_list.h>
#include <sh_tinyhash.h>
#include "sm_trie.h"
#include "sm_globals.h"
#include <IGameHelpers.h>
#include <KeyValues.h>

using namespace SourceHook;
using namespace SourceMod;

#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

struct DataTableInfo
{
	ServerClass *sc;
	Trie *lookup;
};

struct DataMapTrie
{
	DataMapTrie() : trie(NULL) {}
	Trie *trie;
};

class CHalfLife2 : 
	public SMGlobalClass,
	public IGameHelpers
{
public:
	CHalfLife2();
	~CHalfLife2();
public:
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	/*void OnSourceModAllShutdown();*/
public: //IGameHelpers
	SendProp *FindInSendTable(const char *classname, const char *offset);
	datamap_t *GetDataMap(CBaseEntity *pEntity);
	ServerClass *FindServerClass(const char *classname);
	typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset);
	void SetEdictStateChanged(edict_t *pEdict, unsigned short offset);
	bool TextMsg(int client, int dest, const char *msg);
	bool HintTextMsg(int client, const char *msg);
	bool ShowVGUIMenu(int client, const char *name, KeyValues *data, bool show);
private:
	DataTableInfo *_FindServerClass(const char *classname);
private:
	Trie *m_pClasses;
	List<DataTableInfo *> m_Tables;
	THash<datamap_t *, DataMapTrie> m_Maps;
	int m_MsgTextMsg;
	int m_HinTextMsg;
	int m_VGUIMenu;
};

extern CHalfLife2 g_HL2;
extern bool g_IsOriginalEngine;

#endif //_INCLUDE_SOURCEMOD_CHALFLIFE2_H_
