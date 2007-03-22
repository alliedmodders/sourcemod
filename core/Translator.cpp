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

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include "Translator.h"
#include "Logger.h"
#include "TextParsers.h"
#include "LibrarySys.h"
#include "sm_stringutil.h"
#include "sourcemod.h"

Translator g_Translator;
CPhraseFile *g_pCorePhrases = NULL;

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
	m_pPhraseLookup = NULL;
}

CPhraseFile::~CPhraseFile()
{
	if (m_pPhraseLookup)
	{
		sm_trie_destroy(m_pPhraseLookup);
	}
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
		g_Logger.LogError("[SM] Warning(s) encountered in translation file \"%s\"", m_File.c_str());
		m_FileLogged = true;
	}

	g_Logger.LogError("[SM] %s", message);
}

void CPhraseFile::ReparseFile()
{
	if (m_pPhraseLookup)
	{
		sm_trie_destroy(m_pPhraseLookup);
	}
	m_pPhraseLookup = sm_trie_create();

	m_LangCount = m_pTranslator->GetLanguageCount();

	if (!m_LangCount)
	{
		return;
	}

	SMCParseError err;
	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, path, PLATFORM_MAX_PATH, "translations/%s", m_File.c_str());
	unsigned int line=0, col=0;

	if ((err=g_TextParser.ParseFile_SMC(path, this, &line, &col)) != SMCParse_Okay)
	{
		const char *msg = g_TextParser.GetSMCErrorString(err);
		if (!msg)
		{
			msg = m_ParseError.c_str();
		}

		g_Logger.LogError("[SM] Fatal error encountered parsing translation file \"%s\"", m_File.c_str());
		g_Logger.LogError("[SM] Error (line %d, column %d): %s", line, col, msg);
	}
}

void CPhraseFile::ReadSMC_ParseStart()
{
	m_CurPhrase = -1;
	m_ParseState = PPS_None;
	m_CurLine = 0;
	m_FileLogged = false;
	m_LastPhraseString.clear();
}

SMCParseResult CPhraseFile::ReadSMC_RawLine(const char *line, unsigned int curline)
{
	m_CurLine = curline;

	return SMCParse_Continue;
}

SMCParseResult CPhraseFile::ReadSMC_NewSection(const char *name, bool opt_quotes)
{
	bool recognized = false;
	if (m_ParseState == PPS_None)
	{
		if (strcmp(name, "Phrases") == 0)
		{
			m_ParseState = PPS_Phrases;
			recognized = true;
		}
	} else if (m_ParseState == PPS_Phrases) {
		m_ParseState = PPS_InPhrase;
		recognized = true;

		phrase_t *pPhrase;
		m_CurPhrase = m_pMemory->CreateMem(sizeof(phrase_t), (void **)&pPhrase);

		/* Create the reverse lookup */
		if (!sm_trie_insert(m_pPhraseLookup, name, reinterpret_cast<void *>(m_CurPhrase)))
		{
			ParseWarning("Skipping duplicate phrase \"%s\" on line %d.", name, m_CurLine);
			/* :IDEA: prevent memory waste by seeking backwards in memtable? */
			m_CurPhrase = -1;
		} else {
			/* Initialize new phrase */
			trans_t *pTrans;
	
			pPhrase->fmt_count = 0;
			pPhrase->fmt_list = -1;
			pPhrase->trans_tbl = m_pMemory->CreateMem(sizeof(trans_t) * m_LangCount, (void **)&pTrans);
			pPhrase->translations = 0;
			pPhrase->fmt_bytes = 0;
	
			for (unsigned int i=0; i<m_LangCount; i++)
			{
				pTrans[i].stridx = -1;
			}
			m_LastPhraseString.assign(name);
		}
	} else if (m_ParseState == PPS_InPhrase) {
		ParseError("Phrase sections may not have sub-sections");
		return SMCParse_HaltFail;
	}

	if (!recognized)
	{
		ParseWarning("Ignoring invalid section \"%s\" on line %d.", name, m_CurLine);
	}

	return SMCParse_Continue;
}

