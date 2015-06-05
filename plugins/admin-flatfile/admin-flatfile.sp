/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Manages the standard flat files for admins.  This is the file to compile.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

/* We like semicolons */
#pragma semicolon 1

#include <sourcemod>

public Plugin myinfo = 
{
	name = "Admin File Reader",
	author = "AlliedModders LLC",
	description = "Reads admin files",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/** Various parsing globals */
bool g_LoggedFileName = false;       /* Whether or not the file name has been logged */
int g_ErrorCount = 0;                /* Current error count */
int g_IgnoreLevel = 0;               /* Nested ignored section count, so users can screw up files safely */
int g_CurrentLine = 0;               /* Current line we're on */
char g_Filename[PLATFORM_MAX_PATH];  /* Used for error messages */

#include "admin-overrides.sp"
#include "admin-groups.sp"
#include "admin-users.sp"
#include "admin-simple.sp"

public void OnRebuildAdminCache(AdminCachePart part)
{
	if (part == AdminCache_Overrides)
	{
		ReadOverrides();
	} else if (part == AdminCache_Groups) {
		ReadGroups();
	} else if (part == AdminCache_Admins) {
		ReadUsers();
		ReadSimpleUsers();
	}
}

void ParseError(const char[] format, any ...)
{
	char buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing %s", g_Filename);
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (line %d) %s", g_CurrentLine, buffer);
	
	g_ErrorCount++;
}

void InitGlobalStates()
{
	g_ErrorCount = 0;
	g_IgnoreLevel = 0;
	g_CurrentLine = 0;
	g_LoggedFileName = false;
}
