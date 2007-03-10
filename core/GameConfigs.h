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
public:
	CGameConfig(const char *file);
	~CGameConfig();
public:
	bool Reparse(char *error, size_t maxlength);
public: //ITextListener_SMC
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
public: //IGameConfig
	const char *GetKeyValue(const char *key);
	bool GetOffset(const char *key, int *value);
	SendProp *GetSendProp(const char *key);
public:
	void IncRefCount();
	unsigned int DecRefCount();
private:
	BaseStringTable *m_pStrings;
	char *m_pFile;
	Trie *m_pOffsets;
	Trie *m_pProps;
	Trie *m_pKeys;
	unsigned int m_RefCount;
	/* Parse states */
	int m_ParseState;
	unsigned int m_IgnoreLevel;
	char m_class[64];
	char m_prop[64];
	char m_offset[64];
	char m_mod[255];
};

class CGameConfigManager : 
	public IGameConfigManager,
	public SMGlobalClass
{
public:
	CGameConfigManager();
	~CGameConfigManager();
public: //IGameConfigManager
	bool LoadGameConfigFile(const char *file, IGameConfig **pConfig, char *error, size_t maxlength);
	void CloseGameConfigFile(IGameConfig *cfg);
public: //SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModAllShutdown();
private:
	List<CGameConfig *> m_cfgs;
	Trie *m_pLookup;
};

extern CGameConfigManager g_GameConfigs;
extern IGameConfig *g_pGameConf;

#endif //_INCLUDE_SOURCEMOD_CGAMECONFIGS_H_
