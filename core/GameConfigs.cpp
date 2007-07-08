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

#include <string.h>
#include <stdlib.h>
#include "GameConfigs.h"
#include "TextParsers.h"
#include "sm_stringutil.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "HalfLife2.h"
#include "Logger.h"
#include "ShareSys.h"
#include "MemoryUtils.h"
#include "LibrarySys.h"
#include "HandleSys.h"

#if defined PLATFORM_LINUX
#include <dlfcn.h>
#endif

GameConfigManager g_GameConfigs;
IGameConfig *g_pGameConf = NULL;
char g_mod[255];

#define PSTATE_NONE						0
#define PSTATE_GAMES					1
#define PSTATE_GAMEDEFS					2
#define PSTATE_GAMEDEFS_OFFSETS			3
#define PSTATE_GAMEDEFS_OFFSETS_OFFSET	4
#define PSTATE_GAMEDEFS_KEYS			5
#define PSTATE_GAMEDEFS_SUPPORTED		6
#define PSTATE_GAMEDEFS_SIGNATURES		7
#define PSTATE_GAMEDEFS_SIGNATURES_SIG	8

#if defined PLATFORM_WINDOWS
#define PLATFORM_NAME	"windows"
#elif defined PLATFORM_LINUX
#define PLATFORM_NAME	"linux"
#endif

struct TempSigInfo
{
	void Reset()
	{
		library[0] = '\0';
		sig[0] = '\0';
	}
	char sig[512];
	char library[64];
} s_TempSig;

CGameConfig::CGameConfig(const char *file)
{
	m_pFile = sm_strdup(file);
	m_pOffsets = sm_trie_create();
	m_pProps = sm_trie_create();
	m_pKeys = sm_trie_create();
	m_pSigs = sm_trie_create();
	m_pStrings = new BaseStringTable(512);
	m_RefCount = 0;
}

CGameConfig::~CGameConfig()
{
	sm_trie_destroy(m_pOffsets);
	sm_trie_destroy(m_pProps);
	sm_trie_destroy(m_pKeys);
	sm_trie_destroy(m_pSigs);
	delete [] m_pFile;
	delete m_pStrings;
}

