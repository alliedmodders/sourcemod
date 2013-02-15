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

#ifndef _INCLUDE_SOURCEMOD_CHALFLIFE2_H_
#define _INCLUDE_SOURCEMOD_CHALFLIFE2_H_

#include <sh_list.h>
#include <sh_string.h>
#include <sh_tinyhash.h>
#include <sm_trie_tpl.h>
#include "sm_trie.h"
#include "sm_globals.h"
#include "sm_queue.h"
#include <IGameHelpers.h>
#include <KeyValues.h>
#include <server_class.h>
#include <datamap.h>
#include <ihandleentity.h>
#include <tier0/icommandline.h>
#if SOURCE_ENGINE >= SE_PORTAL2
#include <string_t.h>
#endif

class CCommand;

using namespace SourceHook;
using namespace SourceMod;

#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

struct DataTableInfo
{
	ServerClass *sc;
	KTrie<sm_sendprop_info_t> lookup;
};

struct DataMapTrie
{
	DataMapTrie() : trie(NULL) {}
	Trie *trie;
};

struct DelayedFakeCliCmd
{
	String cmd;
	int client;
	int userid;
};

struct CachedCommandInfo
{
	const CCommand *args;
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	char cmd[300];
#endif
};

struct DelayedKickInfo
{
	int userid;
	int client;
	char buffer[384];
};

// copy from game/shared/entitylist_base.h
class CEntInfo
{
public:
	IHandleEntity	*m_pEntity;
	int				m_SerialNumber;
	CEntInfo		*m_pPrev;
	CEntInfo		*m_pNext;
#if SOURCE_ENGINE >= SE_PORTAL2
	string_t		m_iName;
	string_t		m_iClassName;
#endif
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
	void OnSourceModAllInitialized_Post();
	/*void OnSourceModAllShutdown();*/
public: //IGameHelpers
	SendProp *FindInSendTable(const char *classname, const char *offset);
	bool FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info);
	datamap_t *GetDataMap(CBaseEntity *pEntity);
	ServerClass *FindServerClass(const char *classname);
	typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset);
	void SetEdictStateChanged(edict_t *pEdict, unsigned short offset);
	bool TextMsg(int client, int dest, const char *msg);
	bool HintTextMsg(int client, const char *msg);
	bool HintTextMsg(cell_t *players, int count, const char *msg);
	bool ShowVGUIMenu(int client, const char *name, KeyValues *data, bool show);
	bool IsLANServer();
	bool KVLoadFromFile(KeyValues *kv, IBaseFileSystem *filesystem, const char *resourceName, const char *pathID = NULL);
	edict_t *EdictOfIndex(int index);
	int IndexOfEdict(edict_t *pEnt);
	edict_t *GetHandleEntity(CBaseHandle &hndl);
	void SetHandleEntity(CBaseHandle &hndl, edict_t *pEnt);
	const char *GetCurrentMap();
	void ServerCommand(const char *buffer);
	CBaseEntity *ReferenceToEntity(cell_t entRef);
	cell_t EntityToReference(CBaseEntity *pEntity);
	cell_t IndexToReference(int entIndex);
	int ReferenceToIndex(cell_t entRef);
	cell_t ReferenceToBCompatRef(cell_t entRef);
	CEntInfo *LookupEntity(int entIndex);
	cell_t EntityToBCompatRef(CBaseEntity *pEntity);
	void *GetGlobalEntityList();
	int GetSendPropOffset(SendProp *prop);
	ICommandLine *GetValveCommandLine();
	const char *GetEntityClassname(edict_t *pEdict);
	const char *GetEntityClassname(CBaseEntity *pEntity);
	bool IsMapValid(const char *map);
public:
	void AddToFakeCliCmdQueue(int client, int userid, const char *cmd);
	void ProcessFakeCliCmdQueue();
public:
	void PushCommandStack(const CCommand *cmd);
	void PopCommandStack();
	const CCommand *PeekCommandStack();
	const char *CurrentCommandName();
	void AddDelayedKick(int client, int userid, const char *msg);
	void ProcessDelayedKicks();
#if !defined METAMOD_PLAPI_VERSION || PLAPI_VERSION < 11
	bool IsOriginalEngine();
#endif
private:
	DataTableInfo *_FindServerClass(const char *classname);
private:
	void InitLogicalEntData();
	void InitCommandLine();
private:
	Trie *m_pClasses;
	List<DataTableInfo *> m_Tables;
	THash<datamap_t *, DataMapTrie> m_Maps;
	int m_MsgTextMsg;
	int m_HinTextMsg;
	int m_SayTextMsg;
	int m_VGUIMenu;
	Queue<DelayedFakeCliCmd *> m_CmdQueue;
	CStack<DelayedFakeCliCmd *> m_FreeCmds;
	CStack<CachedCommandInfo> m_CommandStack;
	Queue<DelayedKickInfo> m_DelayedKicks;
	void *m_pGetCommandLine;
};

extern CHalfLife2 g_HL2;

bool IndexToAThings(cell_t, CBaseEntity **pEntData, edict_t **pEdictData);

#endif //_INCLUDE_SOURCEMOD_CHALFLIFE2_H_
