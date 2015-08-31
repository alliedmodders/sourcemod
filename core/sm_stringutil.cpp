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

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <ITextParsers.h>
#include "sm_stringutil.h"
#include "Logger.h"
#include "PlayerManager.h"
#include "logic_bridge.h"
#include "sourcemod.h"
#include <am-utility.h>
#include <am-float.h>

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	return ke::SafeStrcpy(dest, count, src);
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = ke::SafeVsprintf(buffer, maxlength, fmt, ap);
	va_end(ap);
	return len;
}

size_t UTIL_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap)
{
	return ke::SafeVsprintf(buffer, maxlength, fmt, ap);
}

char *sm_strdup(const char *str)
{
	char *ptr = new char[strlen(str)+1];
	strcpy(ptr, str);
	return ptr;
}

char *UTIL_ToLowerCase(const char *str)
{
	size_t len = strlen(str);
	char *buffer = new char[len + 1];
	for (size_t i = 0; i < len; i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
			buffer[i] = tolower(str[i]);
		else
			buffer[i] = str[i];
	}
	buffer[len] = '\0';
	return buffer;
}

