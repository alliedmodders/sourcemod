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

#include "common_logic.h"
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sm_platform.h>
#include "Translator.h"
#include <IPlayerHelpers.h>
#include <ISourceMod.h>
#include <ILibrarySys.h>
#include "PhraseCollection.h"
#include "stringutil.h"
#include "sprintf.h"
#include <am-string.h>
#include <bridge/include/ILogger.h>
#include <bridge/include/CoreProvider.h>

Translator g_Translator;
IPhraseCollection *g_pCorePhrases = NULL;
unsigned int g_pCorePhraseID = 0;

using namespace SourceMod;

struct trans_t
{
	int stridx;
	int fmt_order;
};

struct phrase_t
{
	int fmt_list;
	unsigned int fmt_count;
	unsigned int fmt_bytes;
	int trans_tbl;
	unsigned int translations;
};

CPhraseFile::CPhraseFile(Translator *pTranslator, const char *file)
{
	m_pStringTab = pTranslator->GetStringTable();
	m_pMemory = m_pStringTab->GetMemTable();
	m_LangCount = pTranslator->GetLanguageCount();
	m_File.assign(file);
	m_pTranslator = pTranslator;
}

CPhraseFile::~CPhraseFile()
{
}

void CPhraseFile::ParseError(const char *message, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, message);
	vsnprintf(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	m_ParseError.assign(buffer);
}

void CPhraseFile::ParseWarning(const char *message, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, message);
	vsnprintf(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	if (!m_FileLogged)
	{
		logger->LogError("[SM] Warning(s) encountered in translation file \"%s\"", m_File.c_str());
		m_FileLogged = true;
	}

	logger->LogError("[SM] %s", buffer);
}

void CPhraseFile::ReparseFile()
{
	m_PhraseLookup.clear();

	m_LangCount = m_pTranslator->GetLanguageCount();

	if (!m_LangCount)
	{
		return;
	}

	SMCError err;
	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, path, PLATFORM_MAX_PATH, "translations/%s", m_File.c_str());

	//backwards compatibility shim
	/* :HACKHACK: Change .cfg/.txt and vice versa for compatibility */
	if (!libsys->PathExists(path))
	{
		if (m_File.compare("common.cfg") == 0)
		{
			UTIL_ReplaceAll(path, sizeof(path), "common.cfg", "common.phrases.txt", true);
		} else if (strstr(path, ".cfg")) {
			UTIL_ReplaceAll(path, sizeof(path), ".cfg", ".txt", true);
		} else if (strstr(path, ".txt")) {
			UTIL_ReplaceAll(path, sizeof(path), ".txt", ".cfg", true);
		}
	}

	SMCStates states;

	if ((err=textparsers->ParseFile_SMC(path, this, &states)) != SMCError_Okay)
	{
		const char *msg = textparsers->GetSMCErrorString(err);
		if (!msg)
		{
			msg = m_ParseError.c_str();
		}

		logger->LogError("[SM] Fatal error encountered parsing translation file \"%s\"", m_File.c_str());
		logger->LogError("[SM] Error (line %d, column %d): %s", states.line, states.col, msg);
	}

	const char *code;
	for (unsigned int i = 1; i < m_LangCount; i++)
	{
		if (!m_pTranslator->GetLanguageInfo(i, &code, NULL))
		{
			continue;
		}

		g_pSM->BuildPath(Path_SM, 
			path,
			PLATFORM_MAX_PATH,
			"translations/%s/%s",
			code,
			m_File.c_str());

		/* Speculatively load these. */
		if (!libsys->PathExists(path))
		{
			continue;
		}

		if ((err=textparsers->ParseFile_SMC(path, this, &states)) != SMCError_Okay)
		{
			const char *msg = textparsers->GetSMCErrorString(err);
			if (!msg)
			{
				msg = m_ParseError.c_str();
			}

			logger->LogError("[SM] Fatal error encountered parsing translation file \"%s/%s\"", 
				code, 
				m_File.c_str());
			logger->LogError("[SM] Error (line %d, column %d): %s",
				states.line,
				states.col,
				msg);
		}
	}
}

