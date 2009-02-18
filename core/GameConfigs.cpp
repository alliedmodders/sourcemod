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

#include <string.h>
#include <stdlib.h>
#include "GameConfigs.h"
#include "sm_stringutil.h"
#include "sourcemod.h"
#include "sourcemm_api.h"
#include "HalfLife2.h"
#include "Logger.h"
#include "ShareSys.h"
#include "MemoryUtils.h"
#include "LibrarySys.h"
#include "HandleSys.h"
#include "sm_crc32.h"

#if defined PLATFORM_LINUX
#include <dlfcn.h>
#endif

GameConfigManager g_GameConfigs;
IGameConfig *g_pGameConf = NULL;
char g_Game[256];
char g_GameDesc[256] = {'!', '\0'};
char g_GameName[256] = {'$', '\0'};

#define PSTATE_NONE						0
#define PSTATE_GAMES					1
#define PSTATE_GAMEDEFS					2
#define PSTATE_GAMEDEFS_OFFSETS			3
#define PSTATE_GAMEDEFS_OFFSETS_OFFSET	4
#define PSTATE_GAMEDEFS_KEYS			5
#define PSTATE_GAMEDEFS_SUPPORTED		6
#define PSTATE_GAMEDEFS_SIGNATURES		7
#define PSTATE_GAMEDEFS_SIGNATURES_SIG	8
#define PSTATE_GAMEDEFS_CRC				9
#define PSTATE_GAMEDEFS_CRC_BINARY		10
#define PSTATE_GAMEDEFS_CUSTOM			11

#if defined PLATFORM_WINDOWS
#define PLATFORM_NAME				"windows"
#define PLATFORM_SERVER_BINARY		"server.dll"
#elif defined PLATFORM_LINUX
#define PLATFORM_NAME				"linux"
#define PLATFORM_SERVER_BINARY		"server_i486.so"
#elif defined PLATFORM_APPLE
#define PLATFORM_NAME               "undef"
#define PLATFORM_SERVER_BINARY      "undef.dylib"
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
unsigned int s_ServerBinCRC;
bool s_ServerBinCRC_Ok = false;

static bool DoesGameMatch(const char *value)
{
	if (strcmp(value, g_Game) == 0 ||
		strcmp(value, g_GameDesc) == 0 ||
		strcmp(value, g_GameName) == 0)
	{
		return true;
	}
	return false;
}

static bool DoesEngineMatch(const char *value)
{
#if SOURCE_ENGINE == SE_EPISODEONE
	if (strcmp(value, "original") == 0)
#elif SOURCE_ENGINE == SE_ORANGEBOX
	if (strcmp(value, "orangebox") == 0)
#elif SOURCE_ENGINE == SE_LEFT4DEAD
	if (strcmp(value, "left4dead") == 0)
#else
#error "Unknown engine type"
#endif
	{
		return true;
	}
	return false;
}

CGameConfig::CGameConfig(const char *file)
{
	strncopy(m_File, file, sizeof(m_File));
	m_pOffsets = sm_trie_create();
	m_pProps = sm_trie_create();
	m_pKeys = sm_trie_create();
	m_pSigs = sm_trie_create();
	m_pStrings = new BaseStringTable(512);
	m_RefCount = 0;

	m_CustomLevel = 0;
	m_CustomHandler = NULL;
}

CGameConfig::~CGameConfig()
{
	sm_trie_destroy(m_pOffsets);
	sm_trie_destroy(m_pProps);
	sm_trie_destroy(m_pKeys);
	sm_trie_destroy(m_pSigs);
	delete m_pStrings;
}

