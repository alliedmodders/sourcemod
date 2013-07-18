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

#include "HalfLife2.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "UserMessages.h"
#include "PlayerManager.h"
#include "sm_stringutil.h"
#include <IGameConfigs.h>
#include <compat_wrappers.h>
#include <Logger.h>
#include "LibrarySys.h"
#include "logic_bridge.h"
#include <tier0/mem.h>

#if SOURCE_ENGINE == SE_DOTA
#include <game/shared/protobuf/usermessages.pb.h>
#elif SOURCE_ENGINE == SE_CSGO
#include <cstrike15_usermessages.pb.h>
#endif


typedef ICommandLine *(*FakeGetCommandLine)();

#if defined _WIN32
#define TIER0_NAME			"tier0.dll"
#define VSTDLIB_NAME		"vstdlib.dll"
#elif defined __APPLE__
#define TIER0_NAME			"libtier0.dylib"
#define VSTDLIB_NAME		"libvstdlib.dylib"
#elif defined __linux__
#if SOURCE_ENGINE == SE_ORANGEBOXVALVE || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_LEFT4DEAD2
#define TIER0_NAME			"libtier0_srv.so"
#define VSTDLIB_NAME		"libvstdlib_srv.so"
#elif SOURCE_ENGINE >= SE_LEFT4DEAD
#define TIER0_NAME			"libtier0.so"
#define VSTDLIB_NAME		"libvstdlib.so"
#else
#define TIER0_NAME			"tier0_i486.so"
#define VSTDLIB_NAME		"vstdlib_i486.so"
#endif
#endif

CHalfLife2 g_HL2;
ConVar *sv_lan = NULL;

static void *g_EntList = NULL;
static int entInfoOffset = -1;

namespace SourceHook
{
	template<>
	int HashFunction<datamap_t *>(datamap_t * const &k)
	{
		return reinterpret_cast<int>(k);
	}

	template<>
	int Compare<datamap_t *>(datamap_t * const &k1, datamap_t * const &k2)
	{
		return (k1-k2);
	}
}

CHalfLife2::CHalfLife2()
{
	m_pClasses = sm_trie_create();
}

