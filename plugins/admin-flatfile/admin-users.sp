/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Reads the admins.cfg file.  Do not compile this directly.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

#define USER_STATE_NONE			0
#define USER_STATE_ADMINS		1
#define USER_STATE_INADMIN		2

static Handle:g_hUserParser = INVALID_HANDLE;
static AdminId:g_CurUser = INVALID_ADMIN_ID;
static g_UserState = USER_STATE_NONE;
static String:g_CurAuth[64];
static String:g_CurIdent[64];

public SMCResult:ReadUsers_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_UserState == USER_STATE_NONE)
	{
		if (StrEqual(name, "Admins"))
		{
			g_UserState = USER_STATE_ADMINS;
			g_CurUser = INVALID_ADMIN_ID;
		} else {
			g_IgnoreLevel++;
		}
	} else if (g_UserState == USER_STATE_ADMINS) {
		g_UserState = USER_STATE_INADMIN;
		g_CurUser = CreateAdmin(name);
		g_CurAuth[0] = '\0';
		g_CurIdent[0] = '\0';
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadUsers_KeyValue(Handle:smc, 
									const String:key[], 
									const String:value[], 
									bool:key_quotes, 
									bool:value_quotes)
{
	if (g_UserState != USER_STATE_INADMIN
		|| g_IgnoreLevel
		|| g_CurUser == INVALID_ADMIN_ID)
	{
		return SMCParse_Continue;
	}
	
	new bool:auth = false;
	
	if (StrEqual(key, "auth"))
	{
		auth = true;
		strcopy(g_CurAuth, sizeof(g_CurAuth), value);
	}
	else if (StrEqual(key, "identity"))
	{
		auth = true;
		strcopy(g_CurIdent, sizeof(g_CurIdent), value);
	}
	else if (StrEqual(key, "password")) 
	{
		SetAdminPassword(g_CurUser, value);
	} 
	else if (StrEqual(key, "group")) 
	{
		new GroupId:id = FindAdmGroup(value);
		if (id == INVALID_GROUP_ID)
		{
			ParseError("Unknown group \"%s\"", value);
		} else if (!AdminInheritGroup(g_CurUser, id)) {
			ParseError("Unable to inherit group \"%s\"", value);
		}
	} 
	else if (StrEqual(key, "flags")) 
	{
		new len = strlen(value);
		new AdminFlag:flag;
		
		for (new i=0; i<len; i++)
		{
			if (!FindFlagByChar(value[i], flag))
			{
				ParseError("Invalid flag detected: %c", value[i]);
			}
			SetAdminFlag(g_CurUser, flag, true);
		}
	} 
	else if (StrEqual(key, "immunity")) 
	{
		new level = StringToInt(value);
		SetAdminImmunityLevel(g_CurUser, level);
	}

	if (auth && g_CurIdent[0] && g_CurAuth[0])
	{
		if (BindAdminIdentity(g_CurUser, g_CurAuth, g_CurIdent))
		{
			g_CurAuth[0] = '\0';
			g_CurIdent[0] = '\0';
		} else {
			ParseError("Failed to bind auth \"%s\" to identity \"%s\"", g_CurAuth, g_CurIdent);
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadUsers_EndSection(Handle:smc)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_UserState == USER_STATE_INADMIN)
	{
		g_UserState = USER_STATE_ADMINS;
		g_CurUser = INVALID_ADMIN_ID;
	} else if (g_UserState == USER_STATE_ADMINS) {
		g_UserState = USER_STATE_NONE;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadUsers_CurrentLine(Handle:smc, const String:line[], lineno)
{
	g_CurrentLine = lineno;
	
	return SMCParse_Continue;
}

static InitializeUserParser()
{
	if (g_hUserParser == INVALID_HANDLE)
	{
		g_hUserParser = SMC_CreateParser();
		SMC_SetReaders(g_hUserParser,
					   ReadUsers_NewSection,
					   ReadUsers_KeyValue,
					   ReadUsers_EndSection);
		SMC_SetRawLine(g_hUserParser, ReadUsers_CurrentLine);
	}
}

ReadUsers()
{
	InitializeUserParser();
	
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admins.cfg");

	/* Set states */
	InitGlobalStates();
	g_UserState = USER_STATE_NONE;
		
	new SMCError:err = SMC_ParseFile(g_hUserParser, g_Filename);
	if (err != SMCError_Okay)
	{
		decl String:buffer[64];
		if (SMC_GetErrorString(err, buffer, sizeof(buffer)))
		{
			ParseError("%s", buffer);
		} else {
			ParseError("Fatal parse error");
		}
	}
}
