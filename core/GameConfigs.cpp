/**
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

CGameConfigManager g_GameConfigs;
IGameConfig *g_pGameConf = NULL;
char g_mod[255];

#define PSTATE_NONE						0
#define PSTATE_GAMES					1
#define PSTATE_GAMEDEFS					2
#define PSTATE_GAMEDEFS_OFFSETS			3
#define PSTATE_GAMEDEFS_OFFSETS_OFFSET	4
#define PSTATE_GAMEDEFS_KEYS			5

#if defined PLATFORM_WINDOWS
#define PLATFORM_NAME	"windows"
#elif defined PLATFORM_LINUX
#define PLATFORM_NAME	"linux"
#endif

CGameConfig::CGameConfig(const char *file)
{
	m_pFile = sm_strdup(file);
	m_pOffsets = sm_trie_create();
	m_pProps = sm_trie_create();
	m_pKeys = sm_trie_create();
	m_pStrings = new BaseStringTable(512);
}

CGameConfig::~CGameConfig()
{
	sm_trie_destroy(m_pOffsets);
	sm_trie_destroy(m_pProps);
	sm_trie_destroy(m_pKeys);
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
			} else if (strcmp(name, "Keys") == 0) {
				m_ParseState = PSTATE_GAMEDEFS_KEYS;
			} else {
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
	/* No sub-sections allowed:
	 case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
	 case PSTATE_GAMEDEFS_KEYS:
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
			sm_trie_insert(m_pOffsets, m_offset, (void *)val);
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		int id = m_pStrings->AddString(value);
		sm_trie_insert(m_pKeys, key, (void *)id);
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
					sm_trie_insert(m_pOffsets, m_offset, (void *)val);
					sm_trie_insert(m_pProps, m_offset, pProp);
				} else {
					/* Check if it's a non-default game and no offsets exist */
					if ((strcmp(m_mod, "*") != 0)
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
	}

	return SMCParse_Continue;
}

bool CGameConfig::Reparse(char *error, size_t maxlength)
{
	SMCParseError err;

	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "configs/gamedata/%s.txt", m_pFile);

	/* Initialize parse states */
	m_IgnoreLevel = 0;
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

void CGameConfig::IncRefCount()
{
	m_RefCount++;
}

unsigned int CGameConfig::DecRefCount()
{
	m_RefCount--;
	return m_RefCount;
}

CGameConfigManager::CGameConfigManager()
{
	m_pLookup = sm_trie_create();
}

CGameConfigManager::~CGameConfigManager()
{
	sm_trie_destroy(m_pLookup);
}

void CGameConfigManager::OnSourceModStartup(bool late)
{
	LoadGameConfigFile("core.games", &g_pGameConf, NULL, 0);

	char mod[255];
	engine->GetGameDir(mod, sizeof(mod));

	g_mod[0] = '\0';
	size_t len = strlen(mod);
	for (size_t i=len-1; i>=0 && i<len; i++)
	{
		if (mod[i] == '/')
		{
			if (i == len-1)
			{
				mod[i] = '\0';
				continue;
			}
			strcpy(g_mod, &mod[i]);
			break;
		}
	}
	if (g_mod[0] != '\0')
	{
		strcpy(g_mod, mod);
	}
}

void CGameConfigManager::OnSourceModAllInitialized()
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

void CGameConfigManager::OnSourceModAllShutdown()
{
	CloseGameConfigFile(g_pGameConf);
}

bool CGameConfigManager::LoadGameConfigFile(const char *file, IGameConfig **_pConfig, char *error, size_t maxlength)
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

void CGameConfigManager::CloseGameConfigFile(IGameConfig *cfg)
{
	CGameConfig *pConfig = (CGameConfig *)cfg;

	if (pConfig->DecRefCount() == 0)
	{
		delete pConfig;
	}
}
