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

#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_H_

#include <ITextParsers.h>
#include "sm_globals.h"

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

bool IsWhitespace(const char *stream);

/**
 * @param void *			IN: Stream pointer
 * @param char *			IN/OUT: Stream buffer
 * @param size_t			IN: Maximum size of buffer
 * @param unsigned int *	OUT: Number of bytes read (0 = end of stream)
 * @return					True on success, false on failure
 */
typedef bool (*STREAMREADER)(void *, char *, size_t, unsigned int *);

class TextParsers : 
	public ITextParsers,
	public SMGlobalClass
{
public:
	TextParsers();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
public:
	bool ParseFile_INI(const char *file, 
		ITextListener_INI *ini_listener,
		unsigned int *line,
		unsigned int *col);

	SMCParseError ParseFile_SMC(const char *file, 
		ITextListener_SMC *smc_listener, 
		unsigned int *line, 
		unsigned int *col);

	unsigned int GetUTF8CharBytes(const char *stream);

	const char *GetSMCErrorString(SMCParseError err);
private:
	SMCParseError ParseString_SMC(const char *stream, 
		ITextListener_SMC *smc,
		unsigned int *line,
		unsigned int *col);
	SMCParseError ParseStream_SMC(void *stream, 
		STREAMREADER srdr,
		ITextListener_SMC *smc,
		unsigned int *line, 
		unsigned int *col);

};

extern TextParsers g_TextParser;

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_H_