SMCResult CGameConfig::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (m_IgnoreLevel)
	{
		m_IgnoreLevel++;
		return SMCResult_Continue;
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
			if (strcmp(name, "*") == 0 ||
				strcmp(name, "#default") == 0 ||
				DoesGameMatch(name))
			{
				bShouldBeReadingDefault = true;
				m_ParseState = PSTATE_GAMEDEFS;
				strncopy(m_Game, name, sizeof(m_Game));
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
			else if ((strcmp(name, "#supported") == 0) && (strcmp(m_Game, "#default") == 0))
			{
				m_ParseState = PSTATE_GAMEDEFS_SUPPORTED;
				/* Ignore this section unless we get a game. */
				bShouldBeReadingDefault = false;
				had_game = false;
				matched_game = false;
				had_engine = false;
				matched_engine = false;
			}
			else if (strcmp(name, "Signatures") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_SIGNATURES;
			}
			else if (strcmp(name, "CRC") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_CRC;
				bShouldBeReadingDefault = false;
			}
			else
			{
				ITextListener_SMC **pListen = g_GameConfigs.m_customHandlers.retrieve(name);

				if (pListen != NULL)
				{
					m_CustomLevel = 0;
					m_ParseState = PSTATE_GAMEDEFS_CUSTOM;
					m_CustomHandler = *pListen;
					m_CustomHandler->ReadSMC_ParseStart();
					break;
				}

				m_IgnoreLevel++;
			}
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS:
		{
			m_Prop[0] = '\0';
			m_Class[0] = '\0';
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
	case PSTATE_GAMEDEFS_CRC:
		{
			char error[255];
			error[0] = '\0';
			if (strcmp(name, "server") != 0)
			{
				UTIL_Format(error, sizeof(error), "Unrecognized library \"%s\"", name);
			} 
			else if (!s_ServerBinCRC_Ok)
			{
				FILE *fp;
				char path[PLATFORM_MAX_PATH];

				g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "bin/" PLATFORM_SERVER_BINARY);
				if ((fp = fopen(path, "rb")) == NULL)
				{
					UTIL_Format(error, sizeof(error), "Could not open binary: %s", path);
				} else {
					size_t size;
					void *buffer;

					fseek(fp, 0, SEEK_END);
					size = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					buffer = malloc(size);
					fread(buffer, size, 1, fp);
					s_ServerBinCRC = UTIL_CRC32(buffer, size);
					free(buffer);
					s_ServerBinCRC_Ok = true;
					fclose(fp);
				}
			}
			if (error[0] != '\0')
			{
				m_IgnoreLevel = 1;
				g_Logger.LogError("[SM] Error while parsing CRC section for \"%s\" (%s):", m_Game, m_CurFile);
				g_Logger.LogError("[SM] %s", error);
			} else {
				m_ParseState = PSTATE_GAMEDEFS_CRC_BINARY;
			}
			break;
		}
	case PSTATE_GAMEDEFS_CUSTOM:
		{
			m_CustomLevel++;
			return m_CustomHandler->ReadSMC_NewSection(states, name);
			break;
		}
	/* No sub-sections allowed:
	 case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
	 case PSTATE_GAMEDEFS_KEYS:
	 case PSTATE_GAMEDEFS_SUPPORTED:
	 case PSTATE_GAMEDEFS_SIGNATURES_SIG:
	 case PSTATE_GAMEDEFS_CRC_BINARY:
	 */
	default:
		{
			/* If we don't know what we got, start ignoring */
			m_IgnoreLevel++;
			break;
		}
	}

	return SMCResult_Continue;
}

