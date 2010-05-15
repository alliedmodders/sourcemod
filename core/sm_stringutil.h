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

#ifndef _INCLUDE_SOURCEMOD_STRINGUTIL_H_
#define _INCLUDE_SOURCEMOD_STRINGUTIL_H_

#include <math.h>
#include <sp_vm_api.h>
#include <sp_typeutil.h>
#include <ITranslator.h>

using namespace SourcePawn;
using namespace SourceMod;

#define IS_STR_FILLED(var) (var[0] != '\0')

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param);
unsigned int strncopy(char *dest, const char *src, size_t count);
bool gnprintf(char *buffer,
			  size_t maxlen,
			  const char *format,
			  IPhraseCollection *pPhrases,
			  void **params,
			  unsigned int numparams,
			  unsigned int &curparam,
			  size_t *pOutLength,
			  const char **pFailPhrase);
size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
size_t UTIL_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap);
char *sm_strdup(const char *str);
char *UTIL_TrimWhitespace(char *str, size_t &len);
size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr);
char *UTIL_ToLowerCase(const char *str);

#endif // _INCLUDE_SOURCEMOD_STRINGUTIL_H_