SMCParseResult CGameConfig::ReadSMC_NewSection(const char *name, bool opt_quotes)
{
	if (m_IgnoreLevel)
	{
		m_IgnoreLevel++;
		return SMCParse_Continue;
	}

	switch (m_ParseState)
	{
	case PSTATE_NONE:
		{
			if (strcmp(name, "Games") == 0)
			{
				m_ParseState = PSTATE_GAMES;
			} else {
				m_IgnoreLevel++;
			}
			break;
		}
	case PSTATE_GAMES:
		{
			if ((strcmp(name, "*") == 0)
				|| (strcmp(name, "#default") == 0)
				|| (strcmp(name, g_mod) == 0))
			{
				m_ParseState = PSTATE_GAMEDEFS;
				strncopy(m_mod, name, sizeof(m_mod));
			} else {
				m_IgnoreLevel++;
			}
			break;
		}
	case PSTATE_GAMEDEFS:
		{
			if (strcmp(name, "Offsets") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_OFFSETS;
			}
			else if (strcmp(name, "Keys") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_KEYS;
			}
			else if ((strcmp(name, "#supported") == 0)
					 && (strcmp(m_mod, "#default") == 0))
			{
				m_ParseState = PSTATE_GAMEDEFS_SUPPORTED;
				/* Ignore this section unless we get a game. */
				bShouldBeReadingDefault = false;
			}
			else if (strcmp(name, "Signatures") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_SIGNATURES;
			}
			else
			{
				m_IgnoreLevel++;
			}
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS:
		{
			m_prop[0] = '\0';
			m_class[0] = '\0';
			strncopy(m_offset, name, sizeof(m_offset));
			m_ParseState = PSTATE_GAMEDEFS_OFFSETS_OFFSET;
			break;
		}
	case PSTATE_GAMEDEFS_SIGNATURES:
		{
			strncopy(m_offset, name, sizeof(m_offset));
			s_TempSig.Reset();
			m_ParseState = PSTATE_GAMEDEFS_SIGNATURES_SIG;
			break;
		}
	/* No sub-sections allowed:
	 case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
	 case PSTATE_GAMEDEFS_KEYS:
	 case PSTATE_GAMEDEFS_SUPPORTED:
	 case PSTATE_GAMEDEFS_SIGNATURES_SIG:
	 */
	default:
		{
			/* If we don't know what we got, start ignoring */
			m_IgnoreLevel++;
			break;
		}
	}

	return SMCParse_Continue;
}

SMCParseResult CGameConfig::ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
{
	if (m_IgnoreLevel)
	{
		return SMCParse_Continue;
	}

	if (m_ParseState == PSTATE_GAMEDEFS_OFFSETS_OFFSET)
	{
		if (strcmp(key, "class") == 0)
		{
			strncopy(m_class, value, sizeof(m_class));
		} else if (strcmp(key, "prop") == 0) {
			strncopy(m_prop, value, sizeof(m_prop));
		} else if (strcmp(key, PLATFORM_NAME) == 0) {
			int val = atoi(value);
			sm_trie_replace(m_pOffsets, m_offset, (void *)val);
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		int id = m_pStrings->AddString(value);
		sm_trie_replace(m_pKeys, key, (void *)id);
	} else if (m_ParseState == PSTATE_GAMEDEFS_SUPPORTED) {
		if (strcmp(key, "game") == 0
			&& strcmp(value, g_mod) == 0)
		{
			bShouldBeReadingDefault = true;
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_SIGNATURES_SIG) {
		if (strcmp(key, PLATFORM_NAME) == 0)
		{
			strncopy(s_TempSig.sig, value, sizeof(s_TempSig.sig));
		} else if (strcmp(key, "library") == 0) {
			strncopy(s_TempSig.library, value, sizeof(s_TempSig.library));
		}
	}

	return SMCParse_Continue;
}

SMCParseResult CGameConfig::ReadSMC_LeavingSection()
{
	if (m_IgnoreLevel)
	{
		m_IgnoreLevel--;
		return SMCParse_Continue;
	}

	switch (m_ParseState)
	{
	case PSTATE_GAMES:
		{
			m_ParseState = PSTATE_NONE;
			break;
		}
	case PSTATE_GAMEDEFS:
		{
			m_ParseState = PSTATE_GAMES;
			break;
		}
	case PSTATE_GAMEDEFS_KEYS:
	case PSTATE_GAMEDEFS_OFFSETS:
		{
			m_ParseState = PSTATE_GAMEDEFS;
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
		{
			/* Parse the offset... */
			if (m_class[0] != '\0'
				&& m_prop[0] != '\0')
			{
				SendProp *pProp = g_HL2.FindInSendTable(m_class, m_prop);
				if (pProp)
				{
					int val = pProp->GetOffset();
					sm_trie_replace(m_pOffsets, m_offset, (void *)val);
					sm_trie_replace(m_pProps, m_offset, pProp);
				} else {
					/* Check if it's a non-default game and no offsets exist */
					if (((strcmp(m_mod, "*") != 0) && strcmp(m_mod, "#default") != 0)
						&& (!sm_trie_retrieve(m_pOffsets, m_offset, NULL)))
					{
						g_Logger.LogError("[SM] Unable to find property %s.%s (file \"%s\") (mod \"%s\")", 
							m_class,
							m_prop,
							m_pFile,
							m_mod);
					}
				}
			}
			m_ParseState = PSTATE_GAMEDEFS_OFFSETS;
			break;
		}
	case PSTATE_GAMEDEFS_SUPPORTED:
		{
			if (!bShouldBeReadingDefault)
			{
				/* If we shouldn't read the rest of this section, set the ignore level. */
				m_IgnoreLevel = 1;
				m_ParseState = PSTATE_GAMES;
			} else {
				m_ParseState = PSTATE_GAMEDEFS;
			}
			break;
		}
	case PSTATE_GAMEDEFS_SIGNATURES:
		{
			m_ParseState = PSTATE_GAMEDEFS;
			break;
		}
	case PSTATE_GAMEDEFS_SIGNATURES_SIG:
		{
			if (s_TempSig.library[0] == '\0')
			{
				/* assume server */
				strncopy(s_TempSig.library, "server", sizeof(s_TempSig.library));
			}
			void *addrInBase = NULL;
			if (strcmp(s_TempSig.library, "server") == 0)
			{
				addrInBase = (void *)g_SMAPI->serverFactory(false);
			} else if (strcmp(s_TempSig.library, "engine") == 0) {
				addrInBase = (void *)g_SMAPI->engineFactory(false);
			}
			void *final_addr = NULL;
			if (addrInBase == NULL)
			{
				g_Logger.LogError("[SM] Unrecognized library \"%s\" (gameconf \"%s\")", 
					s_TempSig.library, 
					m_pFile);
			} else {
#if defined PLATFORM_LINUX
				if (s_TempSig.sig[0] == '@')
				{
					Dl_info info;
					/* GNU only: returns 0 on error, inconsistent! >:[ */
					if (dladdr(addrInBase, &info) != 0)
					{
						void *handle = dlopen(info.dli_fname, RTLD_NOW);
						if (handle)
						{
							final_addr = dlsym(handle, &s_TempSig.sig[1]);
							dlclose(handle);
						} else {
							g_Logger.LogError("[SM] Unable to load library \"%s\" (gameconf \"%s\")",
								s_TempSig.library,
								m_pFile);
						}
					} else {
						g_Logger.LogError("[SM] Unable to find library \"%s\" in memory (gameconf \"%s\")",
							s_TempSig.library,
							m_pFile);
					}
				}
				if (final_addr)
				{
					goto skip_find;
				}
#endif
				/* First, preprocess the signature */
				char real_sig[255];
				size_t real_bytes;
				size_t length;

				real_bytes = 0;
				length = strlen(s_TempSig.sig);

				for (size_t i=0; i<length; i++)
				{
					if (real_bytes >= sizeof(real_sig))
					{
						break;
					}
					real_sig[real_bytes++] = s_TempSig.sig[i];
					if (s_TempSig.sig[i] == '\\'
						&& s_TempSig.sig[i+1] == 'x')
					{
						if (i + 3 >= length)
						{
							continue;
						}
						/* Get the hex part */
						char s_byte[3];
						int r_byte;
						s_byte[0] = s_TempSig.sig[i+2];
						s_byte[1] = s_TempSig.sig[i+3];
						s_byte[2] = '\0';
						/* Read it as an integer */
						sscanf(s_byte, "%x", &r_byte);
						/* Save the value */
						real_sig[real_bytes-1] = r_byte;
						/* Adjust index */
						i += 3;
					}
				}

				if (real_bytes >= 1)
				{
					final_addr = g_MemUtils.FindPattern(addrInBase, real_sig, real_bytes);
				}
			}

#if defined PLATFORM_LINUX
skip_find:
#endif
			sm_trie_replace(m_pSigs, m_offset, final_addr);
			m_ParseState = PSTATE_GAMEDEFS_SIGNATURES;

			break;
		}
	}

	return SMCParse_Continue;
}

bool CGameConfig::Reparse(char *error, size_t maxlength)
{
	SMCParseError err;

	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "gamedata/%s.txt", m_pFile);
	/* Backwards compatibility */
	if (!g_LibSys.PathExists(path))
	{
		g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "configs/gamedata/%s.txt", m_pFile);
	}

	/* Initialize parse states */
	m_IgnoreLevel = 0;
	bShouldBeReadingDefault = true;
	m_ParseState = PSTATE_NONE;
	/* Reset cached data */
	m_pStrings->Reset();
	sm_trie_clear(m_pOffsets);
	sm_trie_clear(m_pProps);
	sm_trie_clear(m_pKeys);

	if ((err=g_TextParser.ParseFile_SMC(path, this, NULL, NULL))
		!= SMCParse_Okay)
	{
		if (error && (err != SMCParse_Custom))
		{
			const char *str = g_TextParser.GetSMCErrorString(err);
			snprintf(error, maxlength, "%s", str);
		}
		return false;
	}

	return true;
}

bool CGameConfig::GetOffset(const char *key, int *value)
{
	void *obj;

	if (!sm_trie_retrieve(m_pOffsets, key, &obj))
	{
		return false;
	}

	*value = (int)obj;
	
	return true;
}

const char *CGameConfig::GetKeyValue(const char *key)
{
	void *obj;
	if (!sm_trie_retrieve(m_pKeys, key, &obj))
	{
		return NULL;
	}
	return m_pStrings->GetString((int)obj);
}

SendProp *CGameConfig::GetSendProp(const char *key)
{
	SendProp *pProp;

	if (!sm_trie_retrieve(m_pProps, key, (void **)&pProp))
	{
		return NULL;
	}

	return pProp;
}

bool CGameConfig::GetMemSig(const char *key, void **addr)
{
	return sm_trie_retrieve(m_pSigs, key, addr);
}

void CGameConfig::IncRefCount()
{
	m_RefCount++;
}

unsigned int CGameConfig::DecRefCount()
{
	m_RefCount--;
	return m_RefCount;
}

GameConfigManager::GameConfigManager()
{
	m_pLookup = sm_trie_create();
}

GameConfigManager::~GameConfigManager()
{
	sm_trie_destroy(m_pLookup);
}

void GameConfigManager::OnSourceModStartup(bool late)
{
	LoadGameConfigFile("core.games", &g_pGameConf, NULL, 0);

	strncopy(g_mod, g_SourceMod.GetGameFolderName(), sizeof(g_mod));
}

void GameConfigManager::OnSourceModAllInitialized()
{
	/* NOW initialize the game file */
	CGameConfig *pGameConf = (CGameConfig *)g_pGameConf;
	
	char error[255];
	if (!pGameConf->Reparse(error, sizeof(error)))
	{
		/* :TODO: log */
	}

	g_ShareSys.AddInterface(NULL, this);
}

void GameConfigManager::OnSourceModAllShutdown()
{
	CloseGameConfigFile(g_pGameConf);
}

bool GameConfigManager::LoadGameConfigFile(const char *file, IGameConfig **_pConfig, char *error, size_t maxlength)
{
	CGameConfig *pConfig;

	if (sm_trie_retrieve(m_pLookup, file, (void **)&pConfig))
	{
		pConfig->IncRefCount();
		*_pConfig = pConfig;
		return true;
	}

	pConfig = new CGameConfig(file);

	/* :HACKHACK: Don't parse the main config file */
	bool retval = true;
	if (_pConfig != &g_pGameConf)
	{
		retval = pConfig->Reparse(error, maxlength);
	}

	m_cfgs.push_back(pConfig);
	sm_trie_insert(m_pLookup, file, pConfig);

	pConfig->IncRefCount();

	*_pConfig = pConfig;

	return retval;
}

void GameConfigManager::CloseGameConfigFile(IGameConfig *cfg)
{
	CGameConfig *pConfig = (CGameConfig *)cfg;

	if (pConfig->DecRefCount() == 0)
	{
		sm_trie_delete(m_pLookup, pConfig->m_pFile);
		delete pConfig;
	}
}

extern HandleType_t g_GameConfigsType;

IGameConfig *GameConfigManager::ReadHandle(Handle_t hndl, IdentityToken_t *ident, HandleError *err)
{
	HandleSecurity sec(ident, g_pCoreIdent);
	IGameConfig *conf = NULL;
	HandleError _err = g_HandleSys.ReadHandle(hndl, g_GameConfigsType, &sec, (void **)&conf);

	if (err)
	{
		*err = _err;
	}

	return conf;
}
