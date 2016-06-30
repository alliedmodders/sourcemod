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

#include "HalfLife2.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "UserMessages.h"
#include "PlayerManager.h"
#include "sm_stringutil.h"
#include <IGameConfigs.h>
#include <compat_wrappers.h>
#include <Logger.h>
#include <amtl/os/am-shared-library.h>
#include "logic_bridge.h"
#include <tier0/mem.h>
#include <bridge/include/ILogger.h>

#if SOURCE_ENGINE == SE_CSGO
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
#if SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_LEFT4DEAD2 || SOURCE_ENGINE == SE_NUCLEARDAWN \
	|| SOURCE_ENGINE == SE_BMS
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
static void **g_pEntInfoList = NULL;
static int entInfoOffset = -1;

static CEntInfo *EntInfoArray()
{
	if (g_EntList != NULL)
	{
		return (CEntInfo *)((intp)g_EntList + entInfoOffset);
	}
	else if (g_pEntInfoList)
	{
		return *(CEntInfo **)g_pEntInfoList;
	}
	
	return NULL;
}

CHalfLife2::CHalfLife2()
{
	m_Maps.init();

	m_pGetCommandLine = NULL;
}

CHalfLife2::~CHalfLife2()
{
	for (NameHashSet<DataTableInfo *>::iterator iter = m_Classes.iter(); !iter.empty(); iter.next())
		delete *iter;

	for (DataTableMap::iterator iter = m_Maps.iter(); !iter.empty(); iter.next())
		delete iter->value;
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;
#endif

void CHalfLife2::OnSourceModStartup(bool late)
{
#if SOURCE_ENGINE != SE_DARKMESSIAH
	if (g_SMAPI->GetSourceEngineBuild() != SOURCE_ENGINE_ORIGINAL && !g_pSharedChangeInfo)
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
#if SOURCE_ENGINE == SE_CSGO
	m_CSGOBadList.init();
	m_CSGOBadList.add("m_iItemDefinitionIndex");
	m_CSGOBadList.add("m_iEntityLevel");
	m_CSGOBadList.add("m_iItemIDHigh");
	m_CSGOBadList.add("m_iItemIDLow");
	m_CSGOBadList.add("m_iAccountID");
	m_CSGOBadList.add("m_iEntityQuality");
	m_CSGOBadList.add("m_bInitialized");
	m_CSGOBadList.add("m_szCustomName");
	m_CSGOBadList.add("m_iAttributeDefinitionIndex");
	m_CSGOBadList.add("m_iRawValue32");
	m_CSGOBadList.add("m_iRawInitialValue32");
	m_CSGOBadList.add("m_nRefundableCurrency");
	m_CSGOBadList.add("m_bSetBonus");
	m_CSGOBadList.add("m_OriginalOwnerXuidLow");
	m_CSGOBadList.add("m_OriginalOwnerXuidHigh");
	m_CSGOBadList.add("m_nFallbackPaintKit");
	m_CSGOBadList.add("m_nFallbackSeed");
	m_CSGOBadList.add("m_flFallbackWear");
	m_CSGOBadList.add("m_nFallbackStatTrak");
	m_CSGOBadList.add("m_iCompetitiveRanking");
	m_CSGOBadList.add("m_nActiveCoinRank");
	m_CSGOBadList.add("m_nMusicID");
#endif
}

ConfigResult CHalfLife2::OnSourceModConfigChanged(const char *key, const char *value,
	ConfigSource source, char *error, size_t maxlength)
{
	if (strcasecmp(key, "FollowCSGOServerGuidelines") == 0)
	{
#if SOURCE_ENGINE == SE_CSGO
		if (strcasecmp(value, "no") == 0)
		{
			m_bFollowCSGOServerGuidelines = false;
		}
		else if (strcasecmp(value, "yes") == 0)
		{
			m_bFollowCSGOServerGuidelines = true;
		}
		else
		{
			return ConfigResult_Reject;
		}
#endif
	}

	return ConfigResult_Ignore;
}

void CHalfLife2::InitLogicalEntData()
{
#if SOURCE_ENGINE == SE_TF2        \
	|| SOURCE_ENGINE == SE_DODS    \
	|| SOURCE_ENGINE == SE_HL2DM   \
	|| SOURCE_ENGINE == SE_CSS     \
	|| SOURCE_ENGINE == SE_SDK2013 \
	|| SOURCE_ENGINE == SE_BMS     \
	|| SOURCE_ENGINE == SE_NUCLEARDAWN

	if (g_SMAPI->GetServerFactory(false)("VSERVERTOOLS003", nullptr))
	{
		g_EntList = servertools->GetEntityList();
	}
#endif

	char *addr = NULL;

	/*
	 * gEntList and/or g_pEntityList
	 *
	 * First try to lookup pointer directly for platforms with symbols.
	 * If symbols aren't present (Windows or stripped Linux/Mac), 
	 * attempt find via LevelShutdown + offset
	 */
	if (!g_EntList)
	{
		if (g_pGameConf->GetMemSig("gEntList", (void **) &addr))
		{
#if !defined PLATFORM_WINDOWS
			if (!addr)
			{
				// Key exists so notify if lookup fails, but try other method.
				logger->LogError("Failed lookup of gEntList directly - Reverting to lookup via LevelShutdown");
			}
			else
			{
#endif
				g_EntList = reinterpret_cast<void *>(addr);
#if !defined PLATFORM_WINDOWS
			}
#endif
		}
	}

	if (!g_EntList)
	{
		if (g_pGameConf->GetMemSig("LevelShutdown", (void **) &addr) && addr)
		{
			int offset;
			if (!g_pGameConf->GetOffset("gEntList", &offset))
			{
				logger->LogError("Logical Entities not supported by this mod (gEntList) - Reverting to networkable entities only");
				return;
			}

			g_EntList = *reinterpret_cast<void **>(addr + offset);

		}
	}

	// If we have g_EntList from either of the above methods, make sure we can get the offset from it to EntInfo as well
	if (g_EntList && !g_pGameConf->GetOffset("EntInfo", &entInfoOffset))
	{
		logger->LogError("Logical Entities not supported by this mod (EntInfo) - Reverting to networkable entities only");
		g_EntList = NULL;
		return;
	}

	// If we don't have g_EntList or have it but don't know where EntInfo is on it, use fallback.
	if (!g_EntList || entInfoOffset == -1)
	{
		g_pGameConf->GetAddress("EntInfosPtr", (void **)&g_pEntInfoList);
	}
	
	if (!g_EntList && !g_pEntInfoList)
	{
		logger->LogError("Failed lookup of gEntList - Reverting to networkable entities only");
		return;
	}
}

void CHalfLife2::InitCommandLine()
{
	char error[256];
#if SOURCE_ENGINE != SE_DARKMESSIAH
	if (g_SMAPI->GetSourceEngineBuild() != SOURCE_ENGINE_ORIGINAL)
	{
		ke::RefPtr<ke::SharedLib> lib = ke::SharedLib::Open(TIER0_NAME, error, sizeof(error));
		if (!lib) {
			logger->LogError("Could not load %s: %s", TIER0_NAME, error);
			return;
		}
		
		m_pGetCommandLine = lib->get<decltype(m_pGetCommandLine)>("CommandLine_Tier0");

		/* '_Tier0' dropped on Alien Swarm version */
		if (m_pGetCommandLine == NULL)
		{
			m_pGetCommandLine = lib->get<decltype(m_pGetCommandLine)>("CommandLine");
		}
	}
	else
#endif
	{
		ke::RefPtr<ke::SharedLib> lib = ke::SharedLib::Open(VSTDLIB_NAME, error, sizeof(error));
		if (!lib) {
			logger->LogError("Could not load %s: %s", VSTDLIB_NAME, error);
			return;
		}

		m_pGetCommandLine = lib->get<decltype(m_pGetCommandLine)>("CommandLine");
	}
	
	if (m_pGetCommandLine == NULL)
	{
		logger->LogError("Could not locate any command line functionality");
	}
}

ICommandLine *CHalfLife2::GetValveCommandLine()
{
	if (!m_pGetCommandLine)
		return NULL;

	return ((FakeGetCommandLine)((FakeGetCommandLine *)m_pGetCommandLine))();
}

#if SOURCE_ENGINE != SE_DARKMESSIAH
IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
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

bool UTIL_FindDataMapInfo(datamap_t *pMap, const char *name, sm_datatable_info_t *pDataTable)
{
	while (pMap)
	{
		for (int i = 0; i < pMap->dataNumFields; ++i)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
			{
				continue;
			}
			if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
			{
				pDataTable->prop = &(pMap->dataDesc[i]);
				pDataTable->actual_offset = GetTypeDescOffs(pDataTable->prop);
				return true;
			}
			if (pMap->dataDesc[i].td == NULL || !UTIL_FindDataMapInfo(pMap->dataDesc[i].td, name, pDataTable))
			{
				continue;
			}
			
			pDataTable->actual_offset += GetTypeDescOffs(&(pMap->dataDesc[i]));
			return true;
		}
		
		pMap = pMap->baseMap;
	}

	return false; 
}

