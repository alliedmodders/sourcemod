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
#elif PLATFORM_LINUX
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

	return snprintf(str, params[3], "%d", params[1]);
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

	return snprintf(str, params[3], "%f", sp_ctof(params[1]));
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
	{NULL,					NULL},
};
