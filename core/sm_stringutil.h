#ifndef _INCLUDE_SOURCEMOD_STRINGUTIL_H_
#define _INCLUDE_SOURCEMOD_STRINGUTIL_H_

#include <math.h>
#include "sp_vm_api.h"
#include "sp_vm_typeutil.h"

using namespace SourcePawn;

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param);

#endif // _INCLUDE_SOURCEMOD_STRINGUTIL_H_
