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
