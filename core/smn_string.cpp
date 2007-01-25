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

#include "sm_platform.h"
#include <string.h>
#include <stdlib.h>
#include "sm_globals.h"
#include "sm_stringutil.h"

using namespace SourcePawn;

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

REGISTER_NATIVES(basicstrings)
{
	{"strlen",				sm_strlen},
	{"StrContains",			sm_contain},
	{"StrCompare",			sm_strcmp},
	{"StrCopy",				sm_strcopy},
	{"StringToInt",			sm_strconvint},
	{"IntToString",			sm_numtostr},
	{"StringToFloat",		sm_strtofloat},
	{"FloatToString",		sm_floattostr},
	{"Format",				sm_format},
	{"FormatEx",			sm_formatex},
	{NULL,					NULL},
};
