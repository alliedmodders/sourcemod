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

#ifndef _INCLUDE_SOURCEMOD_TRANSLATOR_H_
#define _INCLUDE_SOURCEMOD_TRANSLATOR_H_

#include "common_logic.h"
#include <sm_stringhashmap.h>
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_memtable.h"
#include "ITextParsers.h"
#include <ITranslator.h>
#include "PhraseCollection.h"

/* :TODO: write a templatized version of tries? */

using namespace SourceMod;
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
	char m_code2[32];
	int m_CanonicalName;
};

class CPhraseFile : 
	public ITextListener_SMC,
	public IPhraseFile
{
public:
	CPhraseFile(Translator *pTranslator, const char *file);
	~CPhraseFile();
public:
	void ReparseFile();
	const char *GetFilename();
	TransError GetTranslation(const char *szPhrase, unsigned int lang_id, Translation *pTrans);
	bool TranslationPhraseExists(const char *phrase);
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	void ReadSMC_ParseEnd(bool halted, bool failed);
private:
	void ParseError(const char *message, ...);
	void ParseWarning(const char *message, ...);
private:
	StringHashMap<int> m_PhraseLookup;
	String m_File;
	Translator *m_pTranslator;
	PhraseParseState m_ParseState;
	int m_CurPhrase;
	BaseMemTable *m_pMemory;
	BaseStringTable *m_pStringTab;
	unsigned int m_LangCount;
	String m_ParseError;
	String m_LastPhraseString;
	bool m_FileLogged;
};

class Translator : 
	public ITextListener_SMC,
	public SMGlobalClass,
	public ITranslator
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
	void OnSourceModShutdown();
public: // ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
public:
	unsigned int FindOrAddPhraseFile(const char *phrase_file);
	BaseStringTable *GetStringTable();
	unsigned int GetLanguageCount();
	bool GetLanguageByCode(const char *code, unsigned int *index);
	bool GetLanguageByName(const char *name, unsigned int *index);
	CPhraseFile *GetFileByIndex(unsigned int index);
public: //ITranslator
	unsigned int GetServerLanguage();
	unsigned int GetClientLanguage(int client);
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
	CPhraseCollection *CreatePhraseCollection();
	int SetGlobalTarget(int index);
	int GetGlobalTarget() const;
	size_t FormatString(
		char *buffer, 
		size_t maxlength, 
		SourcePawn::IPluginContext *pContext,
		const cell_t *params,
		unsigned int param);
	bool FormatString(
		char *buffer,
		size_t maxlength,
		const char *format,
		IPhraseCollection *pPhrases,
		void **params,
		unsigned int numparams,
		size_t *pOutLength,
		const char **pFailPhrase);
	bool GetLanguageInfo(unsigned int number, const char **code, const char **name);
	void RebuildLanguageDatabase();
private:
	bool AddLanguage(const char *langcode, const char *description);
private:
	CVector<Language *> m_Languages;
	CVector<CPhraseFile *> m_Files;
	BaseStringTable *m_pStringTab;
	StringHashMap<unsigned int> m_LCodeLookup;
	StringHashMap<unsigned int> m_LAliases;
	bool m_InLanguageSection;
	String m_CustomError;
	unsigned int m_ServerLang;
	char m_InitialLang[4];
};

/* Nice little wrapper to handle error logging and whatnot */
bool CoreTranslate(char *buffer, 
				   size_t maxlength,
				   const char *format,
				   unsigned int numparams,
				   size_t *pOutLength,
				   ...);

extern IPhraseCollection *g_pCorePhrases;
extern unsigned int g_pCorePhraseID;
extern Translator g_Translator;

#endif //_INCLUDE_SOURCEMOD_TRANSLATOR_H_