ServerClass *CHalfLife2::FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = _FindServerClass(classname);

	if (!pInfo)
		return NULL;

	return pInfo->sc;
}

DataTableInfo *CHalfLife2::_FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = NULL;
	if (!m_Classes.retrieve(classname, &pInfo))
	{
		ServerClass *sc = gamedll->GetAllServerClasses();
		while (sc)
		{
			if (strcmp(classname, sc->GetName()) == 0)
			{
				pInfo = new DataTableInfo(sc);
				m_Classes.insert(classname, pInfo);
				break;
			}
			sc = sc->m_pNext;
		}
		if (!pInfo)
			return NULL;
	}

	return pInfo;
}

bool CHalfLife2::FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info)
{
	DataTableInfo *pInfo;

	if ((pInfo = _FindServerClass(classname)) == NULL)
	{
		return false;
	}

	if (!pInfo->lookup.retrieve(offset, info))
	{
		sm_sendprop_info_t temp_info;

		if (!UTIL_FindInSendTable(pInfo->sc->m_pTable, offset, &temp_info, 0))
		{
			return false;
		}

		pInfo->lookup.insert(offset, temp_info);
		*info = temp_info;
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
	sm_datatable_info_t dt_info;
	
	if (!(this->FindDataMapInfo(pMap, offset, &dt_info)))
	{
		return NULL;
	}
	
	return dt_info.prop;
}

bool CHalfLife2::FindDataMapInfo(datamap_t *pMap, const char *offset, sm_datatable_info_t *pDataTable)
{
	DataTableMap::Insert i = m_Maps.findForAdd(pMap);
	if (!i.found())
		m_Maps.add(i, pMap, new DataMapCache());

	DataMapCache *cache = i->value;

	if (!cache->retrieve(offset, pDataTable))
	{
		if (!UTIL_FindDataMapInfo(pMap, offset, pDataTable))
			return false;
		cache->insert(offset, *pDataTable);
	}

	return true;
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
			char buffer[253];
			ke::SafeSprintf(buffer, sizeof(buffer), "%s\1\n", msg);

#if SOURCE_ENGINE == SE_CSGO
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

#if SOURCE_ENGINE == SE_CSGO
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

#if SOURCE_ENGINE == SE_CSGO
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
#if SOURCE_ENGINE == SE_CSGO
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

#if SOURCE_ENGINE == SE_CSGO
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

#if SOURCE_ENGINE == SE_CSGO
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
			serverpluginhelpers->ClientCommand(pPlayer->GetEdict(), pFake->cmd.c_str());
		}

		m_CmdQueue.pop();
		m_FreeCmds.push(pFake);
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
	if (g_SMAPI->GetSourceEngineBuild() == SOURCE_ENGINE_ORIGINAL)
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

void CHalfLife2::PushCommandStack(const ICommandArgs *cmd)
{
	CachedCommandInfo info;

	info.args = cmd;
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	ke::SafeStrcpy(info.cmd, sizeof(info.cmd), cmd->Arg(0));
#endif

	m_CommandStack.push(info);
}

const ICommandArgs *CHalfLife2::PeekCommandStack()
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
	ke::SafeSprintf(kick.buffer, sizeof(kick.buffer), "%s", msg);

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

	CEntInfo *entInfos = EntInfoArray();

	if (!entInfos)
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

	return &entInfos[entIndex];
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
		if (pGetterEnt == NULL)
		{
			// If we don't have a world entity yet, we'll have to rely on the given entity
			pGetterEnt = pEntity;
		}

		datamap_t *pMap = GetDataMap(pGetterEnt);
		
		sm_datatable_info_t info;
		if (!FindDataMapInfo(pMap, "m_iClassname", &info))
		{
			return NULL;
		}
		
		offset = info.actual_offset;
	}

	return *(const char **)(((unsigned char *)pEntity) + offset);
}

