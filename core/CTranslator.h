#ifndef _INCLUDE_SOURCEMOD_TRANSLATOR_H_
#define _INCLUDE_SOURCEMOD_TRANSLATOR_H_

#include "sm_trie.h"
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_globals.h"
#include "sm_memtable.h"
#include "ITextParsers.h"

#define MAX_TRANSLATE_PARAMS		32

/* :TODO: write a templatized version of tries? */

using namespace SourceHook;
class CTranslator;

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
	const char *szPhrase;
	unsigned int lang_id;
	unsigned int file_id;
	void **params;
};

class CPhraseFile : public ITextListener_SMC
{
public:
	CPhraseFile(CTranslator *pTranslator, const char *file);
	~CPhraseFile();
public:
	void ReparseFile();
	const char *GetFilename();
	const char *GetTranslation(const Translation *pTrans, int **fmt_order, unsigned int *fmt_count);
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
	CTranslator *m_pTranslator;
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

class CTranslator : public ITextListener_SMC
{
public:
	CTranslator();
	~CTranslator();
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
public:
	void RebuildLanguageDatabase(const char *lang_header_file);
	unsigned int FindOrAddPhraseFile(const char *phrase_file);
	BaseStringTable *GetStringTable();
	unsigned int GetLanguageCount();
	bool GetLanguageByCode(const char *code, unsigned int *index);
	size_t Translate(char *buffer, size_t maxlength, const Translation *pTrans);
private:
	CVector<Language *> m_Languages;
	CVector<CPhraseFile *> m_Files;
	BaseStringTable *m_pStringTab;
	Trie *m_pLCodeLookup;
	bool m_InLanguageSection;
	String m_CustomError;
};

extern CTranslator g_Translator;

#endif //_INCLUDE_SOURCEMOD_TRANSLATOR_H_
