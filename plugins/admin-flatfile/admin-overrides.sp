/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Reads overrides from the admin_levels.cfg file.  Do not compile
 * this directly.
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

enum OverrideState
{
	OverrideState_None,
	OverrideState_Levels,
	OverrideState_Overrides,
}

static SMCParser g_hOldOverrideParser;
static SMCParser g_hNewOverrideParser;
static OverrideState g_OverrideState = OverrideState_None;

public SMCResult ReadOldOverrides_NewSection(SMCParser smc, const char[] name, bool opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OverrideState_None)
	{
		if (StrEqual(name, "Levels"))
		{
			g_OverrideState = OverrideState_Levels;
		} else {
			g_IgnoreLevel++;
		}
	} else if (g_OverrideState == OverrideState_Levels) {
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OverrideState_Overrides;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadNewOverrides_NewSection(SMCParser smc, const char[] name, bool opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OverrideState_None)
	{
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OverrideState_Overrides;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadOverrides_KeyValue(SMCParser smc, 
										const char[] key, 
										const char[] value,
										bool key_quotes, 
										bool value_quotes)
{
	if (g_OverrideState != OverrideState_Overrides || g_IgnoreLevel)
	{
		return SMCParse_Continue;
	}
	
	int flags = ReadFlagString(value);
	
	if (key[0] == '@')
	{
		AddCommandOverride(key[1], Override_CommandGroup, flags);
	} else {
		AddCommandOverride(key, Override_Command, flags);
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadOldOverrides_EndSection(SMCParser smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OverrideState_Levels)
	{
		g_OverrideState = OverrideState_None;
	} else if (g_OverrideState == OverrideState_Overrides) {
		/* We're totally done parsing */
		g_OverrideState = OverrideState_Levels;
		return SMCParse_Halt;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadNewOverrides_EndSection(SMCParser smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OverrideState_Overrides)
	{
		g_OverrideState = OverrideState_None;
	}
	
	return SMCParse_Continue;
}

public SMCResult ReadOverrides_CurrentLine(SMCParser smc, const char[] line, int lineno)
{
	g_CurrentLine = lineno;
	
	return SMCParse_Continue;
}

static void InitializeOverrideParsers()
{
	if (!g_hOldOverrideParser)
	{
		g_hOldOverrideParser = new SMCParser();
		g_hOldOverrideParser.OnEnterSection = ReadOldOverrides_NewSection;
		g_hOldOverrideParser.OnKeyValue = ReadOverrides_KeyValue;
		g_hOldOverrideParser.OnLeaveSection = ReadOldOverrides_EndSection;
		g_hOldOverrideParser.OnRawLine = ReadOverrides_CurrentLine;
	}
	if (!g_hNewOverrideParser)
	{
		g_hNewOverrideParser = new SMCParser();
		g_hNewOverrideParser.OnEnterSection = ReadNewOverrides_NewSection;
		g_hNewOverrideParser.OnKeyValue = ReadOverrides_KeyValue;
		g_hNewOverrideParser.OnLeaveSection = ReadNewOverrides_EndSection;
		g_hNewOverrideParser.OnRawLine = ReadOverrides_CurrentLine;
	}
}

void InternalReadOverrides(SMCParser parser, const char[] file)
{
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), file);
	
	/* Set states */
	InitGlobalStates();
	g_OverrideState = OverrideState_None;
		
	SMCError err = parser.ParseFile(g_Filename);
	if (err != SMCError_Okay)
	{
		char buffer[64];
		if (parser.GetErrorString(err, buffer, sizeof(buffer)))
		{
			ParseError("%s", buffer);
		} else {
			ParseError("Fatal parse error");
		}
	}
}

void ReadOverrides()
{
	InitializeOverrideParsers();
	InternalReadOverrides(g_hOldOverrideParser, "configs/admin_levels.cfg");
	InternalReadOverrides(g_hNewOverrideParser, "configs/admin_overrides.cfg");
}
