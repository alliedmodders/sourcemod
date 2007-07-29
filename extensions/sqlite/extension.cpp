/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SQLite Driver Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "driver/SqDriver.h"

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

SqliteExt g_SqliteExt;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_SqliteExt);

bool SqliteExt::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	g_SqDriver.Initialize();
	dbi->AddDriver(&g_SqDriver);

	return true;
}

void SqliteExt::SDK_OnUnload()
{
	dbi->RemoveDriver(&g_SqDriver);
	g_SqDriver.Shutdown();
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	} else {
		return len;
	}
}
