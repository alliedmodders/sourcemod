/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#include "common_logic.h"
#include <string.h>
#include <stdlib.h>
#include <ITextParsers.h>
#include <ctype.h>
#include "stringutil.h"
#include "sprintf.h"
#include <am-string.h>

inline const char *_strstr(const char *str, const char *substr)
{
#ifdef PLATFORM_WINDOWS
	return strstr(str, substr);
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
	return (const char *)strstr(str, substr);
#endif
}

/*********************************************
*                                            *
* STRING MANIPULATION NATIVE IMPLEMENTATIONS *
*                                            *
*********************************************/

static cell_t sm_strlen(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[1], &str);

	return strlen(str);
}

static cell_t sm_contain(IPluginContext *pCtx, const cell_t *params)
{
	typedef const char *(*STRSEARCH)(const char *, const char *);
	STRSEARCH func;
	char *str, *substr;

	pCtx->LocalToString(params[1], &str);
	pCtx->LocalToString(params[2], &substr);

	func = (params[3]) ? _strstr : stristr;
	const char *pos = func(str, substr);
	if (pos)
	{
		return (pos - str);
	}

	return -1;
}

static cell_t sm_strcmp(IPluginContext *pCtx, const cell_t *params)
{
	typedef int (*STRCOMPARE)(const char *, const char *);
	STRCOMPARE func;
	char *str1, *str2;

	pCtx->LocalToString(params[1], &str1);
	pCtx->LocalToString(params[2], &str2);

	func = (params[3]) ? strcmp : stricmp;

	return (func(str1, str2));
}

static cell_t sm_strncmp(IPluginContext *pCtx, const cell_t *params)
{
	char *str1, *str2;

	pCtx->LocalToString(params[1], &str1);
	pCtx->LocalToString(params[2], &str2);
	
	if  (params[4])
	{
		return strncmp(str1, str2, (size_t)params[3]);
	} else {
		return strncasecmp(str1, str2, (size_t)params[3]);
	}
}

static cell_t sm_strcopy(IPluginContext *pCtx, const cell_t *params)
{
	char *dest, *src;

	pCtx->LocalToString(params[1], &dest);
	pCtx->LocalToString(params[3], &src);

	return strncopy(dest, src, params[2]);
}

static cell_t sm_strconvint(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy;
	pCtx->LocalToString(params[1], &str);

	return static_cast<cell_t>(strtoul(str, &dummy, params[2]));
}

static cell_t StringToIntEx(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy = NULL;
	cell_t *addr;
	pCtx->LocalToString(params[1], &str);
	pCtx->LocalToPhysAddr(params[2], &addr);

	*addr = static_cast<cell_t>(strtoul(str, &dummy, params[3]));

	return dummy - str;
}

static cell_t StringToInt64(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy = NULL;
	cell_t *addr;
	pCtx->LocalToString(params[1], &str);
	pCtx->LocalToPhysAddr(params[2], &addr);

	// uint64_t for correct signed right shift.
	uint64_t number = (uint64_t)strtoll(str, &dummy, params[3]);

	addr[0] = (cell_t)(number & 0xFFFFFFFFull);
	addr[1] = (cell_t)(number >> 32ull);

	return dummy - str;
}

static cell_t sm_numtostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);
	size_t res = ke::SafeSprintf(str, params[3], "%d", params[1]);

	return static_cast<cell_t>(res);
}

static cell_t Int64ToString(IPluginContext *pCtx, const cell_t *params)
{
	cell_t *num;
	char *str;
	pCtx->LocalToPhysAddr(params[1], &num);
	pCtx->LocalToString(params[2], &str);
	size_t res = ke::SafeSprintf(str, params[3], "%" KE_FMT_I64, (uint64_t)num[1] << 32ll | (uint32_t)num[0]);

	return static_cast<cell_t>(res);
}

static cell_t sm_strtofloat(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy;
	pCtx->LocalToString(params[1], &str);

	float val = (float)strtod(str, &dummy);

	return sp_ftoc(val);
}

static cell_t StringToFloatEx(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy = NULL;
	cell_t *addr;
	pCtx->LocalToString(params[1], &str);
	pCtx->LocalToPhysAddr(params[2], &addr);

	float val = (float)strtod(str, &dummy);

	*addr = sp_ftoc(val);

	return dummy - str;
}

static cell_t sm_floattostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);
	size_t res = ke::SafeSprintf(str, params[3], "%f", sp_ctof(params[1]));

	return static_cast<cell_t>(res);
}

static cell_t sm_formatex(IPluginContext *pCtx, const cell_t *params)
{
	char *buf, *fmt;
	size_t res;
	int arg = 4;

	pCtx->LocalToString(params[1], &buf);
	pCtx->LocalToString(params[3], &fmt);
	res = atcprintf(buf, static_cast<size_t>(params[2]), fmt, pCtx, params, &arg);

	return static_cast<cell_t>(res);
}

