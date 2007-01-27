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

#ifndef _INCLUDE_SOURCEMOD_STRINGUTIL_H_
#define _INCLUDE_SOURCEMOD_STRINGUTIL_H_

#include <math.h>
#include "sp_vm_api.h"
#include "sp_typeutil.h"

using namespace SourcePawn;

#define LANG_SERVER		0

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param);
const char *stristr(const char *str, const char *substr);
unsigned int strncopy(char *dest, const char *src, size_t count);
size_t gnprintf(char *buffer, size_t maxlen, const char *format, void **args);
size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

#endif // _INCLUDE_SOURCEMOD_STRINGUTIL_H_