void CPhraseFile::ReadSMC_ParseStart()
{
	m_CurPhrase = -1;
	m_ParseState = PPS_None;
	m_FileLogged = false;
	m_LastPhraseString.clear();
}

SMCResult CPhraseFile::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	bool recognized = false;
	if (m_ParseState == PPS_None)
	{
		if (strcmp(name, "Phrases") == 0)
		{
			m_ParseState = PPS_Phrases;
			recognized = true;
		}
	} 
	else if (m_ParseState == PPS_Phrases) 
	{
		m_ParseState = PPS_InPhrase;
		recognized = true;

		if (!m_PhraseLookup.retrieve(name, &m_CurPhrase))
		{
			phrase_t *pPhrase;

			m_CurPhrase = m_pMemory->CreateMem(sizeof(phrase_t), (void **)&pPhrase);
			m_PhraseLookup.insert(name, m_CurPhrase);

			/* Initialize new phrase */
			trans_t *pTrans;

			pPhrase->fmt_count = 0;
			pPhrase->fmt_list = -1;

			int trans_tbl = m_pMemory->CreateMem(sizeof(trans_t) * m_LangCount, (void **)&pTrans);
			pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);
			pPhrase->trans_tbl = trans_tbl;

			pPhrase->translations = 0;
			pPhrase->fmt_bytes = 0;

			for (unsigned int i=0; i<m_LangCount; i++)
			{
				pTrans[i].stridx = -1;
			}
		}

		m_LastPhraseString.assign(name);
	} 
	else if (m_ParseState == PPS_InPhrase) 
	{
		ParseError("Phrase sections may not have sub-sections");
		return SMCResult_HaltFail;
	}

	if (!recognized)
	{
		ParseWarning("Ignoring invalid section \"%s\" on line %d.", name, states->line);
	}

	return SMCResult_Continue;
}

