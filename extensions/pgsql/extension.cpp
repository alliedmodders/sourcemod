/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod PostgreSQL Extension
 * Copyright (C) 2013 AlliedModders LLC.  All rights reserved.
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

#include <sourcemod_version.h>
#include "extension.h"
#include "pgsql/PgDriver.h"
#include <assert.h>
#include <stdlib.h>

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

DBI_PgSQL g_PgSqlDBI;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_PgSqlDBI);

bool DBI_PgSQL::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	dbi->AddDriver(&g_PgDriver);

	return true;
}

void DBI_PgSQL::SDK_OnUnload()
{
	dbi->RemoveDriver(&g_PgDriver);
}

const char *DBI_PgSQL::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *DBI_PgSQL::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}
