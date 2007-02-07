/**
 * admin-users.sp
 * Reads the admins.cfg file.  Do not compile this directly.
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
		StrCopy(g_CurAuth, sizeof(g_CurAuth), value);
	} else if (StrEqual(key, "ident")) {
		auth = true;
		StrCopy(g_CurIdent, sizeof(g_CurIdent), value);
	} else if (StrEqual(key, "password")) {
		SetAdminPassword(g_CurUser, value);
	} else if (StrEqual(key, "group")) {
		new GroupId:id = FindAdmGroup(value);
		if (id == INVALID_GROUP_ID)
		{
			ParseError("Unknown group \"%s\"", value);
		} else if (!AdminInheritGroup(g_CurUser, id)) {
			ParseError("Unable to inherit group \"%s\"", value);
		}
	} else if (StrEqual(key, "flags")) {
		new len = strlen(value);
		
		for (new i=0; i<len; i++)
		{
			if (value[i] < 'a' || value[i] > 'z')
			{
				ParseError("Invalid flag detected: %c", value[i]);
				continue;
			}
			new val = value[i] - 'a';
			if (!g_FlagsSet[val])
			{
				ParseError("Invalid flag detected: %c", value[i]);
				continue;
			}
			SetAdminFlag(g_CurUser, g_FlagLetters[val], true);
		}
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

static InitializeUserParser()
{
	if (g_hUserParser == INVALID_HANDLE)
	{
		g_hUserParser = SMC_CreateParser();
		SMC_SetReaders(g_hUserParser,
					   ReadUsers_NewSection,
					   ReadUsers_KeyValue,
					   ReadUsers_EndSection);
	}
}

ReadUsers()
{
	InitializeUserParser();
	
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admims.cfg");

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