SMCResult CGameConfig::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	if (m_IgnoreLevel)
	{
		return SMCResult_Continue;
	}

	if (m_ParseState == PSTATE_GAMEDEFS_OFFSETS_OFFSET)
	{
		if (strcmp(key, "class") == 0)
		{
			strncopy(m_Class, value, sizeof(m_Class));
		} else if (strcmp(key, "prop") == 0) {
			strncopy(m_Prop, value, sizeof(m_Prop));
		} else if (strcmp(key, PLATFORM_NAME) == 0) {
			int val = atoi(value);
			sm_trie_replace(m_pOffsets, m_offset, (void *)val);
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		int id = m_pStrings->AddString(value);
		sm_trie_replace(m_pKeys, key, (void *)id);
	} else if (m_ParseState == PSTATE_GAMEDEFS_SUPPORTED) {
		if (strcmp(key, "game") == 0)
		{
			had_game = true;
			if (DoesGameMatch(value))
			{
				matched_game = true;
			}
			if ((!had_engine && matched_game) || (matched_engine && matched_game))
			{
				bShouldBeReadingDefault = true;
			}
		}
		else if (strcmp(key, "engine") == 0)
		{
			had_engine = true;
			if (DoesEngineMatch(value))
			{
				matched_engine = true;
			}
			if ((!had_game && matched_engine) || (matched_game && matched_engine))
			{
				bShouldBeReadingDefault = true;
			}
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_SIGNATURES_SIG) {
		if (strcmp(key, PLATFORM_NAME) == 0)
		{
			strncopy(s_TempSig.sig, value, sizeof(s_TempSig.sig));
		} else if (strcmp(key, "library") == 0) {
			strncopy(s_TempSig.library, value, sizeof(s_TempSig.library));
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_CRC_BINARY) {
		if (strcmp(key, PLATFORM_NAME) == 0 
			&& s_ServerBinCRC_Ok
			&& !bShouldBeReadingDefault)
		{
			unsigned int crc = 0;
			sscanf(value, "%08X", &crc);
			if (s_ServerBinCRC == crc)
			{
				bShouldBeReadingDefault = true;
			}
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_CUSTOM) {
		return m_CustomHandler->ReadSMC_KeyValue(states, key, value);
	}

	return SMCResult_Continue;
}

SMCResult CGameConfig::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (m_IgnoreLevel)
	{
		m_IgnoreLevel--;
		return SMCResult_Continue;
	}

	if (m_CustomLevel)
	{
		m_CustomLevel--;
		m_CustomHandler->ReadSMC_LeavingSection(states);
		return SMCResult_Continue;
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
	case PSTATE_GAMEDEFS_CUSTOM:
		{
			m_ParseState = PSTATE_GAMEDEFS;
			m_CustomHandler->ReadSMC_ParseEnd(false, false);
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
			if (m_Class[0] != '\0'
				&& m_Prop[0] != '\0')
			{
				SendProp *pProp = g_HL2.FindInSendTable(m_Class, m_Prop);
				if (pProp)
				{
					int val = pProp->GetOffset();
					sm_trie_replace(m_pOffsets, m_offset, (void *)val);
					sm_trie_replace(m_pProps, m_offset, pProp);
				} else {
					/* Check if it's a non-default game and no offsets exist */
					if (((strcmp(m_Game, "*") != 0) && strcmp(m_Game, "#default") != 0)
						&& (!sm_trie_retrieve(m_pOffsets, m_offset, NULL)))
					{
						g_Logger.LogError("[SM] Unable to find property %s.%s (file \"%s\") (mod \"%s\")", 
							m_Class,
							m_Prop,
							m_CurFile,
							m_Game);
					}
				}
			}
			m_ParseState = PSTATE_GAMEDEFS_OFFSETS;
			break;
		}
	case PSTATE_GAMEDEFS_CRC:
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
	case PSTATE_GAMEDEFS_CRC_BINARY:
		{
			m_ParseState = PSTATE_GAMEDEFS_CRC;
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
				addrInBase = (void *)g_SMAPI->GetServerFactory(false);
			} else if (strcmp(s_TempSig.library, "engine") == 0) {
				addrInBase = (void *)g_SMAPI->GetEngineFactory(false);
			}
			void *final_addr = NULL;
			if (addrInBase == NULL)
			{
				g_Logger.LogError("[SM] Unrecognized library \"%s\" (gameconf \"%s\")", 
					s_TempSig.library, 
					m_CurFile);
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
								m_File);
						}
					} else {
						g_Logger.LogError("[SM] Unable to find library \"%s\" in memory (gameconf \"%s\")",
							s_TempSig.library,
							m_File);
					}
				}
				if (final_addr)
				{
					goto skip_find;
				}
#endif
				/* First, preprocess the signature */
				char real_sig[511];
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

	return SMCResult_Continue;
}

#define MSTATE_NONE		0
#define MSTATE_MAIN		1
#define MSTATE_FILE		2