SMFindMapResult CHalfLife2::FindMap(char *pMapName, size_t nMapNameMax)
{
	return this->FindMap(pMapName, pMapName, nMapNameMax);
}

SMFindMapResult CHalfLife2::FindMap(const char *pMapName, char *pFoundMap, size_t nMapNameMax)
{
	ke::SafeStrcpy(pFoundMap, nMapNameMax, pMapName);

#if SOURCE_ENGINE >= SE_LEFT4DEAD
	static char mapNameTmp[PLATFORM_MAX_PATH];
	g_SourceMod.Format(mapNameTmp, sizeof(mapNameTmp), "maps%c%s.bsp", PLATFORM_SEP_CHAR, pMapName);
	if (filesystem->FileExists(mapNameTmp, "GAME"))
	{
		// If this is already an exact match, don't attempt to autocomplete it further (de_dust -> de_dust2).
		// ... but still check that map file is actually valid.
		// We check FileExists first to avoid console message about IsMapValid with invalid map.
		return engine->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
	}

	static ConCommand *pHelperCmd = g_pCVar->FindCommand("changelevel");
	
	// This shouldn't happen.
	if (!pHelperCmd || !pHelperCmd->CanAutoComplete())
	{
		return engine->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
	}

	static size_t helperCmdLen = strlen(pHelperCmd->GetName());

	CUtlVector<CUtlString> results;
	pHelperCmd->AutoCompleteSuggest(pMapName, results);
	if (results.Count() == 0)
		return SMFindMapResult::NotFound;

	// Results come back as you'd see in autocomplete. (ie. "changelevel fullmapnamehere"),
	// so skip ahead to start of map path/name

	// Like the engine, we're only going to deal with the first match.

	bool bExactMatch = Q_strcmp(pMapName, &results[0][helperCmdLen + 1]) == 0;
	if (bExactMatch)
	{
		return SMFindMapResult::Found;
	}
	else
	{
		ke::SafeStrcpy(pFoundMap, nMapNameMax, &results[0][helperCmdLen + 1]);
		return SMFindMapResult::FuzzyMatch;
	}

#elif SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_BMS
	static char szTemp[PLATFORM_MAX_PATH];
	if (pFoundMap == NULL)
	{
		ke::SafeStrcpy(szTemp, SM_ARRAYSIZE(szTemp), pMapName);
		pFoundMap = szTemp;
		nMapNameMax = 0;
	}

	return static_cast<SMFindMapResult>(engine->FindMap(pFoundMap, static_cast<int>(nMapNameMax)));
#elif SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_SDK2013
	static IVEngineServer *engine21 = (IVEngineServer *)(g_SMAPI->GetEngineFactory()("VEngineServer021", nullptr));
	return engine21->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
#else
	return engine->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
#endif
}

