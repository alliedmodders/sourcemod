#include "sm_platform.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "sp_vm_api.h"
#include "sp_vm_typeutil.h"

using namespace SourcePawn;

const char *stristr(const char *str, const char *substr)
{
	if (!*substr)
	{
		return ((char *)str);
	}

	char *needle = (char *)substr;
	char *prevloc = (char *)str;
	char *haystack = (char *)str;

	while (*haystack)
	{
		if (tolower(*haystack) == tolower(*needle))
		{
			haystack++;
			if (!*++needle)
			{
				return prevloc;
			}
		} else {
			haystack = ++prevloc;
			needle = (char *)substr;
		}
	}

	return NULL;
}

inline const char *_strstr(const char *str, const char *substr)
{
#ifdef PLATFORM_WINDOWS
	return strstr(str, substr);
#elif PLATFORM_LINUX
	return (const char *)strstr(str, substr);
#endif
}

inline int StrConvInt(const char *str)
{
	char *dummy;
	return strtol(str, &dummy, 10);
}

inline float StrConvFloat(const char *str)
{
	char *dummy;
	return (float)strtod(str, &dummy);
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

static cell_t sm_equal(IPluginContext *pCtx, const cell_t *params)
{
	typedef int (*STRCOMPARE)(const char *, const char *);
	STRCOMPARE func;
	char *str1, *str2;

	pCtx->LocalToString(params[1], &str1);
	pCtx->LocalToString(params[2], &str2);

	func = (params[3]) ? strcmp : stricmp;

	return (func(str1, str2)) ? 0 : 1;
}

static cell_t sm_strcopy(IPluginContext *pCtx, const cell_t *params)
{
	char *dest, *src, *start;
	int len;

	pCtx->LocalToString(params[1], &dest);
	pCtx->LocalToString(params[3], &src);
	len = params[2];

	start = dest;
	while ((*src) && (len--))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

static cell_t sm_strtonum(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[1], &str);

	return StrConvInt(str);
}

static cell_t sm_numtostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);

	return snprintf(str, params[3], "%d", params[1]);
}

static cell_t sm_strtofloat(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[1], &str);

	return ftoc(StrConvFloat(str));
}

static cell_t sm_floattostr(IPluginContext *pCtx, const cell_t *params)
{
	char *str;
	pCtx->LocalToString(params[2], &str);

	return snprintf(str, params[3], "%f", ctof(params[1]));
}