class MasterReader : public ITextListener_SMC
{
public:
	virtual void ReadSMC_ParseStart()
	{
		state = MSTATE_NONE;
		ignoreLevel = 0;
	}

	virtual SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		if (ignoreLevel)
		{
			return SMCResult_Continue;
		}

		if (state == MSTATE_NONE)
		{
			if (strcmp(name, "Game Master") == 0)
			{
				state = MSTATE_MAIN;
			}
			else
			{
				ignoreLevel++;
			}
		}
		else if (state == MSTATE_MAIN)
		{
			strncopy(cur_file, name, sizeof(cur_file));
			had_engine = false;
			matched_engine = false;
			had_game = false;
			matched_game = false;
			state = MSTATE_FILE;
		}
		else if (state == MSTATE_FILE)
		{
			ignoreLevel++;
		}

		return SMCResult_Continue;
	}

	virtual SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		if (ignoreLevel || state != MSTATE_FILE)
		{
			return SMCResult_Continue;
		}

		if (strcmp(key, "engine") == 0)
		{
			had_engine = true;
			if (DoesEngineMatch(value))
			{
				matched_engine = true;
			}
		}
		else if (strcmp(key, "game") == 0)
		{
			had_game = true;
			if (DoesGameMatch(value))
			{
				matched_game = true;
			}
		}

		return SMCResult_Continue;
	}

	virtual SMCResult ReadSMC_LeavingSection(const SMCStates *states)
	{
		if (ignoreLevel)
		{
			ignoreLevel--;
			return SMCResult_Continue;
		}

		if (state == MSTATE_FILE)
		{
			/* The four success conditions:
			 * 1. Needed nothing.
			 * 2. Needed game only.
			 * 3. Needed engine only.
			 * 4. Needed both engine and game.
			 * Final result is minimized via k-map.
			 */
#if 0
			if ((!had_engine && !had_game) ||
				(!had_engine && (had_game && matched_game)) ||
				(!had_game && (had_engine && matched_engine)) ||
				((had_game && had_engine) && (matched_game && matched_engine)))
#endif
			if ((!had_engine && !had_game) ||
				(!had_engine && matched_game) ||
				(!had_game && matched_engine) ||
				(matched_engine && matched_game))
			{
				fileList->push_back(cur_file);
			}
			state = MSTATE_MAIN;
		}
		else if (state == MSTATE_MAIN)
		{
			state = MSTATE_NONE;
		}

		return SMCResult_Continue;
	}
public:
	List<String> *fileList;
	unsigned int state;
	unsigned int ignoreLevel;
	char cur_file[PLATFORM_MAX_PATH];
	bool had_engine;
	bool matched_engine;
	bool had_game;
	bool matched_game;
};

static MasterReader master_reader;

bool CGameConfig::Reparse(char *error, size_t maxlength)
{
	/* Reset cached data */
	m_pStrings->Reset();
	sm_trie_clear(m_pOffsets);
	sm_trie_clear(m_pProps);
	sm_trie_clear(m_pKeys);

	char path[PLATFORM_MAX_PATH];

	/* See if we can use the extended gamedata format. */
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "gamedata/%s/master.games.txt", m_File);
	if (!g_LibSys.PathExists(path))
	{
		/* Nope, use the old mechanism. */
		UTIL_Format(path, sizeof(path), "%s.txt", m_File);
		return EnterFile(path, error, maxlength);
	}

	/* Otherwise, it's time to parse the master. */
	SMCError err;
	SMCStates state = {0, 0};
	List<String> fileList;
	master_reader.fileList = &fileList;

	err = textparsers->ParseSMCFile(path, &master_reader, &state, error, maxlength);
	if (err != SMCError_Okay)
	{
		const char *msg = textparsers->GetSMCErrorString(err);

		g_Logger.LogError("[SM] Error parsing master gameconf file \"%s\":", path);
		g_Logger.LogError("[SM] Error %d on line %d, col %d: %s", 
			err,
			state.line,
			state.col,
			msg ? msg : "Unknown error");
		return false;
	}

	/* Go through each file we found and parse it. */
	List<String>::iterator iter;
	for (iter = fileList.begin(); iter != fileList.end(); iter++)
	{
		UTIL_Format(path, sizeof(path), "%s/%s", m_File, (*iter).c_str());
		if (!EnterFile(path, error, maxlength))
		{
			return false;
		}
	}

	/* Parse the contents of the 'custom' directory */
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "gamedata/%s/custom", m_File);
	IDirectory *customDir = g_LibSys.OpenDirectory(path);

	if (!customDir)
	{
		return true;
	}

	while (customDir->MoreFiles())
	{
		if (!customDir->IsEntryFile())
		{
			customDir->NextEntry();
			continue;
		}
		
		const char *curFile = customDir->GetEntryName();

		/* Only allow .txt files */
		int len = strlen(curFile);
		if (len > 4 && strcmp(&curFile[len-4], ".txt") != 0)
		{
			customDir->NextEntry();
			continue;	
		}

		UTIL_Format(path, sizeof(path), "%s/custom/%s", m_File, curFile);
		if (!EnterFile(path, error, maxlength))
		{
			g_LibSys.CloseDirectory(customDir);
			return false;
		}

		customDir->NextEntry();
	}

	g_LibSys.CloseDirectory(customDir);
	return true;
}

