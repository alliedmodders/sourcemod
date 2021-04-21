/**
 * vim: set ts=4 sw=4 tw=99 et :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_COMMON_STRINGUTIL_H_
#define _INCLUDE_SOURCEMOD_COMMON_STRINGUTIL_H_

const char *stristr(const char *str, const char *substr);
unsigned int strncopy(char *dest, const char *src, size_t count);
unsigned int UTIL_ReplaceAll(char *subject, size_t maxlength, const char *search,
                             const char *replace, bool caseSensitive = true);
char *UTIL_ReplaceEx(char *subject, size_t maxLen, const char *search, size_t searchLen,
                     const char *replace, size_t replaceLen, bool caseSensitive = true);
size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr);

void UTIL_StripExtension(const char *in, char *out, int outSize);
char *UTIL_TrimWhitespace(char *str, size_t &len);

// Internal copying Format helper, expects (char[] buffer, int maxlength, const char[] format, any ...) starting at |start|
// i.e. you can stuff your own params before |buffer|.
cell_t InternalFormat(IPluginContext *pCtx, const cell_t *params, int start);

#endif /* _INCLUDE_SOURCEMOD_COMMON_STRINGUTIL_H_ */

