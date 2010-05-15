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

#ifndef _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
#define _INCLUDE_SOURCEMOD_CGAMECONFIGS_H_

#include "common_logic.h"
#include <IGameConfigs.h>
#include <ITextParsers.h>
#include <sh_list.h>
#include "sm_memtable.h"
#include <sm_trie_tpl.h>

using namespace SourceMod;
using namespace SourceHook;

class SendProp;

class CGameConfig : 
	public ITextListener_SMC,
	public IGameConfig
{
	friend class GameConfigManager;
public:
	CGameConfig(const char *file);
	~CGameConfig();
public:
	bool Reparse(char *error, size_t maxlength);
	bool EnterFile(const char *file, char *error, size_t maxlength);
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
public:
	void IncRefCount();
	unsigned int DecRefCount();
private:
	BaseStringTable *m_pStrings;
	char m_File[PLATFORM_MAX_PATH];
	char m_CurFile[PLATFORM_MAX_PATH];
	KTrie<int> m_Offsets;
	KTrie<SendProp *> m_Props;
	KTrie<int> m_Keys;
	KTrie<void *> m_Sigs;
	unsigned int m_RefCount;
	/* Parse states */
	int m_ParseState;
	unsigned int m_IgnoreLevel;
	char m_Class[64];
	char m_Prop[64];
	char m_offset[64];
	char m_Game[256];
	bool bShouldBeReadingDefault;
	bool had_game;
	bool matched_game;
	bool had_engine;
	bool matched_engine;

	/* Custom Sections */
	unsigned int m_CustomLevel;
	ITextListener_SMC *m_CustomHandler;

	/* Support for reading Addresses */
	struct AddressConf
	{
		char signatureName[64];
		int readCount;
		int read[8];

		AddressConf(char *sigName, unsigned sigLength, unsigned readCount, int *read);

		AddressConf() {}
	};

	char m_Address[64];
	char m_AddressSignature[64];
	int m_AddressReadCount;
	int m_AddressRead[8];
	KTrie<AddressConf> *m_pAddresses;
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
private:
	List<CGameConfig *> m_cfgs;
	KTrie<CGameConfig *> m_Lookup;
public:
	KTrie<ITextListener_SMC *> m_customHandlers;
};

extern GameConfigManager g_GameConfigs;
extern IGameConfig *g_pGameConf;

#endif //_INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