static cell_t sm_format(IPluginContext *pCtx, const cell_t *params)
{
	return InternalFormat(pCtx, params, 0);
}

static char g_vformatbuf[2048];
static cell_t sm_vformat(IPluginContext *pContext, const cell_t *params)
{
	int vargPos = static_cast<int>(params[4]);

	/* Get the parent parameter array */
	cell_t *local_params = pContext->GetLocalParams();

	cell_t max = local_params[0];
	if (vargPos > (int)max + 1)
	{
		return pContext->ThrowNativeError("Argument index is invalid: %d", vargPos);
	}

	cell_t addr_start = params[1];
	cell_t addr_end = addr_start + params[2];
	bool copy = false;
	for (int i=vargPos; i<=max; i++)
	{
		/* Does this clip bounds? */
		if ((local_params[i] >= addr_start)
			&& (local_params[i] <= addr_end))
		{
			copy = true;
			break;
		}
	}

	/* Get destination info */
	char *format, *destination;
	size_t maxlen = static_cast<size_t>(params[2]);

	if (copy)
	{
		destination = g_vformatbuf;
	} else {
		pContext->LocalToString(params[1], &destination);
	}

	pContext->LocalToString(params[3], &format);

	size_t total = atcprintf(destination, maxlen, format, pContext, local_params, &vargPos);

	/* Perform copy-on-write if we need to */
	if (copy)
	{
		pContext->StringToLocal(params[1], maxlen, g_vformatbuf);
	}

	return total;
}

/* :TODO: make this UTF8 safe */
static cell_t BreakString(IPluginContext *pContext, const cell_t *params)
{
	const char *input;
	char *out;
	size_t outMax;

	/* Get parameters */
	pContext->LocalToString(params[1], (char **)&input);
	pContext->LocalToString(params[2], &out);
	outMax = params[3];

	const char *inptr = input;
	/* Eat up whitespace */
	while (*inptr != '\0' && textparsers->IsWhitespace(inptr))
	{
		inptr++;
	}

	if (*inptr == '\0')
	{
		if (outMax)
		{
			*out = '\0';
		}
		return -1;
	}

	const char *start, *end = NULL;

	bool quoted = (*inptr == '"');
	if (quoted)
	{
		inptr++;
		start = inptr;
		/* Read input until we reach a quote. */
		while (*inptr != '\0' && *inptr != '"')
		{
			/* Update the end point, increment the stream. */
			end = inptr++;
		}
		/* Read one more token if we reached an end quote */
		if (*inptr == '"')
		{
			inptr++;
		}
	} else {
		start = inptr;
		/* Read input until we reach a space */
		while (*inptr != '\0' && !textparsers->IsWhitespace(inptr))
		{
			/* Update the end point, increment the stream. */
			end = inptr++;
		}
	}

	/* Copy the string we found, if necessary */
	if (end == NULL)
	{
		if (outMax)
		{
			*out = '\0';
		}
	} else if (outMax) {
		char *outptr = out;
		outMax--;
		for (const char *ptr=start;
			(ptr <= end) && ((unsigned)(outptr - out) < (outMax));
			ptr++, outptr++)
			{
			*outptr = *ptr;
		}
		*outptr = '\0';
	}

	/* Consume more of the string until we reach non-whitespace */
	while (*inptr != '\0' && textparsers->IsWhitespace(inptr))
	{
		inptr++;
	}

	if (*inptr == '\0')
	{
		return -1;
	}

	return inptr - input;
}

static cell_t GetCharBytes(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	return _GetUTF8CharBytes(str);
};

static cell_t IsCharAlpha(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isalpha(chr) == 0 ? 0 : 1;
}

static cell_t IsCharNumeric(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isdigit(chr) == 0 ? 0 : 1;
}

static cell_t IsCharSpace(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isspace(chr) == 0 ? 0 : 1;
}

static cell_t IsCharMB(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	unsigned int bytes = _GetUTF8CharBytes(&chr);
	if (bytes == 1)
	{
		return 0;
	}

	return bytes;
}

static cell_t IsCharUpper(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isupper(chr) == 0 ? 0 : 1;
}

static cell_t IsCharLower(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return islower(chr) == 0 ? 0 : 1;
}

static cell_t ReplaceString(IPluginContext *pContext, const cell_t *params)
{
	char *text, *search, *replace;
	size_t maxlength;
	bool caseSensitive;
	
	pContext->LocalToString(params[1], &text);
	pContext->LocalToString(params[3], &search);
	pContext->LocalToString(params[4], &replace);
	maxlength = (size_t)params[2];

	/* Account for old binary plugins that won't pass the last parameter */
	if (params[0] == 5) 
	{
		caseSensitive = (bool)params[5];
	}
	else
	{
		caseSensitive = true;
	}

	if (search[0] == '\0')
	{
		return pContext->ThrowNativeError("Cannot replace searches of empty strings");
	}

	return UTIL_ReplaceAll(text, maxlength, search, replace, caseSensitive);
}