CHalfLife2::~CHalfLife2()
{
	sm_trie_destroy(m_pClasses);

	List<DataTableInfo *>::iterator iter;
	DataTableInfo *pInfo;
	for (iter=m_Tables.begin(); iter!=m_Tables.end(); iter++)
	{
		pInfo = (*iter);
		delete pInfo;
	}

	m_Tables.clear();

	THash<datamap_t *, DataMapTrie>::iterator h_iter;
	for (h_iter=m_Maps.begin(); h_iter!=m_Maps.end(); h_iter++)
	{
		if (h_iter->val.trie)
		{
			sm_trie_destroy(h_iter->val.trie);
			h_iter->val.trie = NULL;
		}
	}

	m_Maps.clear();
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;
#endif

#if !defined METAMOD_PLAPI_VERSION || PLAPI_VERSION < 11
bool is_original_engine = false;
#endif

void CHalfLife2::OnSourceModStartup(bool late)
{
#if SOURCE_ENGINE != SE_DARKMESSIAH

	/* The Ship currently is the only known game to use an older version of the engine */
#if defined METAMOD_PLAPI_VERSION || PLAPI_VERSION >= 11
	if (g_SMAPI->GetSourceEngineBuild() == SOURCE_ENGINE_ORIGINAL)
#else
	if (strcasecmp(g_SourceMod.GetGameFolderName(), "ship") == 0)
#endif
	{
		is_original_engine = true;
	}
	else if (g_pSharedChangeInfo == NULL)
	{
		g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();
	}
#endif
}

void CHalfLife2::OnSourceModAllInitialized()
{
	m_MsgTextMsg = g_UserMsgs.GetMessageIndex("TextMsg");
	m_HinTextMsg = g_UserMsgs.GetMessageIndex("HintText");
	m_SayTextMsg = g_UserMsgs.GetMessageIndex("SayText");
	m_VGUIMenu = g_UserMsgs.GetMessageIndex("VGUIMenu");
	sharesys->AddInterface(NULL, this);
}

void CHalfLife2::OnSourceModAllInitialized_Post()
{
	InitLogicalEntData();
	InitCommandLine();
}

void CHalfLife2::InitLogicalEntData()
{
	char *addr = NULL;

	/*
	 * gEntList and/or g_pEntityList
	 *
	 * First try to lookup pointer directly for platforms with symbols.
	 * If symbols aren't present (Windows or stripped Linux/Mac), 
	 * attempt find via LevelShutdown + offset
	 */
	if (g_pGameConf->GetMemSig("gEntList", (void **)&addr))
	{
#if !defined PLATFORM_WINDOWS
		if (!addr)
		{
			// Key exists so notify if lookup fails, but try other method.
			g_Logger.LogError("Failed lookup of gEntList directly - Reverting to lookup via LevelShutdown");
		}
		else
		{
#endif
			g_EntList = reinterpret_cast<void *>(addr);
#if !defined PLATFORM_WINDOWS
		}
#endif
	}


	if (!g_EntList)
	{
		if (!g_pGameConf->GetMemSig("LevelShutdown", (void **)&addr))
		{
			g_Logger.LogError("Logical Entities not supported by this mod (LevelShutdown) - Reverting to networkable entities only");
			return;
		}

		if (!addr)
		{
			g_Logger.LogError("Failed lookup of LevelShutdown - Reverting to networkable entities only");
			return;
		}

		int offset;
		if (!g_pGameConf->GetOffset("gEntList", &offset))
		{
			g_Logger.LogError("Logical Entities not supported by this mod (gEntList) - Reverting to networkable entities only");
			return;
		}

		g_EntList = *reinterpret_cast<void **>(addr + offset);
	}
	
	if (!g_EntList)
	{
		g_Logger.LogError("Failed lookup of gEntList - Reverting to networkable entities only");
		return;
	}

	if (!g_pGameConf->GetOffset("EntInfo", &entInfoOffset))
	{
		g_Logger.LogError("Logical Entities not supported by this mod (EntInfo) - Reverting to networkable entities only");
		return;
	}
}

void CHalfLife2::InitCommandLine()
{
	char path[PLATFORM_MAX_PATH];
	char error[256];
	
	g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "../bin/" TIER0_NAME);

	if (!g_LibSys.IsPathFile(path))
	{
		g_Logger.LogError("Could not find path for: " TIER0_NAME);
		return;
	}

	ILibrary *lib = g_LibSys.OpenLibrary(path, error, sizeof(error));
	m_pGetCommandLine = lib->GetSymbolAddress("CommandLine_Tier0");

	/* '_Tier0' dropped on Alien Swarm version */
	if (m_pGetCommandLine == NULL)
	{
		m_pGetCommandLine = lib->GetSymbolAddress("CommandLine");
	}

	if (m_pGetCommandLine == NULL)
	{
		/* We probably have a Ship engine. */
		lib->CloseLibrary();
		g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "../bin/" VSTDLIB_NAME);
		if (!g_LibSys.IsPathFile(path))
		{
			g_Logger.LogError("Could not find path for: " VSTDLIB_NAME);
			return;
		}

		if ((lib = g_LibSys.OpenLibrary(path, error, sizeof(error))) == NULL)
		{
			g_Logger.LogError("Could not load %s: %s", path, error);
			return;
		}

		m_pGetCommandLine = lib->GetSymbolAddress("CommandLine");

		if (m_pGetCommandLine == NULL)
		{
			g_Logger.LogError("Could not locate any command line functionality");
		}

		lib->CloseLibrary();
	}
}

ICommandLine *CHalfLife2::GetValveCommandLine()
{
	if (!m_pGetCommandLine)
		return NULL;

	return ((FakeGetCommandLine)((FakeGetCommandLine *)m_pGetCommandLine))();
}

#if !defined METAMOD_PLAPI_VERSION || PLAPI_VERSION < 11
bool CHalfLife2::IsOriginalEngine()
{
	return is_original_engine;
}
#endif

#if SOURCE_ENGINE != SE_DARKMESSIAH
IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
#if SOURCE_ENGINE == SE_DOTA
	return engine->GetChangeAccessor( IndexOfEdict((const edict_t *)this) );
#else
	return engine->GetChangeAccessor( (const edict_t *)this );
#endif
}
#endif

