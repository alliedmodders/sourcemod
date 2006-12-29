#ifndef _INCLUDE_SOURCEMOD_STRINGUTIL_H_
#define _INCLUDE_SOURCEMOD_STRINGUTIL_H_

#include <math.h>
#include "sp_vm_api.h"
#include "sp_vm_typeutil.h"

using namespace SourcePawn;

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param);
const char *stristr(const char *str, const char *substr);
int StrConvInt(const char *str);
float StrConvFloat(const char *str);
int strncopy(char *dest, const char *src, size_t count);

#endif // _INCLUDE_SOURCEMOD_STRINGUTIL_H_