static cell_t ReplaceStringEx(IPluginContext *pContext, const cell_t *params)
{
	char *text, *search, *replace;
	size_t maxlength;
	bool caseSensitive;

	pContext->LocalToString(params[1], &text);
	pContext->LocalToString(params[3], &search);
	pContext->LocalToString(params[4], &replace);
	maxlength = (size_t)params[2];

	size_t searchLen = (params[5] == -1) ? strlen(search) : (size_t)params[5];
	size_t replaceLen = (params[6] == -1) ? strlen(replace) : (size_t)params[6];

	/* Account for old binary plugins that won't pass the last parameter */
	if (params[0] == 7)
	{
		caseSensitive = (bool)params[7];
	}
	else
	{
		caseSensitive = true;
	}

	if (searchLen == 0)
	{
		return pContext->ThrowNativeError("Cannot replace searches of empty strings");
	}

	char *ptr = UTIL_ReplaceEx(text, maxlength, search, searchLen, replace, replaceLen, caseSensitive);

	if (ptr == NULL)
	{
		return -1;
	}

	return ptr - text;
}

static cell_t TrimString(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	size_t chars = strlen(str);

	if (chars == 0)
	{
		return 0;
	}

	char *end = str + chars - 1;

	/* Iterate backwards through string until we reach first non-whitespace char */
	while (end >= str && textparsers->IsWhitespace(end))
	{
		end--;
	}

	/* Replace first whitespace char (at the end) with null terminator */
	*(end + 1) = '\0';

	/* Iterate forwards through string until first non-whitespace char is reached */
	while (textparsers->IsWhitespace(str))
	{
		str++;
	}

	size_t bytes;
	pContext->StringToLocalUTF8(params[1], chars + 1, str, &bytes);

	return bytes;
}

static cell_t SplitString(IPluginContext *pContext, const cell_t *params)
{
	char *text, *split;
	
	pContext->LocalToString(params[1], &text);
	pContext->LocalToString(params[2], &split);

	size_t maxLen = (size_t)params[4];
	size_t textLen = strlen(text);
	size_t splitLen = strlen(split);

	if (splitLen > textLen)
	{
		return -1;
	}

	/**
	 * Note that it's <= ... you could also just add 1,
	 * but this is a bit nicer
	 */
	for (size_t i=0; i<=textLen - splitLen; i++)
	{
		if (strncmp(&text[i], split, splitLen) == 0)
		{
			/* Split hereeeee */
			if (i >= maxLen)
			{
				pContext->StringToLocalUTF8(params[3], maxLen, text, NULL);
			} else {
				pContext->StringToLocalUTF8(params[3], i+1, text, NULL);
			}
			return (cell_t)(i + splitLen);
		}
	}

	return -1;
}

static cell_t StripQuotes(IPluginContext *pContext, const cell_t *params)
{
	char *text;
	size_t length;

	pContext->LocalToString(params[1], &text);
	length = strlen(text);

	if (text[0] == '"' && text[length-1] == '"')
	{
		/* Null-terminate */
		text[--length] = '\0';
		/* Move the remaining bytes (including null terminator) down by one */
		memmove(&text[0], &text[1], length);

		return 1;
	}

	return 0;	
}

REGISTER_NATIVES(basicStrings)
{
	{"BreakString",			BreakString},
	{"FloatToString",		sm_floattostr},
	{"Format",				sm_format},
	{"FormatEx",			sm_formatex},
	{"GetCharBytes",		GetCharBytes},
	{"IntToString",			sm_numtostr},
	{"Int64ToString",		Int64ToString},
	{"IsCharAlpha",			IsCharAlpha},
	{"IsCharLower",			IsCharLower},
	{"IsCharMB",			IsCharMB},
	{"IsCharNumeric",		IsCharNumeric},
	{"IsCharSpace",			IsCharSpace},
	{"IsCharUpper",			IsCharUpper},
	{"ReplaceString",		ReplaceString},
	{"ReplaceStringEx",		ReplaceStringEx},
	{"SplitString",			SplitString},
	{"strlen",				sm_strlen},
	{"StrContains",			sm_contain},
	{"strcmp",				sm_strcmp},
	{"strncmp",				sm_strncmp},
	{"strcopy",				sm_strcopy},
	{"StringToInt",			sm_strconvint},
	{"StringToIntEx",		StringToIntEx},
	{"StringToInt64",		StringToInt64},
	{"StringToFloat",		sm_strtofloat},
	{"StringToFloatEx",		StringToFloatEx},
	{"StripQuotes",			StripQuotes},
	{"TrimString",			TrimString},
	{"VFormat",				sm_vformat},
	{NULL,					NULL},
};
