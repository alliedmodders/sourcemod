#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_H_

#include "interfaces/ITextParsers.h"

using namespace SourceMod;

class CTextParsers : public ITextParsers
{

public:
	CTextParsers();
public:
	virtual bool ParseFile_INI(const char *file, 
		ITextListener_INI *ini_listener,
		unsigned int *line,
		unsigned int *col);

	virtual bool ParseFile_SMC(const char *file, 
		ITextListener_SMC *smc_listener, 
		unsigned int *line, 
		unsigned int *col);
};

extern CTextParsers g_TextParse;

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_H_
