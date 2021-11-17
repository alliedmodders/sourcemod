/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
 */

#include "common_logic.h"
#include <string.h>
#include <stdlib.h>
#include <sh_list.h>
#include <sh_string.h>
#include "GameConfigs.h"
#include "stringutil.h"
#include <IGameHelpers.h>
#include <ILibrarySys.h>
#include <IHandleSys.h>
#include <IMemoryUtils.h>
#include <ISourceMod.h>
#include <IRootConsoleMenu.h>
#include "common_logic.h"
#include "sm_crc32.h"
#include "MemoryUtils.h"
#include <am-string.h>
#include <bridge/include/ILogger.h>
#include <bridge/include/CoreProvider.h>

#if defined PLATFORM_POSIX
#include <dlfcn.h>
#endif

using namespace SourceHook;

GameConfigManager g_GameConfigs;
IGameConfig *g_pGameConf = NULL;
static char g_Game[256];
static char g_GameDesc[256] = {'!', '\0'};
static char g_GameName[256] = {'$', '\0'};
static const char *g_pParseEngine = NULL;

#define PSTATE_NONE						0
#define PSTATE_GAMES					1
#define PSTATE_GAMEDEFS					2
#define PSTATE_GAMEDEFS_OFFSETS			3
#define PSTATE_GAMEDEFS_OFFSETS_OFFSET	4
#define PSTATE_GAMEDEFS_KEYS			5
#define PSTATE_GAMEDEFS_KEYS_PLATFORM	6
#define PSTATE_GAMEDEFS_SUPPORTED		7
#define PSTATE_GAMEDEFS_SIGNATURES		8
#define PSTATE_GAMEDEFS_SIGNATURES_SIG	9
#define PSTATE_GAMEDEFS_CRC				10
#define PSTATE_GAMEDEFS_CRC_BINARY		11
#define PSTATE_GAMEDEFS_CUSTOM			12
#define PSTATE_GAMEDEFS_ADDRESSES		13
#define PSTATE_GAMEDEFS_ADDRESSES_ADDRESS	14
#define PSTATE_GAMEDEFS_ADDRESSES_ADDRESS_READ	15

#if defined PLATFORM_X86
#define PLATFORM_ARCH_SUFFIX		""
#elif defined PLATFORM_X64
#define PLATFORM_ARCH_SUFFIX		"64"
#endif

#if defined PLATFORM_WINDOWS
#define PLATFORM_NAME				"windows" PLATFORM_ARCH_SUFFIX
#elif defined PLATFORM_LINUX
#define PLATFORM_NAME				"linux" PLATFORM_ARCH_SUFFIX
#define PLATFORM_COMPAT_ALT			"mac" PLATFORM_ARCH_SUFFIX	/* Alternate platform name if game data is missing for primary one */
#elif defined PLATFORM_APPLE
#define PLATFORM_NAME				"mac" PLATFORM_ARCH_SUFFIX
#define PLATFORM_COMPAT_ALT			"linux" PLATFORM_ARCH_SUFFIX
#endif

struct TempSigInfo
{
	void Reset()
	{
		library[0] = '\0';
		sig[0] = '\0';
	}
	char sig[1024];
	char library[64];
} s_TempSig;
unsigned int s_ServerBinCRC;
bool s_ServerBinCRC_Ok = false;

static bool DoesGameMatch(const char *value)
{
#if defined PLATFORM_WINDOWS
	if (strcasecmp(value, g_Game) == 0 ||
#else
	if (strcmp(value, g_Game) == 0 ||
#endif
		strcmp(value, g_GameDesc) == 0 ||
		strcmp(value, g_GameName) == 0)
	{
		return true;
	}
	return false;
}

static bool DoesEngineMatch(const char *value)
{
	return strcmp(value, g_pParseEngine) == 0;
}

static inline bool DoesPlatformMatch(const char *platform)
{
	return strcmp(platform, PLATFORM_NAME) == 0;
}

static inline bool IsPlatformCompatible(const char *platform, bool *hadPrimaryMatch)
{
	if (DoesPlatformMatch(platform))
	{
#if defined PLATFORM_COMPAT_ALT
		*hadPrimaryMatch = true;
#endif
		return true;
	}

#if defined PLATFORM_COMPAT_ALT
	/* If entry hasn't been found for the primary platform name, check for compatible alternate */
	if (!*hadPrimaryMatch)
		return strcmp(platform, PLATFORM_COMPAT_ALT) == 0;
#endif

	return false;
}

static inline time_t GetFileModTime(const char *path)
{
	char filepath[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, filepath, sizeof(filepath), "gamedata/%s.txt", path);
#ifdef PLATFORM_WINDOWS
	struct _stat64 s;
	if (_stat64(filepath, &s) != 0)
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(filepath, &s) != 0)
#endif
	{
		return 0;
	}
	return s.st_mtime;
}

