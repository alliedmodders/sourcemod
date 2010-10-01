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

#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_H_

#include <ITextParsers.h>
#include "gdc.h"

using namespace SourceMod;

/**
 * @param void *			IN: Stream pointer
 * @param char *			IN/OUT: Stream buffer
 * @param size_t			IN: Maximum size of buffer
 * @param unsigned int *	OUT: Number of bytes read (0 = end of stream)
 * @return					True on success, false on failure
 */
typedef bool (*STREAMREADER)(void *, char *, size_t, unsigned int *);

class TextParsers : 
	public ITextParsers
{
public:
	TextParsers();
public:
	bool ParseFile_INI(const char *file, 
		ITextListener_INI *ini_listener,
		unsigned int *line,
		unsigned int *col);

	SMCError ParseFile_SMC(const char *file, 
		ITextListener_SMC *smc_listener, 
		SMCStates *states);

	SMCError ParseSMCFile(const char *file,
			ITextListener_SMC *smc_listener,
			SMCStates *states,
			char *buffer,
			size_t maxsize);

	SMCError ParseSMCStream(const char *stream,
			size_t length,
			ITextListener_SMC *smc_listener,
			SMCStates *states,
			char *buffer,
			size_t maxsize);

	unsigned int GetUTF8CharBytes(const char *stream);

	const char *GetSMCErrorString(SMCError err);
	bool IsWhitespace(const char *stream);
private:
	SMCError ParseStream_SMC(void *stream, 
		STREAMREADER srdr,
		ITextListener_SMC *smc,
		SMCStates *states);

};

extern TextParsers g_TextParser;

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_H_

