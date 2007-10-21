/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_TRANSLATOR_H_
#define _INCLUDE_SOURCEMOD_TRANSLATOR_H_

#include "sm_trie.h"
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_globals.h"
#include "sm_memtable.h"
#include "ITextParsers.h"

#define MAX_TRANSLATE_PARAMS		32
#define CORELANG_ENGLISH			0

/* :TODO: write a templatized version of tries? */

using namespace SourceHook;
class Translator;

enum PhraseParseState
{
	PPS_None = 0,
	PPS_Phrases,
	PPS_InPhrase,
};

struct Language
{
	char m_code2[3];
	int m_FullName;
};

struct Translation
{
	const char *szPhrase;		/**< Translated phrase. */
	unsigned int fmt_count;		/**< Number of format parameters. */
	int *fmt_order;				/**< Format phrase order. */
};

#define LANGUAGE_ENGLISH			0

enum TransError
{
	Trans_Okay = 0,
	Trans_BadLanguage = 1,
	Trans_BadPhrase = 2,
	Trans_BadPhraseLanguage = 3,
	Trans_BadPhraseFile = 4,
};

class CPhraseFile : public ITextListener_SMC
{
public:
	CPhraseFile(Translator *pTranslator, const char *file);
	~CPhraseFile();
public:
	void ReparseFile();
	const char *GetFilename();
	TransError GetTranslation(const char *szPhrase, unsigned int lang_id, Translation *pTrans);
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	void ReadSMC_ParseEnd(bool halted, bool failed);
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
	SMCParseResult ReadSMC_RawLine(const char *line, unsigned int curline);
private:
	void ParseError(const char *message, ...);
	void ParseWarning(const char *message, ...);
private:
	Trie *m_pPhraseLookup;
	String m_File;
	Translator *m_pTranslator;
	PhraseParseState m_ParseState;
	int m_CurPhrase;
	BaseMemTable *m_pMemory;
	BaseStringTable *m_pStringTab;
	unsigned int m_LangCount;
	String m_ParseError;
	String m_LastPhraseString;
	unsigned int m_CurLine;
	bool m_FileLogged;
};

class Translator : 
	public ITextListener_SMC,
	public SMGlobalClass
{
public:
	Translator();
	~Translator();
public: // SMGlobalClass
	ConfigResult OnSourceModConfigChanged(const char *key, 
		const char *value, 
		ConfigSource source, 
		char *error, 
		size_t maxlength);
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
public: // ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
public:
	void RebuildLanguageDatabase(const char *lang_header_file);
	unsigned int FindOrAddPhraseFile(const char *phrase_file);
	BaseStringTable *GetStringTable();
	unsigned int GetLanguageCount();
	bool GetLanguageInfo(unsigned int number, const char **code, const char **name);
	bool GetLanguageByCode(const char *code, unsigned int *index);
	bool GetLanguageByName(const char *name, unsigned int *index);
	size_t Translate(char *buffer, size_t maxlength, void **params, const Translation *pTrans);
	CPhraseFile *GetFileByIndex(unsigned int index);
	TransError CoreTrans(int client, 
						 char *buffer, 
						 size_t maxlength, 
						 const char *phrase, 
						 void **params, 
						 size_t *outlen=NULL);
	unsigned int GetServerLanguage();
	unsigned int GetClientLanguage(int client);
private:
	bool AddLanguage(const char *langcode, const char *description);
private:
	CVector<Language *> m_Languages;
	CVector<CPhraseFile *> m_Files;
	BaseStringTable *m_pStringTab;
	Trie *m_pLCodeLookup;
	bool m_InLanguageSection;
	String m_CustomError;
	unsigned int m_ServerLang;
	char m_InitialLang[3];
};

extern CPhraseFile *g_pCorePhrases;
extern unsigned int g_pCorePhraseID;
extern Translator g_Translator;

#endif //_INCLUDE_SOURCEMOD_TRANSLATOR_H_