bool UTIL_FindInSendTable(SendTable *pTable, 
						  const char *name,
						  sm_sendprop_info_t *info,
						  unsigned int offset)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		pname = prop->GetName();
		if (pname && strcmp(name, pname) == 0)
		{
			info->prop = prop;
			info->actual_offset = offset + info->prop->GetOffset();
			return true;
		}
		if (prop->GetDataTable())
		{
			if (UTIL_FindInSendTable(prop->GetDataTable(), 
				name,
				info,
				offset + prop->GetOffset())
				)
			{
				return true;
			}
		}
	}

	return false;
}

typedescription_t *UTIL_FindInDataMap(datamap_t *pMap, const char *name, bool *isNested)
{
	if (isNested)
	{
		*isNested = false;
	}
	
	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
			{
				continue;
			}
			if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
			{
				return &(pMap->dataDesc[i]);
			}
			if (pMap->dataDesc[i].td)
			{
				if (isNested)
				{
					*isNested = (UTIL_FindInDataMap(pMap->dataDesc[i].td, name, NULL) != NULL);
					if (*isNested)
					{
						return NULL;
					} else {
						continue;
					}
				} else { // Use the old behaviour, we dont want to spring this on extensions - even if they're doing bad things.
					typedescription_t *_td;
					if ((_td=UTIL_FindInDataMap(pMap->dataDesc[i].td, name, NULL)) != NULL)
					{
						return _td;
					}
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return NULL; 
}

ServerClass *CHalfLife2::FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = _FindServerClass(classname);

	if (!pInfo)
	{
		return NULL;
	}

	return pInfo->sc;
}

DataTableInfo *CHalfLife2::_FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = NULL;

	if (!sm_trie_retrieve(m_pClasses, classname, (void **)&pInfo))
	{
		ServerClass *sc = gamedll->GetAllServerClasses();
		while (sc)
		{
			if (strcmp(classname, sc->GetName()) == 0)
			{
				pInfo = new DataTableInfo;
				pInfo->sc = sc;
				sm_trie_insert(m_pClasses, classname, pInfo);
				m_Tables.push_back(pInfo);
				break;
			}
			sc = sc->m_pNext;
		}
		if (!pInfo)
		{
			return NULL;
		}
	}

	return pInfo;
}

bool CHalfLife2::FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info)
{
	DataTableInfo *pInfo;
	sm_sendprop_info_t *prop;

	if ((pInfo = _FindServerClass(classname)) == NULL)
	{
		return false;
	}

	if ((prop = pInfo->lookup.retrieve(offset)) == NULL)
	{
		sm_sendprop_info_t temp_info;

		if (!UTIL_FindInSendTable(pInfo->sc->m_pTable, offset, &temp_info, 0))
		{
			return false;
		}

		pInfo->lookup.insert(offset, temp_info);
		*info = temp_info;
	}
	else
	{
		*info = *prop;
	}
	
	return true;
}

SendProp *CHalfLife2::FindInSendTable(const char *classname, const char *offset)
{
	sm_sendprop_info_t info;

	if (!FindSendPropInfo(classname, offset, &info))
	{
		return NULL;
	}

	return info.prop;
}

typedescription_t *CHalfLife2::FindInDataMap(datamap_t *pMap, const char *offset)
{
	return this->FindInDataMap(pMap, offset, NULL);
}

typedescription_t *CHalfLife2::FindInDataMap(datamap_t *pMap, const char *offset, bool *isNested)
{
	typedescription_t *td = NULL;
	DataMapTrie &val = m_Maps[pMap];

	if (!val.trie)
	{
		val.trie = sm_trie_create();
	}
	if (!sm_trie_retrieve(val.trie, offset, (void **)&td))
	{
		if ((td = UTIL_FindInDataMap(pMap, offset, isNested)) != NULL)
		{
			sm_trie_insert(val.trie, offset, td);
		}
	}

	return td;
}

void CHalfLife2::SetEdictStateChanged(edict_t *pEdict, unsigned short offset)
{
#if SOURCE_ENGINE != SE_DARKMESSIAH
	if (g_pSharedChangeInfo != NULL)
	{
		if (offset)
		{
			pEdict->StateChanged(offset);
		}
		else
		{
			pEdict->StateChanged();
		}
	}
	else
#endif
	{
		pEdict->m_fStateFlags |= FL_EDICT_CHANGED;
	}
}

