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

#ifndef _INCLUDE_SOURCEMOD_CHALFLIFE2_H_
#define _INCLUDE_SOURCEMOD_CHALFLIFE2_H_

#include <sh_list.h>
#include <sh_tinyhash.h>
#include "sm_trie.h"
#include "sm_globals.h"
#include <IGameHelpers.h>

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
	void TextMsg(int client, int dest, const char *msg);
private:
	DataTableInfo *_FindServerClass(const char *classname);
private:
	Trie *m_pClasses;
	List<DataTableInfo *> m_Tables;
	THash<datamap_t *, DataMapTrie> m_Maps;
	int m_MsgTextMsg;
};

extern CHalfLife2 g_HL2;
extern bool g_IsOriginalEngine;

#endif //_INCLUDE_SOURCEMOD_CHALFLIFE2_H_