CGameConfig::CGameConfig(const char *file, const char *engine)
{
	strncopy(m_File, file, sizeof(m_File));

	m_CustomLevel = 0;
	m_CustomHandler = NULL;

	m_ModTime = GetFileModTime(file);

	if (!engine)
		m_pEngine = bridge->GetSourceEngineName();
	else
		m_pEngine = engine;

	if (strcmp(m_pEngine, "css") == 0 || strcmp(m_pEngine, "dods") == 0 || strcmp(m_pEngine, "hl2dm") == 0 || strcmp(m_pEngine, "tf2") == 0)
		this->SetBaseEngine("orangebox_valve");
	else if (strcmp(m_pEngine, "nucleardawn") == 0)
		this->SetBaseEngine("left4dead2");
	else
		this->SetBaseEngine(NULL);
}

CGameConfig::~CGameConfig()
{
	g_GameConfigs.RemoveCachedConfig(this);
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
				m_Game = name;
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
			else if ((strcmp(name, "#supported") == 0) && (m_Game == "#default"))
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
			else if (strcmp(name, "Addresses") == 0)
			{
				m_ParseState = PSTATE_GAMEDEFS_ADDRESSES;
			}
			else
			{
				if (g_GameConfigs.m_customHandlers.retrieve(name, &m_CustomHandler))
				{
					m_CustomLevel = 0;
					m_ParseState = PSTATE_GAMEDEFS_CUSTOM;
					m_CustomHandler->ReadSMC_ParseStart();
					break;
				}

				m_IgnoreLevel++;
			}
			break;
		}
	case PSTATE_GAMEDEFS_KEYS:
		{
			m_Key = name;
			m_ParseState = PSTATE_GAMEDEFS_KEYS_PLATFORM;
			matched_platform = false;
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS:
		{
			m_Prop.clear();
			m_Class.clear();
			m_offset = name;
			m_ParseState = PSTATE_GAMEDEFS_OFFSETS_OFFSET;
			matched_platform = false;
			break;
		}
	case PSTATE_GAMEDEFS_SIGNATURES:
		{
			m_offset = name;
			s_TempSig.Reset();
			m_ParseState = PSTATE_GAMEDEFS_SIGNATURES_SIG;
			matched_platform = false;
			break;
		}
	case PSTATE_GAMEDEFS_CRC:
		{
			char error[255];
			error[0] = '\0';
			if (strcmp(name, "server") != 0)
			{
				ke::SafeSprintf(error, sizeof(error), "Unrecognized library \"%s\"", name);
			} 
			else if (!s_ServerBinCRC_Ok)
			{
				FILE *fp;
				char path[PLATFORM_MAX_PATH];

				char binName[64];
				bridge->FormatSourceBinaryName(name, binName, sizeof(binName));

				g_pSM->BuildPath(Path_Game, path, sizeof(path), "bin/%s", binName);
				if ((fp = fopen(path, "rb")) == NULL)
				{
					ke::SafeSprintf(error, sizeof(error), "Could not open binary: %s", path);
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
				logger->LogError("[SM] Error while parsing CRC section for \"%s\" (%s):", m_Game.c_str(), m_CurFile);
				logger->LogError("[SM] %s", error);
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
	case PSTATE_GAMEDEFS_ADDRESSES:
		{
			m_Address.clear();
			m_AddressSignature.clear();
			m_AddressReadCount = 0;
			m_AddressLastIsOffset = false;

			m_Address = name;
			m_ParseState = PSTATE_GAMEDEFS_ADDRESSES_ADDRESS;

			break;
		}
	case PSTATE_GAMEDEFS_ADDRESSES_ADDRESS:
		{
			if (DoesPlatformMatch(name))
			{
				m_ParseState = PSTATE_GAMEDEFS_ADDRESSES_ADDRESS_READ;
			}
			else
			{
				if (strcmp(name, "linux") != 0 && strcmp(name, "windows") != 0 && strcmp(name, "mac") != 0 &&
					strcmp(name, "linux64") != 0 && strcmp(name, "windows64") != 0 && strcmp(name, "mac64") != 0)
				{
					logger->LogError("[SM] Error while parsing Address section for \"%s\" (%s):", m_Address.c_str(), m_CurFile);
					logger->LogError("[SM] Unrecognized platform \"%s\"", name);
				}
				m_IgnoreLevel = 1;
			}
			break;
		}
	/* No sub-sections allowed:
	 case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
	 case PSTATE_GAMEDEFS_KEYS_PLATFORM:
	 case PSTATE_GAMEDEFS_SUPPORTED:
	 case PSTATE_GAMEDEFS_SIGNATURES_SIG:
	 case PSTATE_GAMEDEFS_CRC_BINARY:
	 case PSTATE_GAMEDEFS_ADDRESSES_ADDRESS_READ:
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
			m_Class = value;
		} else if (strcmp(key, "prop") == 0) {
			m_Prop = value;
		} else if (IsPlatformCompatible(key, &matched_platform)) {
			m_Offsets.replace(m_offset.c_str(), static_cast<int>(strtol(value, NULL, 0)));
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_KEYS) {
		std::string vstr(value);
		m_Keys.replace(key, std::move(vstr));
	}
	else if (m_ParseState == PSTATE_GAMEDEFS_KEYS_PLATFORM) {
		if (IsPlatformCompatible(key, &matched_platform))
		{
			std::string vstr(value);
			m_Keys.replace(m_Key.c_str(), std::move(vstr));
		}
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
		if (IsPlatformCompatible(key, &matched_platform))
		{
			strncopy(s_TempSig.sig, value, sizeof(s_TempSig.sig));
		} else if (strcmp(key, "library") == 0) {
			strncopy(s_TempSig.library, value, sizeof(s_TempSig.library));
		}
	} else if (m_ParseState == PSTATE_GAMEDEFS_CRC_BINARY) {
		if (DoesPlatformMatch(key)
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
	} else if (m_ParseState == PSTATE_GAMEDEFS_ADDRESSES_ADDRESS || m_ParseState == PSTATE_GAMEDEFS_ADDRESSES_ADDRESS_READ) {
		if (strcmp(key, "read") == 0 || strcmp(key, "offset") == 0) {
			int limit = sizeof(m_AddressRead)/sizeof(m_AddressRead[0]);
			if (m_AddressLastIsOffset)
			{
				logger->LogError("[SM] Error parsing Address \"%s\", 'offset' entry must be the last entry (gameconf \"%s\")", m_Address.c_str(), m_CurFile);
			}
			else if (m_AddressReadCount < limit)
			{
				if (strcmp(key, "offset") == 0)
				{
					m_AddressLastIsOffset = true;
				}
				m_AddressRead[m_AddressReadCount] = static_cast<int>(strtol(value, NULL, 0));
				m_AddressReadCount++;
			}
			else
			{
				logger->LogError("[SM] Error parsing Address \"%s\", does not support more than %d read offsets (gameconf \"%s\")", m_Address.c_str(), limit, m_CurFile);
			}
		} else if (strcmp(key, "signature") == 0) {
			m_AddressSignature = value;
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
	case PSTATE_GAMEDEFS_KEYS_PLATFORM:
		{
			m_ParseState = PSTATE_GAMEDEFS_KEYS;
			break;
		}
	case PSTATE_GAMEDEFS_OFFSETS_OFFSET:
		{
			/* Parse the offset... */
			if (!m_Class.empty() && !m_Prop.empty())
			{
				SendProp *pProp = gamehelpers->FindInSendTable(m_Class.c_str(), m_Prop.c_str());
				if (pProp)
				{
					int val = gamehelpers->GetSendPropOffset(pProp);
					m_Offsets.replace(m_offset.c_str(), val);
					m_Props.replace(m_offset.c_str(), pProp);
				} else {
					/* Check if it's a non-default game and no offsets exist */
					if ((m_Game != "*" && m_Game != "#default")
						&& (!m_Offsets.retrieve(m_offset.c_str())))
					{
						logger->LogError("[SM] Unable to find property %s.%s (file \"%s\") (mod \"%s\")", 
							m_Class.c_str(),
							m_Prop.c_str(),
							m_CurFile,
							m_Game.c_str());
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
				addrInBase = bridge->serverFactory;
			} else if (strcmp(s_TempSig.library, "engine") == 0) {
				addrInBase = bridge->engineFactory;
			} else if (strcmp(s_TempSig.library, "matchmaking_ds") == 0) {
				addrInBase = bridge->matchmakingDSFactory;
			}
			void *final_addr = NULL;
			if (addrInBase == NULL)
			{
				logger->LogError("[SM] Unrecognized library \"%s\" (gameconf \"%s\")", 
					s_TempSig.library, 
					m_CurFile);
			}
			else if (s_TempSig.sig[0])
			{
				if (s_TempSig.sig[0] == '@')
				{
#if defined PLATFORM_WINDOWS
					MEMORY_BASIC_INFORMATION mem;

					if (VirtualQuery(addrInBase, &mem, sizeof(mem)))
						final_addr = g_MemUtils.ResolveSymbol(mem.AllocationBase, &s_TempSig.sig[1]);
					else
						logger->LogError("[SM] Unable to find library \"%s\" in memory (gameconf \"%s\")", s_TempSig.library, m_File);
#elif defined PLATFORM_POSIX
					Dl_info info;
					/* GNU only: returns 0 on error, inconsistent! >:[ */
					if (dladdr(addrInBase, &info) != 0)
					{
						void *handle = dlopen(info.dli_fname, RTLD_NOW);
						if (handle)
						{
							if (bridge->SymbolsAreHidden())
								final_addr = g_MemUtils.ResolveSymbol(handle, &s_TempSig.sig[1]);
							else
								final_addr = dlsym(handle, &s_TempSig.sig[1]);
							dlclose(handle);
						} else {
							logger->LogError("[SM] Unable to load library \"%s\" (gameconf \"%s\")",
								s_TempSig.library,
								m_File);
						}
					} else {
						logger->LogError("[SM] Unable to find library \"%s\" in memory (gameconf \"%s\")",
							s_TempSig.library,
							m_File);
					}
#endif
				}

				if (!final_addr)
				{
					/* First, preprocess the signature */
					unsigned char real_sig[511];
					size_t real_bytes;
					size_t length;

					real_bytes = 0;
					length = strlen(s_TempSig.sig);

					real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), s_TempSig.sig);

					if (real_bytes >= 1)
					{
						final_addr = g_MemUtils.FindPattern(addrInBase, (char*) real_sig, real_bytes);
					}
				}

				m_Sigs.replace(m_offset.c_str(), final_addr);
			}

			m_ParseState = PSTATE_GAMEDEFS_SIGNATURES;

			break;
		}
	case PSTATE_GAMEDEFS_ADDRESSES:
		{
			m_ParseState = PSTATE_GAMEDEFS;
			break;
		}
	case PSTATE_GAMEDEFS_ADDRESSES_ADDRESS:
		{
			m_ParseState = PSTATE_GAMEDEFS_ADDRESSES;

			if (!m_Address.empty() && !m_AddressSignature.empty())
			{
				AddressConf addrConf(std::move(m_AddressSignature), m_AddressReadCount, m_AddressRead, m_AddressLastIsOffset);
				m_Addresses.replace(m_Address.c_str(), addrConf);
			}

			break;
		}
	case PSTATE_GAMEDEFS_ADDRESSES_ADDRESS_READ:
		{
			m_ParseState = PSTATE_GAMEDEFS_ADDRESSES_ADDRESS;
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
	m_Offsets.clear();
	m_Props.clear();
	m_Keys.clear();
	m_Addresses.clear();

	char path[PLATFORM_MAX_PATH];

	/* See if we can use the extended gamedata format. */
	g_pSM->BuildPath(Path_SM, path, sizeof(path), "gamedata/%s/master.games.txt", m_File);
	if (!libsys->PathExists(path))
	{
		/* Nope, use the old mechanism. */
		ke::SafeSprintf(path, sizeof(path), "%s.txt", m_File);
		if (!EnterFile(path, error, maxlength))
		{
			return false;
		}

		/* Allow customization. */
		g_pSM->BuildPath(Path_SM, path, sizeof(path), "gamedata/custom/%s.txt", m_File);
		if (libsys->PathExists(path))
		{
			ke::SafeSprintf(path, sizeof(path), "custom/%s.txt", m_File);
			bool success = EnterFile(path, error, maxlength);
			if (success)
			{
				rootmenu->ConsolePrint("[SM] Parsed custom gamedata override file: %s", path);
			}
			return success;
		}
		return true;
	}

	/* Otherwise, it's time to parse the master. */
	SMCError err;
	SMCStates state = {0, 0};
	List<String> fileList;
	master_reader.fileList = &fileList;
	const char *pEngine[2] = { m_pBaseEngine, m_pEngine  };

	for (unsigned char iter = 0; iter < SM_ARRAYSIZE(pEngine); ++iter)
	{
		if (pEngine[iter] == NULL)
		{
			continue;
		}

		this->SetParseEngine(pEngine[iter]);
		err = textparsers->ParseSMCFile(path, &master_reader, &state, error, maxlength);
		if (err != SMCError_Okay)
		{
			const char *msg = textparsers->GetSMCErrorString(err);

			logger->LogError("[SM] Error parsing master gameconf file \"%s\":", path);
			logger->LogError("[SM] Error %d on line %d, col %d: %s", 
				err,
				state.line,
				state.col,
				msg ? msg : "Unknown error");
			return false;
		}
	}

	/* Go through each file we found and parse it. */
	List<String>::iterator iter;
	for (iter = fileList.begin(); iter != fileList.end(); iter++)
	{
		ke::SafeSprintf(path, sizeof(path), "%s/%s", m_File, (*iter).c_str());
		if (!EnterFile(path, error, maxlength))
		{
			return false;
		}
	}

	/* Parse the contents of the 'custom' directory */
	g_pSM->BuildPath(Path_SM, path, sizeof(path), "gamedata/%s/custom", m_File);
	IDirectory *customDir = libsys->OpenDirectory(path);

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

		ke::SafeSprintf(path, sizeof(path), "%s/custom/%s", m_File, curFile);
		if (!EnterFile(path, error, maxlength))
		{
			libsys->CloseDirectory(customDir);
			return false;
		}

		rootmenu->ConsolePrint("[SM] Parsed custom gamedata override file: %s", path);

		customDir->NextEntry();
	}

	libsys->CloseDirectory(customDir);
	return true;
}

bool CGameConfig::EnterFile(const char *file, char *error, size_t maxlength)
{
	SMCError err;
	SMCStates state = {0, 0};

	g_pSM->BuildPath(Path_SM, m_CurFile, sizeof(m_CurFile), "gamedata/%s", file);

	/* Initialize parse states */
	m_IgnoreLevel = 0;
	bShouldBeReadingDefault = true;
	m_ParseState = PSTATE_NONE;
	const char *pEngine[2] = { m_pBaseEngine, m_pEngine };

	for (unsigned char iter = 0; iter < SM_ARRAYSIZE(pEngine); ++iter)
	{
		if (pEngine[iter] == NULL)
		{
			continue;
		}

		this->SetParseEngine(pEngine[iter]);
		if ((err=textparsers->ParseSMCFile(m_CurFile, this, &state, error, maxlength))
			!= SMCError_Okay)
		{
			const char *msg = textparsers->GetSMCErrorString(err);

			logger->LogError("[SM] Error parsing gameconfig file \"%s\":", m_CurFile);
			logger->LogError("[SM] Error %d on line %d, col %d: %s", 
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
	}

	return true;
}

void CGameConfig::SetBaseEngine(const char *engine)
{
	m_pBaseEngine = engine;
}

void CGameConfig::SetParseEngine(const char *engine)
{
	g_pParseEngine = engine;
}

bool CGameConfig::GetOffset(const char *key, int *value)
{
	return m_Offsets.retrieve(key, value);
}

const char *CGameConfig::GetKeyValue(const char *key)
{
	StringHashMap<std::string>::Result r = m_Keys.find(key);
	if (!r.found())
		return NULL;
	return r->value.c_str();
}

//memory addresses below 0x10000 are automatically considered invalid for dereferencing
#define VALID_MINIMUM_MEMORY_ADDRESS 0x10000

bool CGameConfig::GetAddress(const char *key, void **retaddr)
{
	StringHashMap<AddressConf>::Result r = m_Addresses.find(key);
	if (!r.found())
	{
		*retaddr = NULL;
		return false;
	}

	AddressConf &addrConf = r->value;

	void *addr;
	if (!GetMemSig(addrConf.signatureName.c_str(), &addr))
	{
		*retaddr = NULL;
		return false;
	}

	for (int i = 0; i < addrConf.readCount; ++i)
	{
		int offset = addrConf.read[i];

		//NULLs in the middle of an indirection chain are bad, end NULL is ok
		if (addr == NULL || reinterpret_cast<uintptr_t>(addr) < VALID_MINIMUM_MEMORY_ADDRESS)
		{
			*retaddr = NULL;
			return false;
		}
		//If lastIsOffset is set and this is the last iteration of the loop, don't deref
		if (addrConf.lastIsOffset && i == addrConf.readCount-1) {
			addr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(addr) + offset);
		} else {
			addr = *(reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(addr) + offset));
		}
	}

	*retaddr = addr;
	return true;
}

static inline unsigned minOf(unsigned a, unsigned b)
{
	return a <= b ? a : b;
}

CGameConfig::AddressConf::AddressConf(std::string&& sigName, unsigned readCount, int *read, bool lastIsOffset)
{
	unsigned readLimit = minOf(readCount, sizeof(this->read) / sizeof(this->read[0]));

	this->signatureName = std::move(sigName);
	this->readCount = readLimit;
	memcpy(&this->read[0], read, sizeof(this->read[0])*readLimit);

	//For safety: if the readLimit isn't the same as the readCount, then the
	//last read value was definitely discarded, so lastIsOffset should be ignored
	this->lastIsOffset = readLimit < readCount ? false : lastIsOffset;
}

SendProp *CGameConfig::GetSendProp(const char *key)
{
	SendProp *prop;
	if (!m_Props.retrieve(key, &prop))
		return NULL;
	return prop;
}

bool CGameConfig::GetMemSig(const char *key, void **addr)
{
	return m_Sigs.retrieve(key, addr);
}

GameConfigManager::GameConfigManager()
{
}

GameConfigManager::~GameConfigManager()
{
}

void GameConfigManager::OnSourceModStartup(bool late)
{
	LoadGameConfigFile("core.games", &g_pGameConf, NULL, 0);

	strncopy(g_Game, g_pSM->GetGameFolderName(), sizeof(g_Game));
	strncopy(g_GameDesc + 1, bridge->GetGameDescription(), sizeof(g_GameDesc) - 1);
	bridge->GetGameName(g_GameName + 1, sizeof(g_GameName) - 1);
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

	sharesys->AddInterface(NULL, this);
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

	if (m_Lookup.retrieve(file, &pConfig))
	{
		bool ret = true;
		time_t modtime = GetFileModTime(file);
		if (pConfig->m_ModTime != modtime)
		{
			pConfig->m_ModTime = modtime;
			ret = pConfig->Reparse(error, maxlength);
		}

		pConfig->AddRef();
		*_pConfig = pConfig;
		return ret;
	}

	pConfig = new CGameConfig(file);
	pConfig->AddRef();

	/* :HACKHACK: Don't parse the main config file */
	bool retval = true;
	if (_pConfig != &g_pGameConf)
	{
		retval = pConfig->Reparse(error, maxlength);
	}

	m_Lookup.insert(file, pConfig);

	*_pConfig = pConfig;
	return retval;
}

void GameConfigManager::CloseGameConfigFile(IGameConfig *cfg)
{
	CGameConfig *pConfig = (CGameConfig *)cfg;
	pConfig->Release();
}

extern Handle_t g_GameConfigsType;

IGameConfig *GameConfigManager::ReadHandle(Handle_t hndl, IdentityToken_t *ident, HandleError *err)
{
	HandleSecurity sec(ident, g_pCoreIdent);
	IGameConfig *conf = NULL;
	HandleError _err = handlesys->ReadHandle(hndl, g_GameConfigsType, &sec, (void **)&conf);

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

void GameConfigManager::RemoveUserConfigHook(const char *sectionname, ITextListener_SMC *aListener)
{
	ITextListener_SMC *listener;
	if (!m_customHandlers.retrieve(sectionname, &listener))
		return;

	if (listener != aListener)
		return;

	m_customHandlers.remove(sectionname);
	return;
}

void GameConfigManager::AcquireLock()
{
}

void GameConfigManager::ReleaseLock()
{
}

void GameConfigManager::RemoveCachedConfig(CGameConfig *config)
{
	m_Lookup.remove(config->m_File);
}