bool CHalfLife2::TextMsg(int client, int dest, const char *msg)
{
#ifndef USE_PROTOBUF_USERMESSAGES
	bf_write *pBitBuf = NULL;
#endif
	cell_t players[] = {client};

	if (dest == HUD_PRINTTALK)
	{
		const char *chat_saytext = g_pGameConf->GetKeyValue("ChatSayText");

		/* Use SayText user message instead */
		if (chat_saytext != NULL && strcmp(chat_saytext, "yes") == 0)
		{
			char buffer[192];
			UTIL_Format(buffer, sizeof(buffer), "%s\1\n", msg);

#if SOURCE_ENGINE == SE_DOTA
			CUserMsg_SayText *pMsg;
			if ((pMsg = (CUserMsg_SayText *)g_UserMsgs.StartProtobufMessage(m_SayTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
			{
				return false;
			}

			pMsg->set_client(0);
			pMsg->set_text(buffer);
			pMsg->set_chat(false);
#elif SOURCE_ENGINE == SE_CSGO
			CCSUsrMsg_SayText *pMsg;
			if ((pMsg = (CCSUsrMsg_SayText *)g_UserMsgs.StartProtobufMessage(m_SayTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
			{
				return false;
			}

			pMsg->set_ent_idx(0);
			pMsg->set_text(buffer);
			pMsg->set_chat(false);
#else
			if ((pBitBuf = g_UserMsgs.StartBitBufMessage(m_SayTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
			{
				return false;
			}

			pBitBuf->WriteByte(0);
			pBitBuf->WriteString(buffer);
			pBitBuf->WriteByte(1);
#endif

			g_UserMsgs.EndMessage();

			return true;
		}
	}

#if SOURCE_ENGINE == SE_DOTA
	CUserMsg_TextMsg *pMsg;
	if ((pMsg = (CUserMsg_TextMsg *)g_UserMsgs.StartProtobufMessage(m_MsgTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pMsg->set_dest(dest);
	pMsg->add_param(msg);
	pMsg->add_param("");
	pMsg->add_param("");
	pMsg->add_param("");
	pMsg->add_param("");
#elif SOURCE_ENGINE == SE_CSGO
	CCSUsrMsg_TextMsg *pMsg;
	if ((pMsg = (CCSUsrMsg_TextMsg *)g_UserMsgs.StartProtobufMessage(m_MsgTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	// Client tries to read all 5 'params' and will crash if less
	pMsg->set_msg_dst(dest);
	pMsg->add_params(msg);
	pMsg->add_params("");
	pMsg->add_params("");
	pMsg->add_params("");
	pMsg->add_params("");
#else
	if ((pBitBuf = g_UserMsgs.StartBitBufMessage(m_MsgTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pBitBuf->WriteByte(dest);
	pBitBuf->WriteString(msg);
#endif

	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::HintTextMsg(int client, const char *msg)
{
	cell_t players[] = {client};

#if SOURCE_ENGINE == SE_DOTA
	CUserMsg_HintText *pMsg;
	if ((pMsg = (CUserMsg_HintText *)g_UserMsgs.StartProtobufMessage(m_HinTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pMsg->set_message(msg);
#elif SOURCE_ENGINE == SE_CSGO
	CCSUsrMsg_HintText *pMsg;
	if ((pMsg = (CCSUsrMsg_HintText *)g_UserMsgs.StartProtobufMessage(m_HinTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pMsg->set_text(msg);
#else
	bf_write *pBitBuf = NULL;

	if ((pBitBuf = g_UserMsgs.StartBitBufMessage(m_HinTextMsg, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	const char *pre_byte = g_pGameConf->GetKeyValue("HintTextPreByte");
	if (pre_byte != NULL && strcmp(pre_byte, "yes") == 0)
	{
		pBitBuf->WriteByte(1);
	}
	pBitBuf->WriteString(msg);
#endif
	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::HintTextMsg(cell_t *players, int count, const char *msg)
{
#if SOURCE_ENGINE == SE_DOTA
	CUserMsg_HintText *pMsg;
	if ((pMsg = (CUserMsg_HintText *)g_UserMsgs.StartProtobufMessage(m_HinTextMsg, players, count, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pMsg->set_message(msg);
#elif SOURCE_ENGINE == SE_CSGO
	CCSUsrMsg_HintText *pMsg;
	if ((pMsg = (CCSUsrMsg_HintText *)g_UserMsgs.StartProtobufMessage(m_HinTextMsg, players, count, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	pMsg->set_text(msg);
#else
	bf_write *pBitBuf = NULL;

	if ((pBitBuf = g_UserMsgs.StartBitBufMessage(m_HinTextMsg, players, count, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}

	const char *pre_byte = g_pGameConf->GetKeyValue("HintTextPreByte");
	if (pre_byte != NULL && strcmp(pre_byte, "yes") == 0)
	{
		pBitBuf->WriteByte(1);
	}
	pBitBuf->WriteString(msg);
#endif

	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::ShowVGUIMenu(int client, const char *name, KeyValues *data, bool show)
{
	KeyValues *SubKey = NULL;
	int count = 0;
	cell_t players[] = {client};

#if SOURCE_ENGINE == SE_DOTA
	CUserMsg_VGUIMenu *pMsg;
	if ((pMsg = (CUserMsg_VGUIMenu *)g_UserMsgs.StartProtobufMessage(m_VGUIMenu, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}
#elif SOURCE_ENGINE == SE_CSGO
	CCSUsrMsg_VGUIMenu *pMsg;
	if ((pMsg = (CCSUsrMsg_VGUIMenu *)g_UserMsgs.StartProtobufMessage(m_VGUIMenu, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}
#else
	bf_write *pBitBuf = NULL;
	if ((pBitBuf = g_UserMsgs.StartBitBufMessage(m_VGUIMenu, players, 1, USERMSG_RELIABLE)) == NULL)
	{
		return false;
	}
#endif

	if (data)
	{
		SubKey = data->GetFirstSubKey();
		while (SubKey)
		{
			count++;
			SubKey = SubKey->GetNextKey();
		}
		SubKey = data->GetFirstSubKey();
	}

#if SOURCE_ENGINE == SE_DOTA
	pMsg->set_name(name);
	pMsg->set_show(show);

	while (SubKey)
	{
		CUserMsg_VGUIMenu_Keys *key = pMsg->add_keys();
		key->set_name(SubKey->GetName());
		key->set_value(SubKey->GetString());
		SubKey = SubKey->GetNextKey();
	}
#elif SOURCE_ENGINE == SE_CSGO
	pMsg->set_name(name);
	pMsg->set_show(show);

	while (SubKey)
	{
		CCSUsrMsg_VGUIMenu_Subkey *key = pMsg->add_subkeys();
		key->set_name(SubKey->GetName());
		key->set_str(SubKey->GetString());
		SubKey = SubKey->GetNextKey();
	}
#else
	pBitBuf->WriteString(name);
	pBitBuf->WriteByte((show) ? 1 : 0);
	pBitBuf->WriteByte(count);
	while (SubKey)
	{
		pBitBuf->WriteString(SubKey->GetName());
		pBitBuf->WriteString(SubKey->GetString());
		SubKey = SubKey->GetNextKey();
	}
#endif

	g_UserMsgs.EndMessage();

	return true;
}

void CHalfLife2::AddToFakeCliCmdQueue(int client, int userid, const char *cmd)
{
	DelayedFakeCliCmd *pFake;

	if (m_FreeCmds.empty())
	{
		pFake = new DelayedFakeCliCmd;
	} else {
		pFake = m_FreeCmds.front();
		m_FreeCmds.pop();
	}

	pFake->client = client;
	pFake->userid = userid;
	pFake->cmd.assign(cmd);

	m_CmdQueue.push(pFake);
}

void CHalfLife2::ProcessFakeCliCmdQueue()
{
	while (!m_CmdQueue.empty())
	{
		DelayedFakeCliCmd *pFake = m_CmdQueue.first();

		if (g_Players.GetClientOfUserId(pFake->userid) == pFake->client)
		{
			CPlayer *pPlayer = g_Players.GetPlayerByIndex(pFake->client);
#if SOURCE_ENGINE == SE_DOTA
			engine->ClientCommand(pPlayer->GetIndex(), "%s", pFake->cmd.c_str());
#else
			serverpluginhelpers->ClientCommand(pPlayer->GetEdict(), pFake->cmd.c_str());
#endif
		}

		m_CmdQueue.pop();
	}
}

bool CHalfLife2::IsLANServer()
{
	sv_lan = icvar->FindVar("sv_lan");

	if (!sv_lan)
	{
		return false;
	}

	return (sv_lan->GetInt() != 0);
}

bool CHalfLife2::KVLoadFromFile(KeyValues *kv, IBaseFileSystem *filesystem, const char *resourceName, const char *pathID)
{
#if defined METAMOD_PLAPI_VERSION || PLAPI_VERSION >= 11
	if (g_SMAPI->GetSourceEngineBuild() == SOURCE_ENGINE_ORIGINAL)
#else
	if (strcasecmp(g_SourceMod.GetGameFolderName(), "ship") == 0)
#endif
	{
		Assert(filesystem);
#ifdef _MSC_VER
		Assert(_heapchk() == _HEAPOK);
#endif

		FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);
		if (!f)
			return false;

		// load file into a null-terminated buffer
		int fileSize = filesystem->Size(f);
		char *buffer = (char *)MemAllocScratch(fileSize + 1);

		Assert(buffer);

		filesystem->Read(buffer, fileSize, f); // read into local buffer

		buffer[fileSize] = 0; // null terminate file as EOF

		filesystem->Close( f );	// close file after reading

		bool retOK = kv->LoadFromBuffer( resourceName, buffer, filesystem );

		MemFreeScratch();

		return retOK;
	}
	else
	{
		return kv->LoadFromFile(filesystem, resourceName, pathID);
	}
}

void CHalfLife2::PushCommandStack(const CCommand *cmd)
{
	CachedCommandInfo info;

	info.args = cmd;
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	strncopy(info.cmd, cmd->Arg(0), sizeof(info.cmd));
#endif

	m_CommandStack.push(info);
}

const CCommand *CHalfLife2::PeekCommandStack()
{
	if (m_CommandStack.empty())
	{
		return NULL;
	}

	return m_CommandStack.front().args;
}

void CHalfLife2::PopCommandStack()
{
	m_CommandStack.pop();
}

const char *CHalfLife2::CurrentCommandName()
{
#if SOURCE_ENGINE >= SE_ORANGEBOX
	return m_CommandStack.front().args->Arg(0);
#else
	return m_CommandStack.front().cmd;
#endif
}

void CHalfLife2::AddDelayedKick(int client, int userid, const char *msg)
{
	CPlayer *pPlayer = g_Players.GetPlayerByIndex(client);
	if (!pPlayer || !pPlayer->IsConnected() || pPlayer->IsInKickQueue())
	{
		return;
	}
	
	pPlayer->MarkAsBeingKicked();

	DelayedKickInfo kick;

	kick.client = client;
	kick.userid = userid;
	UTIL_Format(kick.buffer, sizeof(kick.buffer), "%s", msg);

	m_DelayedKicks.push(kick);
}

void CHalfLife2::ProcessDelayedKicks()
{
	while (!m_DelayedKicks.empty())
	{
		DelayedKickInfo info = m_DelayedKicks.first();
		m_DelayedKicks.pop();

		CPlayer *player = g_Players.GetPlayerByIndex(info.client);
		if (player == NULL || player->GetUserId() != info.userid)
		{
			continue;
		}

		player->Kick(info.buffer);
	}
}

edict_t *CHalfLife2::EdictOfIndex(int index)
{
	return ::PEntityOfEntIndex(index);
}

int CHalfLife2::IndexOfEdict(edict_t *pEnt)
{
	return ::IndexOfEdict(pEnt);
}

edict_t *CHalfLife2::GetHandleEntity(CBaseHandle &hndl)
{
	if (!hndl.IsValid())
	{
		return NULL;
	}

	int index = hndl.GetEntryIndex();

	edict_t *pStoredEdict;
	CBaseEntity *pStoredEntity;

	if (!IndexToAThings(index, &pStoredEntity, &pStoredEdict))
	{
		return NULL;
	}

	if (pStoredEdict == NULL || pStoredEntity == NULL)
	{
		return NULL;
	}

	IServerEntity *pSE = pStoredEdict->GetIServerEntity();

	if (pSE == NULL)
	{
		return NULL;
	}

	if (pSE->GetRefEHandle() != hndl)
	{
		return NULL;
	}

	return pStoredEdict;
}

void CHalfLife2::SetHandleEntity(CBaseHandle &hndl, edict_t *pEnt)
{
	IServerEntity *pEntOther = pEnt->GetIServerEntity();

	if (pEntOther == NULL)
	{
		return;
	}

	hndl.Set(pEntOther);
}

const char *CHalfLife2::GetCurrentMap()
{
	return STRING(gpGlobals->mapname);
}

void CHalfLife2::ServerCommand(const char *buffer)
{
	engine->ServerCommand(buffer);
}

cell_t CHalfLife2::EntityToReference(CBaseEntity *pEntity)
{
	IServerUnknown *pUnknown = (IServerUnknown *)pEntity;
	CBaseHandle hndl = pUnknown->GetRefEHandle();
	return (hndl.ToInt() | (1<<31));
}

CBaseEntity *CHalfLife2::ReferenceToEntity(cell_t entRef)
{
	if ((unsigned)entRef == INVALID_EHANDLE_INDEX)
	{
		return NULL;
	}

	CEntInfo *pInfo = NULL;

	if (entRef & (1<<31))
	{
		/* Proper ent reference */
		int hndlValue = entRef & ~(1<<31);
		CBaseHandle hndl(hndlValue);

		pInfo = LookupEntity(hndl.GetEntryIndex());
		if (!pInfo || pInfo->m_SerialNumber != hndl.GetSerialNumber())
		{
			return NULL;
		}
	}
	else
	{
		/* Old style index only */
		pInfo = LookupEntity(entRef);
	}

	if (!pInfo)
	{
		return NULL;
	}

	IServerUnknown *pUnk = static_cast<IServerUnknown *>(pInfo->m_pEntity);
	if (pUnk)
	{
		return pUnk->GetBaseEntity();
	}

	return NULL;
}

/**
 * Retrieves the CEntInfo pointer from g_EntList for a given entity index
 */
CEntInfo *CHalfLife2::LookupEntity(int entIndex)
{
	// Make sure that our index is within the bounds of the global ent array
	if (entIndex < 0 || entIndex >= NUM_ENT_ENTRIES)
	{
		return NULL;
	}

	if (!g_EntList || entInfoOffset == -1)
	{
		/* Attempt to use engine interface instead */
		static CEntInfo tempInfo;
		tempInfo.m_pNext = NULL;
		tempInfo.m_pPrev = NULL;

		edict_t *pEdict = PEntityOfEntIndex(entIndex);

		if (!pEdict)
		{
			return NULL;
		}

		IServerUnknown *pUnk = pEdict->GetUnknown();

		if (!pUnk)
		{
			return NULL;
		}

		tempInfo.m_pEntity = pUnk;
		tempInfo.m_SerialNumber = pUnk->GetRefEHandle().GetSerialNumber();

		return &tempInfo;
	}

	CEntInfo *pArray = (CEntInfo *)(((unsigned char *)g_EntList) + entInfoOffset);
	return &pArray[entIndex];
}

/**
	SERIAL_MASK = 0x7fff (15 bits)

	#define	MAX_EDICT_BITS				11
	#define NUM_ENT_ENTRY_BITS		(MAX_EDICT_BITS + 1)
	m_Index = iEntry | (iSerialNumber << NUM_ENT_ENTRY_BITS); 
	
	Top 5 bits of a handle are unused.
	Bit 31 - Our 'reference' flag indicator
*/

cell_t CHalfLife2::IndexToReference(int entIndex)
{
	CBaseEntity *pEnt = ReferenceToEntity(entIndex);
	if (!pEnt)
	{
		return INVALID_EHANDLE_INDEX;
	}

	return EntityToReference(pEnt);
}

int CHalfLife2::ReferenceToIndex(cell_t entRef)
{
	if ((unsigned)entRef == INVALID_EHANDLE_INDEX)
	{
		return INVALID_EHANDLE_INDEX;
	}

	if (entRef & (1<<31))
	{
		/* Proper ent reference */
		int hndlValue = entRef & ~(1<<31);
		CBaseHandle hndl(hndlValue);

		CEntInfo *pInfo = LookupEntity(hndl.GetEntryIndex());

		if (pInfo->m_SerialNumber != hndl.GetSerialNumber())
		{
			return INVALID_EHANDLE_INDEX;
		}

		return hndl.GetEntryIndex();
	}

	return entRef;
}

cell_t CHalfLife2::EntityToBCompatRef(CBaseEntity *pEntity)
{
	if (pEntity == NULL)
	{
		return INVALID_EHANDLE_INDEX;
	}

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity;
	CBaseHandle hndl = pUnknown->GetRefEHandle();

	if (hndl == INVALID_EHANDLE_INDEX)
	{
		return INVALID_EHANDLE_INDEX;
	}
	
	if (hndl.GetEntryIndex() >= MAX_EDICTS)
	{
		return (hndl.ToInt() | (1<<31));
	}
	else
	{
		return hndl.GetEntryIndex();
	}
}

cell_t CHalfLife2::ReferenceToBCompatRef(cell_t entRef)
{
	if ((unsigned)entRef == INVALID_EHANDLE_INDEX)
	{
		return INVALID_EHANDLE_INDEX;
	}

	int hndlValue = entRef & ~(1<<31);
	CBaseHandle hndl(hndlValue);

	if (hndl.GetEntryIndex() < MAX_EDICTS)
	{
		return hndl.GetEntryIndex();
	}

	return entRef;
}

void *CHalfLife2::GetGlobalEntityList()
{
	return g_EntList;
}

int CHalfLife2::GetSendPropOffset(SendProp *prop)
{
	return prop->GetOffset();
}

const char *CHalfLife2::GetEntityClassname(edict_t * pEdict)
{
	if (pEdict == NULL || pEdict->IsFree())
	{
		return NULL;
	}
	
	IServerUnknown *pUnk = pEdict->GetUnknown();
	if (pUnk == NULL)
	{
		return NULL;
	}
	
	CBaseEntity * pEntity = pUnk->GetBaseEntity();
	
	if (pEntity == NULL)
	{
		return NULL;
	}
	
	return GetEntityClassname(pEntity);
}

const char *CHalfLife2::GetEntityClassname(CBaseEntity *pEntity)
{
	static int offset = -1;
	if (offset == -1)
	{
		CBaseEntity *pGetterEnt = ReferenceToEntity(0);
		datamap_t *pMap = GetDataMap(pGetterEnt);
		typedescription_t *pDesc = FindInDataMap(pMap, "m_iClassname");
		offset = GetTypeDescOffs(pDesc);
	}

	return *(const char **)(((unsigned char *)pEntity) + offset);
}

#if SOURCE_ENGINE >= SE_LEFT4DEAD
static bool ResolveFuzzyMapName(const char *fuzzyName, char *outFullname, int size)
{
	static ConCommand *pHelperCmd = g_pCVar->FindCommand("changelevel");
	if (!pHelperCmd || !pHelperCmd->CanAutoComplete())
		return false;

	static size_t helperCmdLen = strlen(pHelperCmd->GetName());

	CUtlVector<CUtlString> results;
	pHelperCmd->AutoCompleteSuggest(fuzzyName, results);
	if (results.Count() == 0)
		return false;

	// Results come back as you'd see in autocomplete. (ie. "changelevel fullmapnamehere"),
	// so skip ahead to start of map path/name

	// Like the engine, we're only going to deal with the first match.

	strncopy(outFullname, &results[0][helperCmdLen + 1], size);

	return true;
}
#endif

bool CHalfLife2::IsMapValid(const char *map)
{
	if (!map || !map[0])
		return false;

	bool ret = engine->IsMapValid(map);
#if SOURCE_ENGINE >= SE_LEFT4DEAD
	if (!ret)
	{
		static char szFuzzyName[PLATFORM_MAX_PATH];
		if (ResolveFuzzyMapName(map, szFuzzyName, sizeof(szFuzzyName)))
		{
			ret = engine->IsMapValid(szFuzzyName);
		}
	}
#endif
	return ret;
}
