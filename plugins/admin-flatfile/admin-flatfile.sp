/**
 * admin-flatfile.sp
 * Manages the standard flat files for admins.  This is the file to compile.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
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


#include <sourcemod>
#include <textparse>

/* We like semicolons */
#pragma semicolon 1

public Plugin:myinfo = 
{
	name = "Admin File Reader",
	author = "AlliedModders LLC",
	description = "Reads admin files",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/** Various parsing globals */
new bool:g_FlagsSet[26];					/* Maps whether flags are set */
new AdminFlag:g_FlagLetters[26];			/* Maps the flag letters */		
new bool:g_LoggedFileName = false;			/* Whether or not the file name has been logged */
new g_ErrorCount = 0;						/* Current error count */
new g_IgnoreLevel = 0;						/* Nested ignored section count, so users can screw up files safely */
new String:g_Filename[PLATFORM_MAX_PATH];	/* Used for error messages */

#include "admin-levels.sp"
#include "admin-overrides.sp"
#include "admin-groups.sp"
#include "admin-users.sp"
#include "admin-simple.sp"

public OnRebuildAdminCache(AdminCachePart:part)
{
	RefreshLevels();
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

ParseError(const String:format[], {Handle,String,Float,_}:...)
{
	decl String:buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing %s", g_Filename);
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (%d) %s", ++g_ErrorCount, buffer);
}

InitGlobalStates()
{
	g_ErrorCount = 0;
	g_IgnoreLevel = 0;
	g_LoggedFileName = false;
}
