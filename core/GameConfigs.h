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

#include <IGameConfigs.h>
#include <ITextParsers.h>
#include <sh_list.h>
#include "sm_trie.h"
#include "sm_globals.h"
#include "sm_memtable.h"

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
public: //ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public: //IGameConfig
	const char *GetKeyValue(const char *key);
	bool GetOffset(const char *key, int *value);
	SendProp *GetSendProp(const char *key);
	bool GetMemSig(const char *key, void **addr);
public:
	void IncRefCount();
	unsigned int DecRefCount();
private:
	BaseStringTable *m_pStrings;
	char *m_pFile;
	Trie *m_pOffsets;
	Trie *m_pProps;
	Trie *m_pKeys;
	Trie *m_pSigs;
	unsigned int m_RefCount;
	/* Parse states */
	int m_ParseState;
	unsigned int m_IgnoreLevel;
	char m_Class[64];
	char m_Prop[64];
	char m_offset[64];
	char m_Game[256];
	bool bShouldBeReadingDefault;
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
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
private:
	List<CGameConfig *> m_cfgs;
	Trie *m_pLookup;
};

extern GameConfigManager g_GameConfigs;
extern IGameConfig *g_pGameConf;

#endif //_INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