bool CGameConfig::EnterFile(const char *file, char *error, size_t maxlength)
{
	SMCError err;
	SMCStates state = {0, 0};

	g_SourceMod.BuildPath(Path_SM, m_CurFile, sizeof(m_CurFile), "gamedata/%s", file);

	/* Initialize parse states */
	m_IgnoreLevel = 0;
	bShouldBeReadingDefault = true;
	m_ParseState = PSTATE_NONE;

	if ((err=textparsers->ParseSMCFile(m_CurFile, this, &state, error, maxlength))
		!= SMCError_Okay)
	{
		const char *msg;

		msg = textparsers->GetSMCErrorString(err);

		g_Logger.LogError("[SM] Error parsing gameconfig file \"%s\":", m_CurFile);
		g_Logger.LogError("[SM] Error %d on line %d, col %d: %s", 
			err,
			state.line,
			state.col,
			msg ? msg : "Unknown error");

		if (m_ParseState == PSTATE_GAMEDEFS_CUSTOM)
		{
			//error occurred while parsing a custom section
			m_CustomHandler->ReadSMC_ParseEnd(true, true);
			m_CustomHandler = NULL;
			m_CustomLevel = 0;
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

	strncopy(g_Game, g_SourceMod.GetGameFolderName(), sizeof(g_Game));
	strncopy(g_GameDesc + 1, SERVER_CALL(GetGameDescription)(), sizeof(g_GameDesc) - 1);

	KeyValues *pGameInfo = new KeyValues("GameInfo");
	if (g_HL2.KVLoadFromFile(pGameInfo, basefilesystem, "gameinfo.txt"))
	{
		const char *str;
		if ((str = pGameInfo->GetString("game", NULL)) != NULL)
		{
			strncopy(g_GameName + 1, str, sizeof(g_GameName) - 1);
		}
	}
	pGameInfo->deleteThis();
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
#if 0
	/* A crash was detected during last load - We block the gamedata loading so it hopefully won't happen again */
	if (g_blockGameDataLoad)
	{
		UTIL_Format(error, maxlength, "GameData loaded blocked due to detected crash");
		return false;
	}
#endif

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
		sm_trie_delete(m_pLookup, pConfig->m_File);
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

void GameConfigManager::AddUserConfigHook(const char *sectionname, ITextListener_SMC *listener)
{
	m_customHandlers.insert(sectionname, listener);
}

void GameConfigManager::RemoveUserConfigHook(const char *sectionname, ITextListener_SMC *listener)
{
	ITextListener_SMC **pListener = m_customHandlers.retrieve(sectionname);

	if (pListener == NULL)
	{
		return;
	}

	if (*pListener != listener)
	{
		return;
	}

	m_customHandlers.remove(sectionname);
	
	return;
}

void GameConfigManager::AcquireLock()
{
}

void GameConfigManager::ReleaseLock()
{
}