bool CHalfLife2::GetMapDisplayName(const char *pMapName, char *pDisplayname, size_t nMapNameMax)
{
	SMFindMapResult result = FindMap(pMapName, pDisplayname, nMapNameMax);

	if (result == SMFindMapResult::NotFound)
	{
		return false;
	}

#if SOURCE_ENGINE == SE_CSGO
	// In CSGO, the path separator is used in workshop maps.
	char workshop[10];
	ke::SafeSprintf(workshop, SM_ARRAYSIZE(workshop), "%s%c", "workshop", PLATFORM_SEP_CHAR);

	char *lastSlashPos;
	// In CSGO, workshop maps show up as workshop/123456789/mapname or workshop\123456789\mapname depending on OS
	if (strncmp(pDisplayname, workshop, 9) == 0 && (lastSlashPos = strrchr(pDisplayname, PLATFORM_SEP_CHAR)) != NULL)
	{
		ke::SafeStrcpy(pDisplayname, nMapNameMax, &lastSlashPos[1]);
		return true;
	}
#elif SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_BMS
	char *ugcPos;
	// In TF2 and BMS, workshop maps show up as workshop/mapname.ugc123456789 regardless of OS
	if (strncmp(pDisplayname, "workshop/", 9) == 0 && (ugcPos = strstr(pDisplayname, ".ugc")) != NULL)
	{
		// Overwrite the . with a null and SafeStrcpy will handle the rest
		ugcPos[0] = '\0';
		ke::SafeStrcpy(pDisplayname, nMapNameMax, &pDisplayname[9]);
		return true;
	}
#endif

	return true;
}

