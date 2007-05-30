/**
 * vim: set ts=4 :
 * ===============================================================
 * Sample SourceMod Extension
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
 * Version: $Id: extension.cpp 763 2007-05-09 05:20:03Z damagedsoul $
 */

#include "extension.h"
#include "mysql/MyDriver.h"
#include <assert.h>
#include <stdlib.h>

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

DBI_MySQL g_MySqlDBI;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_MySqlDBI);

bool DBI_MySQL::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	dbi->AddDriver(&g_MyDriver);

	return true;
}

void DBI_MySQL::SDK_OnUnload()
{
	dbi->RemoveDriver(&g_MyDriver);
}
