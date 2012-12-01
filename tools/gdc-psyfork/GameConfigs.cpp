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
#define PSTATE_GAMEDEFS_OPTIONS			12

#define WIN 0
#define LIN 1

#define PLATFORM_NAME				"linux"
#define PLATFORM_SERVER_BINARY		"server_i486.so"

Offset *tempOffset;
Sig *tempSig;

unsigned int s_ServerBinCRC;
bool s_ServerBinCRC_Ok = false;

bool DoesGameMatch(const char *value)
{
	if (strcmp(value, game) == 0)
	{
		return true;
	}
	return false;
}

bool DoesEngineMatch(const char *value)
{
	if (strcmp(value, engine) == 0)
	{
		return true;
	}
	return false;
}

CGameConfig::CGameConfig()
{
	m_pStrings = new BaseStringTable(512);
	m_RefCount = 0;

	m_CustomLevel = 0;
	m_CustomHandler = NULL;
}

CGameConfig::~CGameConfig()
{
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
			else if (strcmp(name, "Options") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_OPTIONS;
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
			else
			{
				m_IgnoreLevel++;
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
		} else {
			int val = atoi(value);
			if (strcmp(key, "windows") == 0) tempOffset->setWin(val);
			else if (strcmp(key, "linux") == 0) tempOffset->setLin(val);
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		m_Keys.replace(key, m_pStrings->AddString(value));
	} else if (m_ParseState == PSTATE_GAMEDEFS_OPTIONS) {
		m_Options.replace(key, m_pStrings->AddString(value));
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
	case PSTATE_GAMEDEFS_OPTIONS:
		{
			m_ParseState = PSTATE_GAMEDEFS;
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
		{
			for (list<Offset>::iterator it = m_Offsets.begin(); it != m_Offsets.end(); it++)
			{
				if (strcmp(it->name, tempOffset->name) == 0)
				{
					m_Offsets.erase(it);
					break;
				}
			}
			m_Offsets.push_back(*tempOffset);
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
	m_pStrings->Reset();
	m_Offsets.clear();
	m_Sigs.clear();
	m_Keys.clear();
	m_Options.clear();

	char path[PLATFORM_MAX_PATH];

	/* See if we can use the extended gamedata format. */
	//TODO pass in path to gamedata somehow

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

const char *CGameConfig::GetKeyValue(const char *key)
{
	int *pkey;
	if ((pkey = m_Keys.retrieve(key)) == NULL)
		return NULL;
	return m_pStrings->GetString(*pkey);
}

const char *CGameConfig::GetOptionValue(const char *key)
{
	int *pkey;
	if ((pkey = m_Options.retrieve(key)) == NULL)
		return NULL;
	return m_pStrings->GetString(*pkey);
}

list<Offset> CGameConfig::GetOffsets() { return m_Offsets; }
list<Sig> CGameConfig::GetSigs() { return m_Sigs; }

