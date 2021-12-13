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
#elif SOURCE_ENGINE == SE_BLADE
#include <berimbau_usermessages.pb.h>
#endif

typedef ICommandLine *(*FakeGetCommandLine)();

#define TIER0_NAME			FORMAT_SOURCE_BIN_NAME("tier0")
#define VSTDLIB_NAME		FORMAT_SOURCE_BIN_NAME("vstdlib")

CHalfLife2 g_HL2;
ConVar *sv_lan = NULL;

static void *g_EntList = NULL;
static void **g_pEntInfoList = NULL;
static int entInfoOffset = -1;
static int utlVecOffsetOffset = -1;

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
	m_CSGOBadList.add("m_iCompetitiveRankType");
	m_CSGOBadList.add("m_nActiveCoinRank");
	m_CSGOBadList.add("m_nMusicID");
#endif
	g_pGameConf->GetOffset("CSendPropExtra_UtlVector::m_Offset", &utlVecOffsetOffset);
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
			return ConfigResult_Accept;
		}
		else if (strcasecmp(value, "yes") == 0)
		{
			m_bFollowCSGOServerGuidelines = true;
			return ConfigResult_Accept;
		}
		else
		{
			ke::SafeStrcpy(error, maxlength, "Invalid value: must be \"yes\" or \"no\"");
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
	|| SOURCE_ENGINE == SE_BLADE   \
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

#ifdef PLATFORM_X86
			g_EntList = *reinterpret_cast<void **>(addr + offset);
#elif defined PLATFORM_X64
			int32_t varOffset = *reinterpret_cast<int32_t *>(addr + offset);
			g_EntList = reinterpret_cast<void *>(addr + offset + sizeof(int32_t) + varOffset);
#endif

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
	int props = pTable->GetNumProps();
	for (int i = 0; i < props; i++)
	{
		SendProp *prop = pTable->GetProp(i);

		// Skip InsideArray props (SendPropArray / SendPropArray2),
		// we'll find them later by their containing array.
		if (prop->IsInsideArray()) {
			continue;
		}

		const char *pname = prop->GetName();
		SendTable *pInnerTable = prop->GetDataTable();

		if (pname && strcmp(name, pname) == 0)
		{
			// get true offset of CUtlVector
			if (utlVecOffsetOffset != -1 && prop->GetOffset() == 0 && pInnerTable && pInnerTable->GetNumProps())
			{
				SendProp *pLengthProxy = pInnerTable->GetProp(0);
				const char *ipname = pLengthProxy->GetName();
				if (ipname && strcmp(ipname, "lengthproxy") == 0 && pLengthProxy->GetExtraData())
				{
					info->prop = prop;
					info->actual_offset = offset + *reinterpret_cast<size_t *>(reinterpret_cast<intptr_t>(pLengthProxy->GetExtraData()) + utlVecOffsetOffset);
					return true;
				}
			}
			info->prop = prop;
			info->actual_offset = offset + info->prop->GetOffset();
			return true;
		}
		if (pInnerTable)
		{
			if (UTIL_FindInSendTable(pInnerTable, 
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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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
	pBitBuf->WriteString("");
	pBitBuf->WriteString("");
	pBitBuf->WriteString("");
	pBitBuf->WriteString("");
#endif

	g_UserMsgs.EndMessage();

	return true;
}

bool CHalfLife2::HintTextMsg(int client, const char *msg)
{
	cell_t players[] = {client};

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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
#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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

#if SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE
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
	ke::SafeStrcpy(kick.buffer, sizeof(kick.buffer), msg);

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

#ifdef PLATFORM_WINDOWS
bool CheckReservedFilename(const char *in, const char *reservedName)
{
	size_t nameLen = strlen(reservedName);
	for (size_t i = 0; i < nameLen; ++i)
	{
		if (reservedName[i] != tolower(in[i]))
		{
			return false;
		}
	}

	if (in[nameLen] == '\0' || in[nameLen] == '.')
	{
		return true;
	}

	return false;
}

bool IsWindowsReservedDeviceName(const char *pMapname)
{
	static const char * const reservedDeviceNames[] = {
		"con", "prn", "aux", "clock$", "nul", "com1",
		"com2", "com3", "com4", "com5", "com6", "com7",
		"com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4",
		"lpt5", "lpt6", "lpt7", "lpt8", "lpt9"
	};
	
	size_t reservedCount = sizeof(reservedDeviceNames) / sizeof(reservedDeviceNames[0]);
	for (size_t i = 0; i < reservedCount; ++i)
	{
		if (CheckReservedFilename(pMapname, reservedDeviceNames[i]))
		{
			return true;
		}
	}
	
	return false;
}
#endif 

#if SOURCE_ENGINE >= SE_LEFT4DEAD && defined PLATFORM_WINDOWS && SOURCE_ENGINE != SE_MOCK
// This frees memory allocated by the game using the game's CRT on Windows,
// avoiding a crash due to heap corruption (issue #910).
template< class T, class I >
class CUtlMemoryGlobalMalloc : public CUtlMemory< T, I >
{
	typedef CUtlMemory< T, I > BaseClass;

public:
	using BaseClass::BaseClass;

	void Purge()
	{
		if (!IsExternallyAllocated())
		{
			if (m_pMemory)
			{
				UTLMEMORY_TRACK_FREE();
				g_pMemAlloc->Free((void*)m_pMemory);
				m_pMemory = 0;
			}
			m_nAllocationCount = 0;
		}
		BaseClass::Purge();
	}
};

void CHalfLife2::FreeUtlVectorUtlString(CUtlVector<CUtlString, CUtlMemoryGlobalMalloc<CUtlString>> &vec)
{
	CUtlMemoryGlobalMalloc<unsigned char> *pMemory;
	FOR_EACH_VEC(vec, i)
	{
		pMemory = (CUtlMemoryGlobalMalloc<unsigned char> *) &vec[i].m_Storage.m_Memory;
		pMemory->Purge();
		vec[i].m_Storage.SetLength(0);
	}
}
#endif

SMFindMapResult CHalfLife2::FindMap(const char *pMapName, char *pFoundMap, size_t nMapNameMax)
{
	/* We need to ensure user input does not contain reserved device names on windows */
#ifdef PLATFORM_WINDOWS
	if (IsWindowsReservedDeviceName(pMapName))
	{
		return SMFindMapResult::NotFound;
	}
#endif
	
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

#ifdef PLATFORM_WINDOWS
	CUtlVector<CUtlString, CUtlMemoryGlobalMalloc<CUtlString>> results;
	pHelperCmd->AutoCompleteSuggest(pMapName, *(CUtlVector<CUtlString, CUtlMemory<CUtlString>>*)&results);
#else
	CUtlVector<CUtlString> results;
	pHelperCmd->AutoCompleteSuggest(pMapName, results);
#endif
	if (results.Count() == 0)
		return SMFindMapResult::NotFound;

	// Results come back as you'd see in autocomplete. (ie. "changelevel fullmapnamehere"),
	// so skip ahead to start of map path/name

	// Like the engine, we're only going to deal with the first match.

	bool bExactMatch = Q_strcmp(pMapName, &results[0][helperCmdLen + 1]) == 0;
	if (bExactMatch)
	{
#ifdef PLATFORM_WINDOWS
		FreeUtlVectorUtlString(results);
#endif
		return SMFindMapResult::Found;
	}
	else
	{
		ke::SafeStrcpy(pFoundMap, nMapNameMax, &results[0][helperCmdLen + 1]);
#ifdef PLATFORM_WINDOWS
		FreeUtlVectorUtlString(results);
#endif
		return SMFindMapResult::FuzzyMatch;
	}

#elif SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_HL2DM \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS
	static IVEngineServer *engine23 = (IVEngineServer *)(g_SMAPI->GetEngineFactory()("VEngineServer023", nullptr));
	if (engine23)
	{
		static char szTemp[PLATFORM_MAX_PATH];
		if (pFoundMap == NULL)
		{
			ke::SafeStrcpy(szTemp, SM_ARRAYSIZE(szTemp), pMapName);
			pFoundMap = szTemp;
			nMapNameMax = 0;
		}

		return static_cast<SMFindMapResult>(engine->FindMap(pFoundMap, static_cast<int>(nMapNameMax)));
	}
	else
	{
		static IVEngineServer *engine21 = (IVEngineServer *)(g_SMAPI->GetEngineFactory()("VEngineServer021", nullptr));
		return engine21->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
	}
#else
	return engine->IsMapValid(pMapName) == 0 ? SMFindMapResult::NotFound : SMFindMapResult::Found;
#endif
}

bool CHalfLife2::GetMapDisplayName(const char *pMapName, char *pDisplayname, size_t nMapNameMax)
{
	SMFindMapResult result = FindMap(pMapName, pDisplayname, nMapNameMax);

	if (result == SMFindMapResult::NotFound)
		return false;

	char *pPos;
	if ((pPos = strrchr(pDisplayname, '/')) != NULL || (pPos = strrchr(pDisplayname, '\\')) != NULL)
		ke::SafeStrcpy(pDisplayname, nMapNameMax, &pPos[1]);

	if ((pPos = strstr(pDisplayname, ".ugc")) != NULL)
		pPos[0] = '\0';

	return true;
}

bool CHalfLife2::IsMapValid(const char *map)
{
	if (!map || !map[0])
		return false;
	
	return FindMap(map) != SMFindMapResult::NotFound;
}

#if SOURCE_ENGINE < SE_ORANGEBOX
class VKeyValuesSS_Helper {};
static bool VKeyValuesSS(CBaseEntity* pThisPtr, const char *pszKey, const char *pszValue, int offset)
{
	void** this_ptr = *reinterpret_cast<void***>(&pThisPtr);
	void** vtable = *reinterpret_cast<void***>(pThisPtr);
	void* vfunc = vtable[offset];

	union
	{
		bool (VKeyValuesSS_Helper::* mfpnew)(const char *, const char *);
#ifndef PLATFORM_POSIX
		void* addr;
	} u;
	u.addr = vfunc;
#else
		struct
		{
			void* addr;
			intptr_t adjustor;
		} s;
} u;
	u.s.addr = vfunc;
	u.s.adjustor = 0;
#endif

	return (bool)(reinterpret_cast<VKeyValuesSS_Helper*>(this_ptr)->*u.mfpnew)(pszKey, pszValue);
}
#endif

string_t CHalfLife2::AllocPooledString(const char *pszValue)
{
	// This is admittedly a giant hack, but it's a relatively safe method for
	// inserting a string into the game's string pool that isn't likely to break.
	//
	// We find the first valid ent (should always be worldspawn), save off it's
	// current targetname string_t, set it to our string to insert via SetKeyValue,
	// read back the new targetname value, restore the old value, and return the new one.

#if SOURCE_ENGINE < SE_ORANGEBOX
	CBaseEntity* pEntity = nullptr;
	for (int i = 0; i < gpGlobals->maxEntities; ++i)
	{
		pEntity = ReferenceToEntity(i);
		if (pEntity)
		{
			break;
		}
	}

	if (!pEntity)
	{
		logger->LogError("Failed to locate a valid entity for AllocPooledString.");
		return NULL_STRING;
	}

#else
	CBaseEntity *pEntity = ((IServerUnknown *) servertools->FirstEntity())->GetBaseEntity();
#endif
	auto *pDataMap = GetDataMap(pEntity);
	assert(pDataMap);

	static int iNameOffset = -1;
	if (iNameOffset == -1)
	{
		sm_datatable_info_t info;
		bool found = FindDataMapInfo(pDataMap, "m_iName", &info);
		assert(found);
		iNameOffset = info.actual_offset;
	}

	string_t* pProp = (string_t*)((intp)pEntity + iNameOffset);
	string_t backup = *pProp;

#if SOURCE_ENGINE < SE_ORANGEBOX
	static int iFuncOffset;
	if (!g_pGameConf->GetOffset("DispatchKeyValue", &iFuncOffset) || !iFuncOffset)
	{
		logger->LogError("Failed to locate DispatchKeyValue in core gamedata. AllocPooledString unsupported.");
		return NULL_STRING;
	}
	VKeyValuesSS(pEntity, "targetname", pszValue, iFuncOffset);
#else	
	servertools->SetKeyValue(pEntity, "targetname", pszValue);
#endif

	string_t newString = *pProp;
	*pProp = backup;

	return newString;
}

bool CHalfLife2::GetServerSteam3Id(char *pszOut, size_t len) const
{
	CSteamID sid((uint64)GetServerSteamId64());

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
	|| SOURCE_ENGINE == SE_DOI  \
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