SMCResult CPhraseFile::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	/* See if we are ignoring a phrase */
	if (m_CurPhrase == -1)
	{
		return SMCResult_Continue;
	}

	phrase_t *pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);

	/* Duplicate format keys get silently ignored. */
	if (key[0] == '#' 
		&& strcmp(key, "#format") == 0
		&& pPhrase->fmt_list == -1)
	{
		if (pPhrase->translations > 0)
		{
			ParseWarning("#format property should come before translations on line %d, ignoring", states->line);
			return SMCResult_Continue;
		}

		/* Do an initial browsing for error checking and what not */
		enum ParseStates
		{
			Parse_None,
			Parse_Index,
			Parse_Format,
		};

		ParseStates state = Parse_None;

		const char *old_value = value;
		const char *last_value = value;
		while (*value != '\0')
		{
			if (state == Parse_None)
			{
				if (*value == '{')
				{
					pPhrase->fmt_count++;
					state = Parse_Index;
				} 
				else if (*value == ',') 
				{
					/* Do nothing */
				} 
				else 
				{
					unsigned int bytes = textparsers->GetUTF8CharBytes(value);
					if (bytes != 1 || !isalpha(*value))
					{
						ParseWarning("Invalid token '%c' in #format property on line %d.", *value, states->line);
					}
				}
			} 
			else if (state == Parse_Index) 
			{
				if (*value == ':')
				{
					state = Parse_Format;
					if (value - last_value >= 15)
					{
						ParseWarning("Too many digits in format index on line %d, phrase will be ignored.", states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
				} 
				else 
				{
					unsigned int bytes = textparsers->GetUTF8CharBytes(value);
					if (bytes != 1 || !isdigit(*value))
					{
						ParseWarning("Token '%c' in #format property on line %d is not a digit, phrase will be ignored.",
									*value,
									states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
				}
			} 
			else if (state == Parse_Format) 
			{
				if (*value == '}')
				{
					state = Parse_None;
					last_value = value + 1;
				}
			}
			value++;
		}

		if (state != Parse_None)
		{
			/* Moose clam cow. */
			ParseWarning("Unterminated format string on line %d, phrase will be ignored.", states->line);
			m_CurPhrase = -1;
			return SMCResult_Continue;
		}
		value = old_value;

		/* Allocate the format table */
		char fmt_buf[16];
		int *fmt_list;
		int tmp = m_pMemory->CreateMem(sizeof(int) * pPhrase->fmt_count, (void **)&fmt_list);

		/* Update the phrase pointer in case it changed */
		pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);
		pPhrase->fmt_list = tmp;

		/* Initialize */
		for (size_t i=0; i<pPhrase->fmt_count; i++)
		{
			fmt_list[i] = -1;
		}

		/* Now we need to read again... this time into the format buffer */
		const char *in_ptr = value;
		const char *idx_ptr = NULL;
		char *out_ptr = NULL;
		unsigned int cur_idx = 0;
		state = Parse_None;
		while (*in_ptr != '\0')
		{
			if (state == Parse_None)
			{
				if (*in_ptr == '{')
				{
					state = Parse_Index;
					idx_ptr = NULL;
				}
			}
			else if (state == Parse_Index) 
			{
				if (*in_ptr == ':')
				{
					/* Check the number! */
					if (!idx_ptr)
					{
						ParseWarning("Format property contains unindexed format string on line %d, phrase will be ignored.", states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
					long idx = strtol(idx_ptr, NULL, 10);
					if (idx < 1 || idx > (long)pPhrase->fmt_count)
					{
						ParseWarning("Format property contains invalid index '%d' on line %d, phrase will be ignored.", idx, states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
					else if (fmt_list[idx - 1] != -1) 
					{
						ParseWarning("Format property contains duplicated index '%d' on line %d, phrase will be ignored.", idx, states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
					cur_idx = (unsigned int)idx;
					state = Parse_Format;
					out_ptr = NULL;
				}
				else if (!idx_ptr) 
				{
					idx_ptr = in_ptr;
				}
			}
			else if (state == Parse_Format) 
			{
				if (*in_ptr == '}')
				{
					if (!out_ptr)
					{
						ParseWarning("Format property contains empty format string on line %d, phrase will be ignored.", states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
					*out_ptr = '\0';
					state = Parse_None;
					/* Now, add this to our table */
					int tmp_idx = m_pStringTab->AddString(fmt_buf);
					/* Update pointers and update necessary variables */
					pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);
					pPhrase->fmt_bytes += strlen(fmt_buf);
					fmt_list = (int *)m_pMemory->GetAddress(pPhrase->fmt_list);
					fmt_list[cur_idx - 1] = tmp_idx;
				} 
				else 
				{
					if (!out_ptr)
					{
						out_ptr = fmt_buf;
						*out_ptr++ = '%';
					}
					/* Check length ... */
					if ((unsigned)(out_ptr - fmt_buf) >= sizeof(fmt_buf) - 1)
					{
						ParseWarning("Format property contains format string that exceeds maximum length on line %d, phrase will be ignored.",
									 states->line);
						m_CurPhrase = -1;
						return SMCResult_Continue;
					}
					*out_ptr++ = *in_ptr;
				}
			}
			in_ptr++;
		}

		/* If we've gotten here, we only accepted unique indexes in a range.
		 * Therefore, the list has to be completely filled.  Double check anyway.
		 */
		for (size_t i=0; i<pPhrase->fmt_count; i++)
		{
			if (fmt_list[i] == -1)
			{
				ParseWarning("Format property contains no string for index %d on line %d, phrase will be ignored.",
							 i + 1,
							 states->line);
				m_CurPhrase = -1;
				return SMCResult_Continue;
			}
		}
	} 
	else 
	{
		unsigned int lang;
		if (!m_pTranslator->GetLanguageByCode(key, &lang))
		{
			/* Ignore if we don't have a language.
			 * :IDEA: issue a one-time alert?
			 */
			return SMCResult_Continue;
		}

		/* See how many bytes we need for this string, then allocate.
		 * NOTE: THIS SHOULD GUARANTEE THAT WE DO NOT NEED TO NEED TO SIZE CHECK 
		 */
		size_t len = strlen(value) + pPhrase->fmt_bytes + 1;
		char *out_buf;
		int out_idx;

		out_idx = m_pMemory->CreateMem(len, (void **)&out_buf);

		/* Update pointer */
		pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);

		int *fmt_order;
		int *fmt_list = (int *)m_pMemory->GetAddress(pPhrase->fmt_list);
		trans_t *pTrans = (trans_t *)m_pMemory->GetAddress(pPhrase->trans_tbl);

		pTrans = &pTrans[lang];
		pTrans->stridx = out_idx;

		bool params[MAX_TRANSLATE_PARAMS];

		/* Build the format order list, if necessary */
		if (fmt_list)
		{
			int tmp = m_pMemory->CreateMem(pPhrase->fmt_count * sizeof(int), (void **)&fmt_order);
			
			/* Update pointers */
			pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);
			pTrans = (trans_t *)m_pMemory->GetAddress(pPhrase->trans_tbl);
			fmt_list = (int *)m_pMemory->GetAddress(pPhrase->fmt_list);
			out_buf = (char *)m_pMemory->GetAddress(out_idx);
			pTrans = &pTrans[lang];

			/* Now it's safe to save the index */
			pTrans->fmt_order = tmp;

			for (unsigned int i=0; i<pPhrase->fmt_count; i++)
			{
				fmt_order[i] = -1;
			}
			memset(&params[0], 0, sizeof(params));
		}

		const char *in_ptr = value;
		char *out_ptr = out_buf;
		unsigned int order_idx = 0;
		while (*in_ptr != '\0')
		{
			if (*in_ptr == '\\')
			{
				switch (*(in_ptr + 1))
				{
				case '\n':
					*out_ptr++ = '\n';
					break;
				case '\t':
					*out_ptr++ = '\t';
					break;
				case '\r':
					*out_ptr++ = '\r';
					break;
				case '{':
					*out_ptr++ = '{';
					break;
				default:
					/* Copy both bytes since we don't know what's going on */
					*out_ptr++ = *in_ptr++;
					*out_ptr++ = *in_ptr;
					break;
				}
				/* Skip past the last byte read */
				in_ptr++;
			} 
			else if (*in_ptr == '{' && fmt_list != NULL) 
			{
				/* Search for parameters if this is a formatted string */
				const char *scrap_in_point = in_ptr;
				const char *digit_start = ++in_ptr;
				unsigned int bytes;
				while (*in_ptr != '\0')
				{
					bytes = textparsers->GetUTF8CharBytes(in_ptr);
					if (bytes != 1)
					{
						goto scrap_point;
					}
					if (*in_ptr == '}')
					{
						/* Did we get an index? */
						if (in_ptr == digit_start)
						{
							goto scrap_point;
						}
						/* Is it valid? */
						long idx = strtol(digit_start, NULL, 10);
						if (idx < 1 || idx > (int)pPhrase->fmt_count)
						{
							goto scrap_point;
						}
						if (params[idx-1])
						{
							goto scrap_point;
						}

						/* We're safe to insert the string.  First mark order. */
						fmt_order[order_idx++] = (int)idx - 1;
						/* Now concatenate */
						out_ptr += sprintf(out_ptr, "%s", m_pStringTab->GetString(fmt_list[idx-1]));
						/* Mark as used */
						params[idx-1] = true;

						goto cont_loop;
					}
					in_ptr++;
				}
scrap_point:
				/* Pretend none of this ever happened.  Move along now! */
				in_ptr = scrap_in_point;
				*out_ptr++ = *in_ptr;
			} else {
				*out_ptr++ = *in_ptr;
			}
cont_loop:
			in_ptr++;
		}
		*out_ptr = '\0';
		pPhrase->translations++;
	}

	return SMCResult_Continue;
}

SMCResult CPhraseFile::ReadSMC_LeavingSection(const SMCStates *states)
{
	if (m_ParseState == PPS_InPhrase)
	{
		if (m_CurPhrase == -1 && m_LastPhraseString.size())
			m_PhraseLookup.remove(m_LastPhraseString.c_str());
		m_CurPhrase = -1;
		m_ParseState = PPS_Phrases;
		m_LastPhraseString.assign("");
	} 
	else if (m_ParseState == PPS_Phrases) 
	{
		m_ParseState = PPS_None;
	}

	return SMCResult_Continue;
}

void CPhraseFile::ReadSMC_ParseEnd(bool halted, bool failed)
{
	/* Check to see if we have any dangling phrases that weren't completed, and scrap them */
	if ((halted || failed) && m_LastPhraseString.size())
		m_PhraseLookup.remove(m_LastPhraseString.c_str());
}

TransError CPhraseFile::GetTranslation(const char *szPhrase, unsigned int lang_id, Translation *pTrans)
{
	if (lang_id >= m_LangCount)
	{
		return Trans_BadLanguage;
	}

	int address;
	if (!m_PhraseLookup.retrieve(szPhrase, &address))
		return Trans_BadPhrase;

	phrase_t *pPhrase = (phrase_t *)m_pMemory->GetAddress(address);
	trans_t *trans = (trans_t *)m_pMemory->GetAddress(pPhrase->trans_tbl);

	trans = &trans[lang_id];

	if (trans->stridx == -1)
	{
		return Trans_BadPhraseLanguage;
	}

	pTrans->fmt_count = pPhrase->fmt_count;
	pTrans->fmt_order = pTrans->fmt_count > 0 ? (int *)m_pMemory->GetAddress(trans->fmt_order) : NULL;
	pTrans->szPhrase = m_pStringTab->GetString(trans->stridx);

	return Trans_Okay;
}

bool CPhraseFile::TranslationPhraseExists(const char *phrase)
{
	int address;
	return m_PhraseLookup.retrieve(phrase, &address);
}

const char *CPhraseFile::GetFilename()
{
	return m_File.c_str();
}

/**************************
 ** MAIN TRANSLATOR CODE **
 **************************/

Translator::Translator() : m_ServerLang(SOURCEMOD_LANGUAGE_ENGLISH)
{
	m_pStringTab = new BaseStringTable(2048);
	strncopy(m_InitialLang, "en", sizeof(m_InitialLang));
}

Translator::~Translator()
{
	for (size_t i=0; i<m_Files.size(); i++)
	{
		delete m_Files[i];
	}

	for (size_t i=0; i<m_Languages.size(); i++)
	{
		delete m_Languages[i];
	}

	delete m_pStringTab;
}

ConfigResult Translator::OnSourceModConfigChanged(const char *key, 
									  const char *value, 
									  ConfigSource source, 
									  char *error, 
									  size_t maxlength)
{
	if (strcasecmp(key, "ServerLang") == 0)
	{
		if (source == ConfigSource_Console)
		{
			unsigned int index;
			if (!GetLanguageByCode(value, &index))
			{
				ke::SafeSprintf(error, maxlength, "Language code \"%s\" is not registered", value);
				return ConfigResult_Reject;
			}

			m_ServerLang = index;
		} else {
			strncopy(m_InitialLang, value, sizeof(m_InitialLang));
		}

		return ConfigResult_Accept;
	}

	return ConfigResult_Ignore;
}

void Translator::OnSourceModLevelChange(const char *mapName)
{
	RebuildLanguageDatabase();
}

void Translator::OnSourceModAllInitialized()
{
	AddLanguage("en", "English");

	const char* lang = bridge->GetCoreConfigValue("ServerLang");
	if (lang)
	{
		strncpy(m_InitialLang, lang, sizeof(m_InitialLang));
	}

	g_pCorePhrases = CreatePhraseCollection();
	g_pCorePhrases->AddPhraseFile("core.phrases");

	sharesys->AddInterface(NULL, this);

	auto sm_reload_translations = [this] (int client, const ICommandArgs *args) -> bool {
		RebuildLanguageDatabase();
		return true;
	};
	bridge->DefineCommand("sm_reload_translations", "Reparses all loaded translation files",
	                      sm_reload_translations);
}

void Translator::OnSourceModShutdown()
{
	g_pCorePhrases->Destroy();
}

bool Translator::GetLanguageByCode(const char *code, unsigned int *index)
{
	return m_LCodeLookup.retrieve(code, index);
}

bool Translator::GetLanguageByName(const char *name, unsigned int *index)
{
	char lower[256];
	size_t len = strlen(name);
	if (len > 255) len = 255;
	for (size_t i = 0; i < len; i++)
	{
		if (name[i] >= 'A' && name[i] <= 'Z')
			lower[i] = tolower(name[i]);
		else
			lower[i] = name[i];
	}
	lower[len] = '\0';

	return m_LAliases.retrieve(lower, index);
}

unsigned int Translator::GetLanguageCount()
{
	return (unsigned int)m_Languages.size();
}

BaseStringTable *Translator::GetStringTable()
{
	return m_pStringTab;
}

unsigned int Translator::FindOrAddPhraseFile(const char *phrase_file)
{
	for (size_t i=0; i<m_Files.size(); i++)
	{
		if (strcmp(m_Files[i]->GetFilename(), phrase_file) == 0)
		{
			return (unsigned int)i;
		}
	}

	CPhraseFile *pFile = new CPhraseFile(this, phrase_file);
	unsigned int idx = (unsigned int)m_Files.size();
	
	m_Files.push_back(pFile);

	pFile->ReparseFile();

	return idx;
}

void Translator::RebuildLanguageDatabase()
{
	/* Erase everything we have */
	m_LCodeLookup.clear();
	m_LAliases.clear();
	m_pStringTab->Reset();

	for (size_t i=0; i<m_Languages.size(); i++)
	{
		delete m_Languages[i];
	}
	m_Languages.clear();

	/* Start anew */
	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_SM, path, sizeof(path), "configs/languages.cfg");

	SMCError err;
	SMCStates states;
	if ((err=textparsers->ParseFile_SMC(path, this, &states)) != SMCError_Okay)
	{
		const char *str_err = textparsers->GetSMCErrorString(err);
		if (!str_err)
		{
			str_err = m_CustomError.c_str();
		}

		logger->LogError("[SM] Failed to parse language header file: \"%s\"", path);
		logger->LogError("[SM] Parse error (line %d, column %d): %s", states.line, states.col, str_err);
	}

	if (!m_LCodeLookup.retrieve(m_InitialLang, &m_ServerLang))
	{
		logger->LogError("Server language was set to bad language \"%s\" -- reverting to English", m_InitialLang);

		strncopy(m_InitialLang, "en", sizeof(m_InitialLang));
		m_ServerLang = SOURCEMOD_LANGUAGE_ENGLISH;
	}

	if (!m_Languages.size())
	{
		logger->LogError("[SM] Fatal error, no languages found! Translation will not work.");
	}

	for (size_t i=0; i<m_Files.size(); i++)
	{
		m_Files[i]->ReparseFile();
	}
}

void Translator::ReadSMC_ParseStart()
{
	m_InLanguageSection = false;
	m_CustomError.clear();
}

SMCResult Translator::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	if (!m_InLanguageSection)
	{
		if (strcmp(name, "Languages") == 0)
		{
			m_InLanguageSection = true;
		}
	}

	if (!m_InLanguageSection)
	{
		logger->LogError("[SM] Warning: Unrecognized section \"%s\" in languages.cfg", name);
	}

	return SMCResult_Continue;
}

SMCResult Translator::ReadSMC_LeavingSection(const SMCStates *states)
{
	m_InLanguageSection = false;

	return SMCResult_Continue;
}

SMCResult Translator::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	size_t len = strlen(key);

	if (len >= sizeof(((Language *)0)->m_code2))
	{
		logger->LogError("[SM] Warning encountered parsing languages.cfg file.");
		logger->LogError("[SM] Invalid language code \"%s\" is too long.", key);
	}

	AddLanguage(key, value);

	return SMCResult_Continue;
}

bool Translator::AddLanguage(const char *langcode, const char *description)
{
	char lower[256];
	size_t len = strlen(description);
	if (len > 255) len = 255;
	for (size_t i = 0; i < len; i++)
	{
		if (description[i] >= 'A' && description[i] <= 'Z')
			lower[i] = tolower(description[i]);
		else
			lower[i] = description[i];
	}
	lower[len] = '\0';

	if (m_LAliases.contains(lower))
		return false;

	unsigned int idx;
	if (!m_LCodeLookup.retrieve(langcode, &idx))
	{
		Language *pLanguage = new Language;
		idx = m_Languages.size();

		ke::SafeStrcpy(pLanguage->m_code2, sizeof(pLanguage->m_code2), langcode);
		pLanguage->m_CanonicalName = m_pStringTab->AddString(lower);

		m_LCodeLookup.insert(langcode, idx);

		m_Languages.push_back(pLanguage);
	}
	
	m_LAliases.insert(lower, idx);

	return true;
}

CPhraseFile *Translator::GetFileByIndex(unsigned int index)
{
	if (index >= m_Files.size())
	{
		return NULL;
	}

	return m_Files[index];
}

unsigned int Translator::GetServerLanguage()
{
	return m_ServerLang;
}

unsigned int Translator::GetClientLanguage(int client)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(client);
	return pPlayer->GetLanguageId();
}

bool Translator::GetLanguageInfo(unsigned int number, const char **code, const char **name)
{
	if (number >= GetLanguageCount())
	{
		return false;
	}

	Language *l = m_Languages[number];
	if (code)
	{
		*code = l->m_code2;
	}
	if (name)
	{
		*name = m_pStringTab->GetString(l->m_CanonicalName);
	}

	return true;
}

const char *Translator::GetInterfaceName()
{
	return SMINTERFACE_TRANSLATOR_NAME;
}

unsigned int Translator::GetInterfaceVersion()
{
	return SMINTERFACE_TRANSLATOR_VERSION;
}

CPhraseCollection *Translator::CreatePhraseCollection()
{
	return new CPhraseCollection();
}

int Translator::SetGlobalTarget(int index)
{
	return g_pSM->SetGlobalTarget(index);
}

int Translator::GetGlobalTarget() const
{
	return g_pSM->GetGlobalTarget();
}

bool CoreTranslate(char *buffer,  size_t maxlength, const char *format, unsigned int numparams,
                   size_t *pOutLength, ...)
{
	va_list ap;
	unsigned int i;
	const char *fail_phrase;
	void *params[MAX_TRANSLATE_PARAMS];

	if (numparams > MAX_TRANSLATE_PARAMS)
	{
		assert(false);
		return false;
	}

	va_start(ap, pOutLength);
	for (i = 0; i < numparams; i++)
	{
		params[i] = va_arg(ap, void *);
	}
	va_end(ap);

	if (!g_pCorePhrases->FormatString(buffer,
		maxlength, 
		format, 
		params,
		numparams,
		pOutLength,
		&fail_phrase))
	{
		if (fail_phrase != NULL)
		{
			logger->LogError("[SM] Could not find core phrase: %s", fail_phrase);
		}
		else
		{
			logger->LogError("[SM] Unknown fatal error while translating a core phrase.");
		}

		return false;
	}

	return true;
}

bool Translator::FormatString(char *buffer,
							  size_t maxlength,
							  const char *format,
							  IPhraseCollection *pPhrases,
							  void **params,
							  unsigned int numparams,
							  size_t *pOutLength,
							  const char **pFailPhrase)
{
	unsigned int arg;

	arg = 0;
	if (!gnprintf(buffer, maxlength, format, pPhrases, params, numparams, arg, pOutLength, pFailPhrase))
	{
		return false;
	}

	if (arg != numparams)
	{
		if (pFailPhrase != NULL)
		{
			*pFailPhrase = NULL;
		}
		return false;
	}

	return true;
}
