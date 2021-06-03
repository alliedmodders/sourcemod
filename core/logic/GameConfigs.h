/**
 * vim: set ts=4 sw=4 tw=99 noet:
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

#ifndef _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
#define _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_

#include "common_logic.h"
#include <IGameConfigs.h>
#include <ITextParsers.h>
#include <am-refcounting.h>
#include <sm_stringhashmap.h>
#include <sm_namehashset.h>

using namespace SourceMod;

class SendProp;

class CGameConfig : 
	public ITextListener_SMC,
	public IGameConfig,
	public ke::Refcounted<CGameConfig>
{
	friend class GameConfigManager;
public:
	CGameConfig(const char *file, const char *engine = NULL);
	~CGameConfig();
public:
	bool Reparse(char *error, size_t maxlength);
	bool EnterFile(const char *file, char *error, size_t maxlength);
	void SetBaseEngine(const char *engine);
	void SetParseEngine(const char *engine);
public: //ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public: //IGameConfig
	const char *GetKeyValue(const char *key);
	bool GetOffset(const char *key, int *value);
	SendProp *GetSendProp(const char *key);
	bool GetMemSig(const char *key, void **addr);
	bool GetAddress(const char *key, void **addr);
public: //NameHashSet
	static inline bool matches(const char *key, const CGameConfig *value)
	{
		return strcmp(key, value->m_File) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
private:
	char m_File[PLATFORM_MAX_PATH];
	char m_CurFile[PLATFORM_MAX_PATH];
	StringHashMap<int> m_Offsets;
	StringHashMap<SendProp *> m_Props;
	StringHashMap<std::string> m_Keys;
	StringHashMap<void *> m_Sigs;
	/* Parse states */
	int m_ParseState;
	unsigned int m_IgnoreLevel;
	std::string m_Class;
	std::string m_Prop;
	std::string m_offset;
	std::string m_Game;
	std::string m_Key;
	bool bShouldBeReadingDefault;
	bool had_game;
	bool matched_game;
	bool had_engine;
	bool matched_engine;
	bool matched_platform;

	/* Custom Sections */
	unsigned int m_CustomLevel;
	ITextListener_SMC *m_CustomHandler;

	/* Support for reading Addresses */
	struct AddressConf
	{
		std::string signatureName;
		int readCount;
		int read[8];
		bool lastIsOffset;

		AddressConf(std::string&& sigName, unsigned readCount, int *read, bool lastIsOffset);

		AddressConf() {}
	};

	std::string m_Address;
	std::string m_AddressSignature;
	int m_AddressReadCount;
	int m_AddressRead[8];
	bool m_AddressLastIsOffset;
	StringHashMap<AddressConf> m_Addresses;
	const char *m_pEngine;
	const char *m_pBaseEngine;
	time_t m_ModTime;
};

class GameConfigManager : 
	public IGameConfigManager,
	public SMGlobalClass
{
public:
	GameConfigManager();
	~GameConfigManager();
public: //IGameConfigManager
	bool LoadGameConfigFile(const char *file, IGameConfig **pConfig, char *error, size_t maxlength);
	void CloseGameConfigFile(IGameConfig *cfg);
	IGameConfig *ReadHandle(Handle_t hndl,
		IdentityToken_t *ident,
		HandleError *err);
	void AddUserConfigHook(const char *sectionname, ITextListener_SMC *listener);
	void RemoveUserConfigHook(const char *sectionname, ITextListener_SMC *listener);
	void AcquireLock();
	void ReleaseLock();
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
public:
	void RemoveCachedConfig(CGameConfig *config);
private:
	NameHashSet<CGameConfig *> m_Lookup;
public:
	StringHashMap<ITextListener_SMC *> m_customHandlers;
};

extern GameConfigManager g_GameConfigs;
extern IGameConfig *g_pGameConf;

#endif //_INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
