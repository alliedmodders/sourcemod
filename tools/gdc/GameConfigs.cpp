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
//#include "sm_stringutil.h"
//#include "sourcemod.h"
//#include "sourcemm_api.h"
//#include "HalfLife2.h"
//#include "Logger.h"
//#include "ShareSys.h"
//#include "MemoryUtils.h"
//#include "LibrarySys.h"
//#include "HandleSys.h"
//#include "sm_crc32.h"

//#if defined PLATFORM_LINUX
//#include <dlfcn.h>
//#endif

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

#define WIN 0
#define LIN 1

#define PLATFORM_NAME				"linux"
#define PLATFORM_SERVER_BINARY		"server_i486.so"

Offset *tempOffset;
Sig *tempSig;

unsigned int s_ServerBinCRC;
bool s_ServerBinCRC_Ok = false;

bool /*CGameConfig::*/DoesGameMatch(const char *value)
{
	if (strcmp(value, /*m_gdcG*/game) == 0)
	{
		return true;
	}
	return false;
}

bool /*CGameConfig::*/DoesEngineMatch(const char *value)
{
	if (strcmp(value, /*m_gdcE*/engine) == 0)
	{
		return true;
	}
	return false;
}

CGameConfig::CGameConfig()//const char *file)//, const char *game, const char *engine)
{
//	strncopy(m_File, file, sizeof(m_File));
//	strncopy(m_gdcGame, game, sizeof(m_gdcGame));
//	strncopy(m_gdcEngine, engine, sizeof(m_gdcEngine));
	//m_pStrings = new BaseStringTable(512);
	m_RefCount = 0;

	m_CustomLevel = 0;
	m_CustomHandler = NULL;
}

CGameConfig::~CGameConfig()
{
//	delete m_pStrings;
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
#if 0
				m_ParseState = PSTATE_GAMEDEFS_CRC;
				bShouldBeReadingDefault = false;
#endif
			}
			else
			{
#if 0
				ITextListener_SMC **pListen = g_GameConfigs.m_customHandlers.retrieve(name);

				if (pListen != NULL)
				{
					m_CustomLevel = 0;
					m_ParseState = PSTATE_GAMEDEFS_CUSTOM;
					m_CustomHandler = *pListen;
					m_CustomHandler->ReadSMC_ParseStart();
					break;
				}
#endif
			}
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS:
		{
			m_Prop[0] = '\0';
			m_Class[0] = '\0';
			tempOffset = new Offset();
			tempOffset->setName(name);
			m_ParseState = PSTATE_GAMEDEFS_OFFSETS_OFFSET;
			break;
		}
	case PSTATE_GAMEDEFS_SIGNATURES:
		{
			tempSig = new Sig();
			tempSig->setName(name);
			m_ParseState = PSTATE_GAMEDEFS_SIGNATURES_SIG;
			break;
		}
#if 0
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
#endif
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
		} else /*if (strcmp(key, PLATFORM_NAME) == 0) */ {
			int val = atoi(value);
//			sm_trie_replace(m_pOffsets, m_offset, (void *)val);
			if (strcmp(key, "windows") == 0) tempOffset->setWin(val);
			else if (strcmp(key, "linux") == 0) tempOffset->setLin(val);
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		m_Keys[key] = value;
//		printf("Inserting %s - %s\n", key, value);
//		int id = m_pStrings->AddString(value);
//		sm_trie_replace(m_pKeys, key, (void *)id);
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
		if (strcmp(key, "windows") == 0) tempSig->setWin(value);
		else if (strcmp(key, "linux") == 0) tempSig->setLin(value);
		else if (strcmp(key, "library") == 0) tempSig->setLib(value);
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
			m_Offsets.push_back(*tempOffset);
#if 0
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
#endif
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
			m_Sigs.push_back(*tempSig);
#if 0
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
#endif
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
	list<char*> *fileList;
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
//	m_pStrings->Reset();
	m_Offsets.clear();
	m_Sigs.clear();
	

	char path[PLATFORM_MAX_PATH];

	/* See if we can use the extended gamedata format. */
	//TODO pass in path to gamedata somehow
//	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "gamedata/%s/master.games.txt", m_File);

	/* Otherwise, it's time to parse the master. */
	SMCError err;
	SMCStates state = {0, 0};
	list<char*> fileList;
	master_reader.fileList = &fileList;

	err = textparsers->ParseSMCFile(path, &master_reader, &state, error, maxlength);
	if (err != SMCError_Okay)
	{
		const char *msg = textparsers->GetSMCErrorString(err);

		printf("[SM] Error parsing master gameconf file \"%s\":", path);
		printf("[SM] Error %d on line %d, col %d: %s", 
			err,
			state.line,
			state.col,
			msg ? msg : "Unknown error");
		exit(PARSE_ERROR);
	}

	/* Go through each file we found and parse it. */
	list<char*>::iterator iter;
	for (iter = fileList.begin(); iter != fileList.end(); iter++)
	{
		UTIL_Format(path, sizeof(path), "%s/%s", m_File, *iter);
		if (!EnterFile(path, error, maxlength))
		{
			return false;
		}
	}

	return true;
}

bool CGameConfig::EnterFile(const char *file, char *error, size_t maxlength)
{
	SMCError err;
	SMCStates state = {0, 0};

	/* Initialize parse states */
	m_IgnoreLevel = 0;
	bShouldBeReadingDefault = true;
	m_ParseState = PSTATE_NONE;

	if ((err=textparsers->ParseSMCFile(file, this, &state, error, maxlength))
		!= SMCError_Okay)
	{
		const char *msg;

		msg = textparsers->GetSMCErrorString(err);

		printf("[SM] Error parsing gameconfig file \"%s\":\n", file);
		printf("[SM] Error %d on line %d, col %d: %s\n", 
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

		exit(PARSE_ERROR);
	}

	return true;
}

#if 0
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
#endif

const char *CGameConfig::GetKeyValue(const char *key)
{
	map<const char*,const char*,cmp_str>::iterator it = m_Keys.find(key);
	if (it == m_Keys.end()) return NULL;

	return it->second;
}

list<Offset> CGameConfig::GetOffsets() { return m_Offsets; }
list<Sig> CGameConfig::GetSigs() { return m_Sigs; }

