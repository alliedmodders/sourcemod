/* 
 * admin-overrides.sp
 * Reads overrides from the admin_levels.cfg file.  Do not compile this directly.
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

#define OVERRIDE_STATE_NONE			0
#define OVERRIDE_STATE_LEVELS		1
#define OVERRIDE_STATE_OVERRIDES	2

static Handle:g_hOldOverrideParser = INVALID_HANDLE;
static Handle:g_hNewOverrideParser = INVALID_HANDLE;
static g_OverrideState = OVERRIDE_STATE_NONE;

public SMCResult:ReadOldOverrides_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OVERRIDE_STATE_NONE)
	{
		if (StrEqual(name, "Levels"))
		{
			g_OverrideState = OVERRIDE_STATE_LEVELS;
		} else {
			g_IgnoreLevel++;
		}
	} else if (g_OverrideState == OVERRIDE_STATE_LEVELS) {
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OVERRIDE_STATE_OVERRIDES;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadNewOverrides_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OVERRIDE_STATE_NONE)
	{
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OVERRIDE_STATE_OVERRIDES;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOverrides_KeyValue(Handle:smc, 
										const String:key[], 
										const String:value[], 
										bool:key_quotes, 
										bool:value_quotes)
{
	if (g_OverrideState != OVERRIDE_STATE_OVERRIDES
		|| g_IgnoreLevel)
	{
		return SMCParse_Continue;
	}
	
	new flags = ReadFlagString(value);
	
	if (key[0] == '@')
	{
		AddCommandOverride(key[1], Override_CommandGroup, flags);
	} else {
		AddCommandOverride(key, Override_Command, flags);
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOldOverrides_EndSection(Handle:smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OVERRIDE_STATE_LEVELS)
	{
		g_OverrideState = OVERRIDE_STATE_NONE;
	} else if (g_OverrideState == OVERRIDE_STATE_OVERRIDES) {
		/* We're totally done parsing */
		g_OverrideState = OVERRIDE_STATE_LEVELS;
		return SMCParse_Halt;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadNewOverrides_EndSection(Handle:smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OVERRIDE_STATE_OVERRIDES)
	{
		g_OverrideState = OVERRIDE_STATE_NONE;
	}
	
	return SMCParse_Continue;
}

static InitializeOverrideParsers()
{
	if (g_hOldOverrideParser == INVALID_HANDLE)
	{
		g_hOldOverrideParser = SMC_CreateParser();
		SMC_SetReaders(g_hOldOverrideParser,
					   ReadOldOverrides_NewSection,
					   ReadOverrides_KeyValue,
					   ReadOldOverrides_EndSection);
	}
	if (g_hNewOverrideParser == INVALID_HANDLE)
	{
		g_hNewOverrideParser = SMC_CreateParser();
		SMC_SetReaders(g_hNewOverrideParser,
					   ReadNewOverrides_NewSection,
					   ReadOverrides_KeyValue,
					   ReadNewOverrides_EndSection);
	}
}

InternalReadOverrides(Handle:parser, const String:file[])
{
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), file);
	
	/* Set states */
	InitGlobalStates();
	g_OverrideState = OVERRIDE_STATE_NONE;
		
	new SMCError:err = SMC_ParseFile(parser, g_Filename);
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

ReadOverrides()
{
	InitializeOverrideParsers();
	InternalReadOverrides(g_hOldOverrideParser, "configs/admin_levels.cfg");
	InternalReadOverrides(g_hNewOverrideParser, "configs/admin_overrides.cfg");
}
