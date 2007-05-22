/**
 * vim: set ts=4 :
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

#include <string.h>
#include <stdlib.h>
#include "sm_globals.h"
#include "sm_stringutil.h"
#include "TextParsers.h"
#include <ctype.h>

inline const char *_strstr(const char *str, const char *substr)
{
#ifdef PLATFORM_WINDOWS
	return strstr(str, substr);
#elif defined PLATFORM_LINUX
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

	return static_cast<cell_t>(strtol(str, &dummy, params[2]));
}

static cell_t sm_numtostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);
	size_t res = UTIL_Format(str, params[3], "%d", params[1]);

	return static_cast<cell_t>(res);
}

static cell_t sm_strtofloat(IPluginContext *pCtx, const cell_t *params)
{
	char *str, *dummy;
	pCtx->LocalToString(params[1], &str);

	float val = (float)strtod(str, &dummy);

	return sp_ftoc(val);
}

static cell_t sm_floattostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);
	size_t res = UTIL_Format(str, params[3], "%f", sp_ctof(params[1]));

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

static char g_formatbuf[2048];
static cell_t sm_format(IPluginContext *pCtx, const cell_t *params)
{
	char *buf, *fmt, *destbuf;
	cell_t start_addr, end_addr, maxparam;
	size_t res, maxlen;
	int arg = 4;
	bool copy = false;

	pCtx->LocalToString(params[1], &destbuf);
	pCtx->LocalToString(params[3], &fmt);

	maxlen = static_cast<size_t>(params[2]);
	start_addr = params[1];
	end_addr = params[1] + maxlen;
	maxparam = params[0];

	for (cell_t i=3; i<=maxparam; i++)
	{
		if ((params[i] >= start_addr) && (params[i] <= end_addr))
		{
			copy = true;
			break;
		}
	}
	buf = (copy) ? g_formatbuf : destbuf;
	res = atcprintf(buf, maxlen, fmt, pCtx, params, &arg);

	if (copy)
	{
		memcpy(destbuf, g_formatbuf, res+1);
	}

	return static_cast<cell_t>(res);
}

static cell_t sm_vformat(IPluginContext *pContext, const cell_t *params)
{
	int vargPos = static_cast<int>(params[4]);

	/* Get the parent parameter array */
	sp_context_t *ctx = pContext->GetContext();
	cell_t *local_params = (cell_t *)(ctx->memory + ctx->frm + (2 * sizeof(cell_t)));

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
		destination = g_formatbuf;
	} else {
		pContext->LocalToString(params[1], &destination);
	}

	pContext->LocalToString(params[3], &format);

	size_t total = atcprintf(destination, maxlen, format, pContext, local_params, &vargPos);

	/* Perform copy-on-write if we need to */
	if (copy)
	{
		pContext->StringToLocal(params[1], maxlen, g_formatbuf);
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
	while (*inptr != '\0' && IsWhitespace(inptr))
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
		while (*inptr != '\0' && !IsWhitespace(inptr))
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
	while (*inptr != '\0' && IsWhitespace(inptr))
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

	return isalpha(chr);
}

static cell_t IsCharNumeric(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isdigit(chr);
}

static cell_t IsCharSpace(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return isspace(chr);
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

	return isupper(chr);
}

static cell_t IsCharLower(IPluginContext *pContext, const cell_t *params)
{
	char chr = params[1];

	if (_GetUTF8CharBytes(&chr) != 1)
	{
		return 0;
	}

	return islower(chr);
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
	while (end >= str && IsWhitespace(end))
	{
		end--;
	}

	/* Replace first whitespace char (at the end) with null terminator */
	*(end + 1) = '\0';

	/* Iterate forwards through string until first non-whitespace char is reached */
	while (IsWhitespace(str))
	{
		str++;
	}

	size_t bytes;
	pContext->StringToLocalUTF8(params[1], chars + 1, str, &bytes);

	return bytes;
}

REGISTER_NATIVES(basicStrings)
{
	{"GetCharBytes",		GetCharBytes},
	{"IntToString",			sm_numtostr},
	{"IsCharAlpha",			IsCharAlpha},
	{"IsCharLower",			IsCharLower},
	{"IsCharMB",			IsCharMB},
	{"IsCharNumeric",		IsCharNumeric},
	{"IsCharSpace",			IsCharSpace},
	{"IsCharUpper",			IsCharUpper},
	{"strlen",				sm_strlen},
	{"StrBreak",			BreakString},		/* Backwards compat shim */
	{"BreakString",			BreakString},
	{"StrContains",			sm_contain},
	{"strcmp",				sm_strcmp},
	{"StrCompare",			sm_strcmp},			/* Backwards compat shim */
	{"strncmp",				sm_strncmp},
	{"strcopy",				sm_strcopy},
	{"StrCopy",				sm_strcopy},		/* Backwards compat shim */
	{"StringToInt",			sm_strconvint},
	{"StringToFloat",		sm_strtofloat},
	{"FloatToString",		sm_floattostr},
	{"Format",				sm_format},
	{"FormatEx",			sm_formatex},
	{"VFormat",				sm_vformat},
	{"TrimString",			TrimString},
	{NULL,					NULL},
};