SMCParseResult CPhraseFile::ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
{
	/* See if we are ignoring a phrase */
	if (!m_CurPhrase)
	{
		return SMCParse_Continue;
	}

	phrase_t *pPhrase = (phrase_t *)m_pMemory->GetAddress(m_CurPhrase);
	
	if (key[0] == '#' && strcmp(key, "#format") == 0)
	{
		if (pPhrase->fmt_list != -1)
		{
			ParseWarning("Ignoring duplicated #format property on line %d", m_CurLine);
			return SMCParse_Continue;
		}

		if (pPhrase->translations > 0)
		{
			ParseWarning("#format property should come before translations on line %d, ignoring", m_CurLine);
			return SMCParse_Continue;
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
		while (*value != '\0')
		{
			if (state == Parse_None)
			{
				if (*value == '{')
				{
					pPhrase->fmt_count++;
					state = Parse_Index;
				} else if (*value == ',') {
					/* Do nothing */
				} else {
					unsigned int bytes = g_TextParser.GetUTF8CharBytes(value);
					if (bytes != 1 || !isalpha(*value))
					{
						ParseWarning("Invalid token '%c' in #format property on line %d.", *value, m_CurLine);
					}
				}
			} else if (state == Parse_Index) {
				if (*value == ':')
				{
					state = Parse_Format;
					if (value - old_value >= 15)
					{
						ParseWarning("Too many digits in format index on line %d, phrase will be ignored.", m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					}
				} else {
					unsigned int bytes = g_TextParser.GetUTF8CharBytes(value);
					if (bytes != 1 || !isdigit(*value))
					{
						ParseWarning("Token '%c' in #format property on line %d is not a digit, phrase will be ignored.",
									*value,
									m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					}
				}
			} else if (state == Parse_Format) {
				if (*value == '}')
				{
					state = Parse_None;
				}
			}
			value++;
		}

		if (state != Parse_None)
		{
			/* Moose clam cow. */
			ParseWarning("Unterminated format string on line %d, phrase will be ignored.", m_CurLine);
			m_CurPhrase = -1;
			return SMCParse_Continue;
		}
		value = old_value;

		/* Allocate the format table */
		char fmt_buf[16];
		int *fmt_list;
		pPhrase->fmt_list = m_pMemory->CreateMem(sizeof(int) * pPhrase->fmt_count, (void **)&fmt_list);

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
			} else if (state == Parse_Index) {
				if (*in_ptr == ':')
				{
					/* Check the number! */
					if (!idx_ptr)
					{
						ParseWarning("Format property contains unindexed format string on line %d, phrase will be ignored.", m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					}
					long idx = strtol(idx_ptr, NULL, 10);
					if (idx < 1 || idx > (long)pPhrase->fmt_count)
					{
						ParseWarning("Format property contains invalid index '%d' on line %d, phrase will be ignored.", idx, m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					} else if (fmt_list[idx - 1] != -1) {
						ParseWarning("Format property contains duplicated index '%d' on line %d, phrase will be ignored.", idx, m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					}
					cur_idx = (unsigned int)idx;
					state = Parse_Format;
					out_ptr = NULL;
				} else if (!idx_ptr) {
					idx_ptr = in_ptr;
				}
			} else if (state == Parse_Format) {
				if (*in_ptr == '}')
				{
					if (!out_ptr)
					{
						ParseWarning("Format property contains empty format string on line %d, phrase will be ignored.", m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
					}
					*out_ptr = '\0';
					state = Parse_None;
					/* Now, add this to our table */
					fmt_list[cur_idx - 1] = m_pStringTab->AddString(fmt_buf);
					pPhrase->fmt_bytes += strlen(fmt_buf);
				} else {
					if (!out_ptr)
					{
						out_ptr = fmt_buf;
						*out_ptr++ = '%';
					}
					/* Check length ... */
					if ((unsigned)(out_ptr - fmt_buf) >= sizeof(fmt_buf) - 1)
					{
						ParseWarning("Format property contains format string that exceeds maximum length on line %d, phrase will be ignored.",
									 m_CurLine);
						m_CurPhrase = -1;
						return SMCParse_Continue;
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
							 m_CurLine);
				m_CurPhrase = -1;
				return SMCParse_Continue;
			}
		}
	} else {
		size_t len = strlen(key);
		if (len != 2)
		{
			ParseWarning("Ignoring translation to invalid language \"%s\" on line %d.", key, m_CurLine);
			return SMCParse_Continue;
		}

		unsigned int lang;
		if (!m_pTranslator->GetLanguageByCode(key, &lang))
		{
			/* Ignore if we don't have a language.
			 * :IDEA: issue a one-time alert?
			 */
			return SMCParse_Continue;
		}

		/* See how many bytes we need for this string, then allocate.
		 * NOTE: THIS SHOULD GUARANTEE THAT WE DO NOT NEED TO NEED TO SIZE CHECK 
		 */
		len = strlen(value) + pPhrase->fmt_bytes + 1;
		char *out_buf;
		int out_idx;

		out_idx = m_pMemory->CreateMem(len, (void **)&out_buf);

		int *fmt_order;
		int *fmt_list = (int *)m_pMemory->GetAddress(pPhrase->fmt_list);
		trans_t *pTrans = (trans_t *)m_pMemory->GetAddress(pPhrase->trans_tbl);

		pTrans = &pTrans[lang];
		pTrans->stridx = out_idx;

		bool params[MAX_TRANSLATE_PARAMS];

		/* Build the format order list, if necessary */
		if (fmt_list)
		{
			pTrans->fmt_order = m_pMemory->CreateMem(pPhrase->fmt_count * sizeof(int), (void **)&fmt_order);

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
			} else if (*in_ptr == '{' && fmt_list != NULL) {
				/* Search for parameters if this is a formatted string */
				const char *scrap_in_point = in_ptr;
				const char *digit_start = ++in_ptr;
				unsigned int bytes;
				while (*in_ptr != '\0')
				{
					bytes = g_TextParser.GetUTF8CharBytes(in_ptr);
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

	return SMCParse_Continue;
}

SMCParseResult CPhraseFile::ReadSMC_LeavingSection()
{
	if (m_ParseState == PPS_InPhrase)
	{
		if (m_CurPhrase == -1 && m_LastPhraseString.size())
		{
			sm_trie_delete(m_pPhraseLookup, m_LastPhraseString.c_str());
		}
		m_CurPhrase = -1;
		m_ParseState = PPS_Phrases;
		m_LastPhraseString.assign("");
	} else if (m_ParseState == PPS_Phrases) {
		m_ParseState = PPS_None;
	}

	return SMCParse_Continue;
}

void CPhraseFile::ReadSMC_ParseEnd(bool halted, bool failed)
{
	/* Check to see if we have any dangling phrases that weren't completed, and scrap them */
	if ((halted || failed) && m_LastPhraseString.size())
	{
		sm_trie_delete(m_pPhraseLookup, m_LastPhraseString.c_str());
	}
}

TransError CPhraseFile::GetTranslation(const char *szPhrase, unsigned int lang_id, Translation *pTrans)
{
	if (lang_id >= m_LangCount)
	{
		return Trans_BadLanguage;
	}

	void *object;
	if (!sm_trie_retrieve(m_pPhraseLookup, szPhrase, &object))
	{
		return Trans_BadPhrase;
	}

	phrase_t *pPhrase = (phrase_t *)m_pMemory->GetAddress(reinterpret_cast<int>(object));
	trans_t *trans = (trans_t *)m_pMemory->GetAddress(pPhrase->trans_tbl);

	trans = &trans[lang_id];

	if (trans->stridx == -1)
	{
		return Trans_BadPhraseLanguage;
	}

	pTrans->fmt_order = (int *)m_pMemory->GetAddress(trans->fmt_order);
	pTrans->fmt_count = pPhrase->fmt_count;

	pTrans->szPhrase = m_pStringTab->GetString(trans->stridx);

	return Trans_Okay;
}

const char *CPhraseFile::GetFilename()
{
	return m_File.c_str();
}

/**************************
 ** MAIN TRANSLATOR CODE **
 **************************/

Translator::Translator()
{
	m_pStringTab = new BaseStringTable(2048);
	m_pLCodeLookup = sm_trie_create();
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

	sm_trie_destroy(m_pLCodeLookup);

	delete m_pStringTab;
}

void Translator::OnSourceModAllInitialized()
{
	AddLanguage("en", "English");
	unsigned int id = FindOrAddPhraseFile("core.cfg");
	g_pCorePhrases = GetFileByIndex(id);
}

bool Translator::GetLanguageByCode(const char *code, unsigned int *index)
{
	void *_index;

	if (!sm_trie_retrieve(m_pLCodeLookup, code, &_index))
	{
		return false;
	}

	if (index)
	{
		*index = reinterpret_cast<unsigned int>(_index);
	}

	return true;
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

void Translator::RebuildLanguageDatabase(const char *lang_header_file)
{
	/* Erase everything we have */
	sm_trie_destroy(m_pLCodeLookup);
	m_pLCodeLookup = sm_trie_create();
	m_pStringTab->Reset();

	for (size_t i=0; i<m_Languages.size(); i++)
	{
		delete m_Languages[i];
	}
	m_Languages.clear();

	/* Start anew */
	SMCParseError err;
	unsigned int line=0, col=0;
	if ((err=g_TextParser.ParseFile_SMC(lang_header_file, this, &line, &col)) != SMCParse_Okay)
	{
		const char *str_err = g_TextParser.GetSMCErrorString(err);
		if (!str_err)
		{
			str_err = m_CustomError.c_str();
		}

		g_Logger.LogError("[SM] Failed to parse language header file: \"%s\"", lang_header_file);
		g_Logger.LogError("[SM] Parse error (line %d, column %d): %s", line, col, str_err);
	}

	if (!m_Languages.size())
	{
		g_Logger.LogError("[SM] Fatal error, no languages found! Translation will not work.");
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

SMCParseResult Translator::ReadSMC_NewSection(const char *name, bool opt_quotes)
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
		g_Logger.LogError("[SM] Warning: Unrecognized section \"%s\" in languages.cfg", name);
	}

	return SMCParse_Continue;
}

SMCParseResult Translator::ReadSMC_LeavingSection()
{
	m_InLanguageSection = false;

	return SMCParse_Continue;
}

SMCParseResult Translator::ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
{
	size_t len = strlen(key);

	if (len != 2)
	{
		g_Logger.LogError("[SM] Warning encountered parsing languages.cfg file.");
		g_Logger.LogError("[SM] Invalid language code \"%s\" is being ignored.", key);
	}

	AddLanguage(key, value);

	return SMCParse_Continue;
}

bool Translator::AddLanguage(const char *langcode, const char *description)
{
	if (sm_trie_retrieve(m_pLCodeLookup, langcode, NULL))
	{
		return false;
	}

	Language *pLanguage = new Language;
	unsigned int idx = m_Languages.size();

	pLanguage->m_code2[0] = langcode[0];
	pLanguage->m_code2[1] = langcode[1];
	pLanguage->m_code2[2] = langcode[2];
	pLanguage->m_FullName = m_pStringTab->AddString(description);

	sm_trie_insert(m_pLCodeLookup, langcode, reinterpret_cast<void *>(idx));

	m_Languages.push_back(pLanguage);

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

size_t Translator::Translate(char *buffer, size_t maxlength, void **params, const Translation *pTrans)
{
	void *new_params[MAX_TRANSLATE_PARAMS];

	/* Rewrite the parameter order */
	for (unsigned int i=0; i<pTrans->fmt_count; i++)
	{
		new_params[i] = params[pTrans->fmt_order[i]];
	}

	return gnprintf(buffer, maxlength, pTrans->szPhrase, new_params);
}

TransError Translator::CoreTrans(int client, 
								  char *buffer, 
								  size_t maxlength, 
								  const char *phrase, 
								  void **params, 
								  size_t *outlen)
{
	/* :TODO: do language stuff here */

	if (!g_pCorePhrases)
	{
		return Trans_BadPhraseFile;
	}

	Translation trans;
	TransError err;
	if ((err=g_pCorePhrases->GetTranslation(phrase, 0, &trans)) != Trans_Okay)
	{
		return err;
	}

	size_t len = Translate(buffer, maxlength, params, &trans);

	if (outlen)
	{
		*outlen = len;
	}

	return Trans_Okay;
}