bool CHalfLife2::IsMapValid(const char *map)
{
	if (!map || !map[0])
		return false;
	
	return FindMap(map) != SMFindMapResult::NotFound;
}

// TODO: Add ep1 support for this. (No IServerTools available there)
#if SOURCE_ENGINE >= SE_ORANGEBOX
string_t CHalfLife2::AllocPooledString(const char *pszValue)
{
	// This is admittedly a giant hack, but it's a relatively safe method for
	// inserting a string into the game's string pool that isn't likely to break.
	//
	// We find the first valid ent (should always be worldspawn), save off it's
	// current targetname string_t, set it to our string to insert via SetKeyValue,
	// read back the new targetname value, restore the old value, and return the new one.

	CBaseEntity *pEntity = ((IServerUnknown *) servertools->FirstEntity())->GetBaseEntity();
	auto *pDataMap = GetDataMap(pEntity);
	assert(pDataMap);

	static int offset = -1;
	if (offset == -1)
	{
		sm_datatable_info_t info;
		bool found = FindDataMapInfo(pDataMap, "m_iName", &info);
		assert(found);
		offset = info.actual_offset;
	}

	string_t *pProp = (string_t *) ((intp) pEntity + offset);
	string_t backup = *pProp;
	servertools->SetKeyValue(pEntity, "targetname", pszValue);
	string_t newString = *pProp;
	*pProp = backup;

	return newString;
}
#endif

bool CHalfLife2::GetServerSteam3Id(char *pszOut, size_t len) const
{
	CSteamID sid(GetServerSteamId64());

	switch (sid.GetEAccountType())
	{
	case k_EAccountTypeAnonGameServer:
		ke::SafeSprintf(pszOut, len, "[A:%u:%u:%u]", sid.GetEUniverse(), sid.GetAccountID(), sid.GetUnAccountInstance());
		break;
	case k_EAccountTypeGameServer:
		ke::SafeSprintf(pszOut, len, "[G:%u:%u]", sid.GetEUniverse(), sid.GetAccountID());
		break;
	case k_EAccountTypeInvalid:
		ke::SafeSprintf(pszOut, len, "[I:%u:%u]", sid.GetEUniverse(), sid.GetAccountID());
		break;
	default:
		return false;
	}

	return true;
}

#if defined( PLATFORM_WINDOWS )
#define STEAM_LIB_PREFIX
#define STEAM_LIB_SUFFIX
#elif defined( PLATFORM_LINUX )
#define STEAM_LIB_PREFIX "lib"
#define STEAM_LIB_SUFFIX ".so"
#elif defined( PLATFORM_APPLE )
#define STEAM_LIB_PREFIX "lib"
#define STEAM_LIB_SUFFIX ".dylib"
#endif

uint64_t CHalfLife2::GetServerSteamId64() const
{
#if SOURCE_ENGINE == SE_BLADE          \
	|| SOURCE_ENGINE == SE_BMS         \
	|| SOURCE_ENGINE == SE_CSGO        \
	|| SOURCE_ENGINE == SE_CSS         \
	|| SOURCE_ENGINE == SE_DODS        \
	|| SOURCE_ENGINE == SE_EYE         \
	|| SOURCE_ENGINE == SE_HL2DM       \
	|| SOURCE_ENGINE == SE_INSURGENCY  \
	|| SOURCE_ENGINE == SE_SDK2013     \
	|| SOURCE_ENGINE == SE_ALIENSWARM  \
	|| SOURCE_ENGINE == SE_TF2
	const CSteamID *sid = engine->GetGameServerSteamID();
	if (sid)
	{
		return sid->ConvertToUint64();
	}
#else
	typedef uint64_t(* GetServerSteamIdFn)(void);
	static GetServerSteamIdFn fn = nullptr;
	if (!fn)
	{
		ke::SharedLib steam_api(STEAM_LIB_PREFIX "steam_api" STEAM_LIB_SUFFIX);
		if (steam_api.valid())
		{
			fn = (GetServerSteamIdFn)steam_api.lookup("SteamGameServer_GetSteamID");
		}
	}

	if (fn)
	{
		return fn();
	}
#endif

	return 1ULL;
}
