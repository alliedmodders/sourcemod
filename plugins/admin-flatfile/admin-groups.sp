/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Reads the admin_groups.cfg file.  Do not compile this directly.
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

enum GroupState
{
	GroupState_None,
	GroupState_Groups,
	GroupState_InGroup,
	GroupState_Overrides,
}

enum GroupPass
{
	GroupPass_Invalid,
	GroupPass_First,
	GroupPass_Second,
}

static SMCParser g_hGroupParser;
static GroupId g_CurGrp = INVALID_GROUP_ID;
static GroupState g_GroupState = GroupState_None;
static GroupPass g_GroupPass = GroupPass_Invalid;
static bool g_NeedReparse = false;

public SMCResult ReadGroups_NewSection(SMCParser smc, const char[] name, bool opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_GroupState == GroupState_None)
	{
		if (StrEqual(name, "Groups"))
		{
			g_GroupState = GroupState_Groups;
		} else {
			g_IgnoreLevel++;
		}
	} else if (g_GroupState == GroupState_Groups) {
		if ((g_CurGrp = CreateAdmGroup(name)) == INVALID_GROUP_ID)
		{
			g_CurGrp = FindAdmGroup(name);
		}
		g_GroupState = GroupState_InGroup;
	} else if (g_GroupState == GroupState_InGroup) {
		if (StrEqual(name, "Overrides"))
		{
			g_GroupState = GroupState_Overrides;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadGroups_KeyValue(SMCParser smc, 
										const char[] key, 
										const char[] value, 
										bool key_quotes, 
										bool value_quotes)
{
	if (g_CurGrp == INVALID_GROUP_ID || g_IgnoreLevel)
	{
		return SMCParse_Continue;
	}
	
	AdminFlag flag;
	
	if (g_GroupPass == GroupPass_First)
	{
		if (g_GroupState == GroupState_InGroup)
		{
			if (StrEqual(key, "flags"))
			{
				int len = strlen(value);
				for (int i=0; i<len; i++)
				{
					if (!FindFlagByChar(value[i], flag))
					{
						continue;
					}
					g_CurGrp.SetFlag(flag, true);
				}
			} else if (StrEqual(key, "immunity")) {
				g_NeedReparse = true;
			}
		} else if (g_GroupState == GroupState_Overrides) {
			OverrideRule rule = Command_Deny;
			
			if (StrEqual(value, "allow", false))
			{
				rule = Command_Allow;
			}
			
			if (key[0] == '@')
			{
				g_CurGrp.AddCommandOverride(key[1], Override_CommandGroup, rule);
			} else {
				g_CurGrp.AddCommandOverride(key, Override_Command, rule);
			}
		}
	} else if (g_GroupPass == GroupPass_Second
			   && g_GroupState == GroupState_InGroup) {
		/* Check for immunity again, core should handle double inserts */
		if (StrEqual(key, "immunity"))
		{
			/* If it's a value we know about, use it */
			if (StrEqual(value, "*"))
			{
				g_CurGrp.ImmunityLevel = 2;
			} else if (StrEqual(value, "$")) {
				g_CurGrp.ImmunityLevel = 1;
			} else {
				int level;
				if (StringToIntEx(value, level))
				{
					g_CurGrp.ImmunityLevel = level;
				} else {
					GroupId id;
					if (value[0] == '@')
					{
						id = FindAdmGroup(value[1]);
					} else {
						id = FindAdmGroup(value);
					}
					if (id != INVALID_GROUP_ID)
					{
						g_CurGrp.AddGroupImmunity(id);
					} else {
						ParseError("Unable to find group: \"%s\"", value);
					}
				}
			}
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadGroups_EndSection(SMCParser smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_GroupState == GroupState_Overrides)
	{
		g_GroupState = GroupState_InGroup;
	} else if (g_GroupState == GroupState_InGroup) {
		g_GroupState = GroupState_Groups;
		g_CurGrp = INVALID_GROUP_ID;
	} else if (g_GroupState == GroupState_Groups) {
		g_GroupState = GroupState_None;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadGroups_CurrentLine(SMCParser smc, const char[] line, int lineno)
{
	g_CurrentLine = lineno;
	
	return SMCParse_Continue;
}

static void InitializeGroupParser()
{
	if (!g_hGroupParser)
	{
		g_hGroupParser = new SMCParser();
		g_hGroupParser.OnEnterSection = ReadGroups_NewSection;
		g_hGroupParser.OnKeyValue = ReadGroups_KeyValue;
		g_hGroupParser.OnLeaveSection = ReadGroups_EndSection;
		g_hGroupParser.OnRawLine = ReadGroups_CurrentLine;
	}
}

static void InternalReadGroups(const char[] path, GroupPass pass)
{
	/* Set states */
	InitGlobalStates();
	g_GroupState = GroupState_None;
	g_CurGrp = INVALID_GROUP_ID;
	g_GroupPass = pass;
	g_NeedReparse = false;
		
	SMCError err = g_hGroupParser.ParseFile(path);
	if (err != SMCError_Okay)
	{
		char buffer[64];
		if (g_hGroupParser.GetErrorString(err, buffer, sizeof(buffer)))
		{
			ParseError("%s", buffer);
		} else {
			ParseError("Fatal parse error");
		}
	}
}

void ReadGroups()
{
	InitializeGroupParser();
	
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admin_groups.cfg");
	
	InternalReadGroups(g_Filename, GroupPass_First);
	if (g_NeedReparse)
	{
		InternalReadGroups(g_Filename, GroupPass_Second);
	}
}
