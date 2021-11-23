/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2016 AlliedModders LLC.  All rights reserved.
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
#include <am-utility.h>
#include <am-hashset.h>
#include <am-hashmap.h>
#include <sm_stringhashmap.h>
#include <sm_namehashset.h>
#include "sm_globals.h"
#include "sm_queue.h"
#include <IGameHelpers.h>
#include <KeyValues.h>
#include <server_class.h>
#include <datamap.h>
#include <ihandleentity.h>
#include <tier0/icommandline.h>
#include <string_t.h>

namespace SourceMod {
class ICommandArgs;
} // namespace SourceMod

using namespace SourceHook;
using namespace SourceMod;

#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

#if defined _WIN32
#define SOURCE_BIN_PREFIX ""
#define SOURCE_BIN_SUFFIX ""
#define SOURCE_BIN_EXT ".dll"
#elif defined __APPLE__
#define SOURCE_BIN_PREFIX ""
#define SOURCE_BIN_SUFFIX ""
#define SOURCE_BIN_EXT ".dylib"
#elif defined __linux__
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_LEFT4DEAD2 || SOURCE_ENGINE == SE_NUCLEARDAWN \
	|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_INSURGENCY || SOURCE_ENGINE == SE_DOI
#define SOURCE_BIN_PREFIX "lib"
#define SOURCE_BIN_SUFFIX "_srv"
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
#define SOURCE_BIN_PREFIX "lib"
#define SOURCE_BIN_SUFFIX ""
#else
#define SOURCE_BIN_PREFIX ""
#define SOURCE_BIN_SUFFIX "_i486"
#endif
#define SOURCE_BIN_EXT ".so"
#endif

#define FORMAT_SOURCE_BIN_NAME(basename) \
	(SOURCE_BIN_PREFIX basename SOURCE_BIN_SUFFIX SOURCE_BIN_EXT)

struct DataTableInfo
{
	struct SendPropPolicy
	{
		static inline bool matches(const char *name, const sm_sendprop_info_t &info)
		{
			return strcmp(name, info.prop->GetName()) == 0;
		}
		static inline uint32_t hash(const detail::CharsAndLength &key)
		{
			return key.hash();
		}
	};

	static inline bool matches(const char *name, const DataTableInfo *info)
	{
		return strcmp(name, info->sc->GetName()) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
	
	DataTableInfo(ServerClass *sc)
		: sc(sc)
	{
	}

	ServerClass *sc;
	NameHashSet<sm_sendprop_info_t, SendPropPolicy> lookup;
};

struct DataMapCachePolicy
{
	static inline bool matches(const char *name, const sm_datatable_info_t &info)
	{
		return strcmp(name, info.prop->fieldName) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};

typedef NameHashSet<sm_datatable_info_t, DataMapCachePolicy> DataMapCache;

struct DelayedFakeCliCmd
{
	String cmd;
	int client;
	int userid;
};

struct CachedCommandInfo
{
	const ICommandArgs *args;
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

// Corresponds to TF2's eFindMapResult in eiface.h
// Not yet in other games, but eventually in others on same branch.
enum class SMFindMapResult : cell_t {
	Found,
	NotFound,
	FuzzyMatch,
	NonCanonical,
	PossiblyAvailable
};

#if SOURCE_ENGINE >= SE_LEFT4DEAD && defined PLATFORM_WINDOWS
template< class T, class I = int >
class CUtlMemoryGlobalMalloc;
class CUtlString;
#endif

class CHalfLife2 : 
	public SMGlobalClass,
	public IGameHelpers
{
	friend class AutoEnterCommand;
public:
	CHalfLife2();
	~CHalfLife2();
public:
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModAllInitialized_Post();
	/*void OnSourceModAllShutdown();*/
	ConfigResult OnSourceModConfigChanged(const char *key, const char *value,
		ConfigSource source, char *error, size_t maxlength) override;
public: //IGameHelpers
	SendProp *FindInSendTable(const char *classname, const char *offset);
	bool FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info);
	datamap_t *GetDataMap(CBaseEntity *pEntity);
	ServerClass *FindServerClass(const char *classname);
	typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset);
	bool FindDataMapInfo(datamap_t *pMap, const char *offset, sm_datatable_info_t *pDataTable);
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
	SMFindMapResult FindMap(char *pMapName, size_t nMapNameMax);
	SMFindMapResult FindMap(const char *pMapName, char *pFoundMap = NULL, size_t nMapNameMax = 0);
#if SOURCE_ENGINE >= SE_LEFT4DEAD && defined PLATFORM_WINDOWS && SOURCE_ENGINE != SE_MOCK
	void FreeUtlVectorUtlString(CUtlVector<CUtlString, CUtlMemoryGlobalMalloc<CUtlString>> &vec);
#endif
	bool GetMapDisplayName(const char *pMapName, char *pDisplayname, size_t nMapNameMax);
	string_t AllocPooledString(const char *pszValue);
	bool GetServerSteam3Id(char *pszOut, size_t len) const override;
	uint64_t GetServerSteamId64() const override;
public:
	void AddToFakeCliCmdQueue(int client, int userid, const char *cmd);
	void ProcessFakeCliCmdQueue();
public:
	const ICommandArgs *PeekCommandStack();
	const char *CurrentCommandName();
	void AddDelayedKick(int client, int userid, const char *msg);
	void ProcessDelayedKicks();
private:
	void PushCommandStack(const ICommandArgs *cmd);
	void PopCommandStack();
	DataTableInfo *_FindServerClass(const char *classname);
private:
	void InitLogicalEntData();
	void InitCommandLine();
private:
	typedef ke::HashMap<datamap_t *, DataMapCache *, ke::PointerPolicy<datamap_t> > DataTableMap;

	NameHashSet<DataTableInfo *> m_Classes;
	DataTableMap m_Maps;
	int m_MsgTextMsg;
	int m_HinTextMsg;
	int m_SayTextMsg;
	int m_VGUIMenu;
	Queue<DelayedFakeCliCmd *> m_CmdQueue;
	CStack<DelayedFakeCliCmd *> m_FreeCmds;
	CStack<CachedCommandInfo> m_CommandStack;
	Queue<DelayedKickInfo> m_DelayedKicks;
	void *m_pGetCommandLine;
#if SOURCE_ENGINE == SE_CSGO
public:
	bool CanSetCSGOEntProp(const char *pszPropName)
	{
		return !m_bFollowCSGOServerGuidelines || !m_CSGOBadList.has(pszPropName);
	}
private:
	ke::HashSet<std::string, detail::StringHashMapPolicy> m_CSGOBadList;
	bool m_bFollowCSGOServerGuidelines = true;
#endif
};

extern CHalfLife2 g_HL2;

bool IndexToAThings(cell_t, CBaseEntity **pEntData, edict_t **pEdictData);

class AutoEnterCommand
{
public:
	AutoEnterCommand(const ICommandArgs *args) {
		g_HL2.PushCommandStack(args);
	}
	~AutoEnterCommand() {
		g_HL2.PopCommandStack();
	}
};

#endif //_INCLUDE_SOURCEMOD_CHALFLIFE2_H_
