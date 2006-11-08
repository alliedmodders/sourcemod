#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_H_

#include "interfaces/ITextParsers.h"

using namespace SourceMod;

inline unsigned int _GetUTF8CharBytes(const char *stream)
{
	unsigned char c = *(unsigned char *)stream;
	if (c & (1<<7))
	{
		if (c & (1<<5))
		{
			if (c & (1<<4))
			{
				return 4;
			}
			return 3;
		}
		return 2;
	}
	return 1;
}

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
		unsigned int *col,
		bool strict);

	virtual unsigned int GetUTF8CharBytes(const char *stream);
};

extern CTextParsers g_TextParse;

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_H_
