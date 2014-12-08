/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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

#include "common_logic.h"
#include <ITextParsers.h>
#include <ISourceMod.h>
#include "handle_helpers.h"

HandleType_t g_TypeSMC = 0;

class ParseInfo : public ITextListener_SMC
{
public:
	ParseInfo()
	{
		parse_start = NULL;
		parse_end = NULL;
		new_section = NULL;
		key_value = NULL;
		end_section = NULL;
		raw_line = NULL;
		handle = 0;
	}
public:
	void ReadSMC_ParseStart()
	{
		if (parse_start)
		{
			cell_t result;
			parse_start->PushCell(handle);
			parse_start->Execute(&result);
		}
	}

	void ReadSMC_ParseEnd(bool halted, bool failed)
	{
		if (parse_end)
		{
			cell_t result;
			parse_end->PushCell(handle);
			parse_end->PushCell(halted ? 1 : 0);
			parse_end->PushCell(failed ? 1 : 0);
			parse_end->Execute(&result);
		}
	}

	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		cell_t result = SMCResult_Continue;

		if (new_section)
		{
			new_section->PushCell(handle);
			new_section->PushString(name);
			new_section->PushCell(1);
			new_section->Execute(&result);
		}

		return (SMCResult)result;
	}

	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		cell_t result = SMCResult_Continue;

		if (key_value)
		{
			key_value->PushCell(handle);
			key_value->PushString(key);
			key_value->PushString(value);
			key_value->PushCell(1);
			key_value->PushCell(1);
			key_value->Execute(&result);
		}

		return (SMCResult)result;
	}

	SMCResult ReadSMC_LeavingSection(const SMCStates *states)
	{
		cell_t result = SMCResult_Continue;

		if (end_section)
		{
			end_section->PushCell(handle);
			end_section->Execute(&result);
		}

		return (SMCResult)result;
	}

	SMCResult ReadSMC_RawLine(const SMCStates *states, const char *line)
	{
		cell_t result = SMCResult_Continue;

		if (raw_line)
		{
			raw_line->PushCell(handle);
			raw_line->PushString(line);
			raw_line->PushCell(states->line);
			raw_line->Execute(&result);
		}

		return (SMCResult)result;
	}
public:
	IPluginFunction *parse_start;
	IPluginFunction *parse_end;
	IPluginFunction *new_section;
	IPluginFunction *key_value;
	IPluginFunction *end_section;
	IPluginFunction *raw_line;
	Handle_t handle;
};

class TextParseGlobals : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		HandleAccess sec;

		/* These cannot be cloned, because they are locked to a specific plugin.
		 * However, we let anyone read them because we don't care.
		 */
		handlesys->InitAccessDefaults(NULL, &sec);
		sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
		sec.access[HandleAccess_Read] = 0;

		g_TypeSMC = handlesys->CreateType("SMCParser", this, 0, NULL, &sec, g_pCoreIdent, NULL);
	}

	void OnSourceModShutdown()
	{
		handlesys->RemoveType(g_TypeSMC, g_pCoreIdent);
	}

	void OnHandleDestroy(HandleType_t type, void *object)
	{
		ParseInfo *parse = (ParseInfo *)object;
		delete parse;
	}

	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
	{
		*pSize = sizeof(ParseInfo);
		return true;
	}
};

TextParseGlobals g_TextParseGlobals;

static cell_t SMC_CreateParser(IPluginContext *pContext, const cell_t *params)
{
	ParseInfo *pInfo = new ParseInfo();

	Handle_t hndl = handlesys->CreateHandle(g_TypeSMC, pInfo, pContext->GetIdentity(), g_pCoreIdent, NULL);

	/* Should never happen */
	if (!hndl)
	{
		delete pInfo;
		return 0;
	}

	pInfo->handle = hndl;

	return hndl;
}

static cell_t SMC_SetParseStart(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->parse_start = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMC_SetParseEnd(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->parse_end = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMC_SetReaders(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->new_section = pContext->GetFunctionById((funcid_t)params[2]);
	parse->key_value = pContext->GetFunctionById((funcid_t)params[3]);
	parse->end_section = pContext->GetFunctionById((funcid_t)params[4]);
	return 1;
}

static cell_t SMC_SetRawLine(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->raw_line = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMC_ParseFile(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	char *file;
	pContext->LocalToString(params[2], &file);

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	SMCStates states;
	SMCError p_err = textparsers->ParseFile_SMC(path, parse, &states);

	cell_t *c_line, *c_col;
	pContext->LocalToPhysAddr(params[3], &c_line);
	pContext->LocalToPhysAddr(params[4], &c_col);

	*c_line = states.line;
	*c_col = states.col;
	return (cell_t)p_err;
}

static cell_t SMC_GetErrorString(IPluginContext *pContext, const cell_t *params)
{
	const char *str = textparsers->GetSMCErrorString((SMCError)params[1]);
	if (!str)
		return 0;

	pContext->StringToLocal(params[2], params[3], str);
	return 1;
}

static cell_t SMCParser_OnEnterSection_set(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->new_section = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMCParser_OnLeaveSection_set(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->end_section = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMCParser_OnKeyValue_set(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	parse->key_value = pContext->GetFunctionById((funcid_t)params[2]);
	return 1;
}

static cell_t SMCParser_GetErrorString(IPluginContext *pContext, const cell_t *params)
{
	OpenHandle<ParseInfo> parse(pContext, params[1], g_TypeSMC);
	if (!parse.Ok())
		return 0;

	const char *str = "no error";
	if (params[2]) {
		str = textparsers->GetSMCErrorString((SMCError)params[2]);
		if (!str)
			str = "unknown error";
	}

	pContext->StringToLocal(params[3], params[4], str);
	return 1;
}

REGISTER_NATIVES(textNatives)
{
	{"SMC_CreateParser",			SMC_CreateParser},
	{"SMC_ParseFile",				SMC_ParseFile},
	{"SMC_GetErrorString",			SMC_GetErrorString},
	{"SMC_SetParseStart",			SMC_SetParseStart},
	{"SMC_SetParseEnd",				SMC_SetParseEnd},
	{"SMC_SetReaders",				SMC_SetReaders},
	{"SMC_SetRawLine",				SMC_SetRawLine},

	// Transitional syntax support.
	{"SMCParser.SMCParser",				SMC_CreateParser},
	{"SMCParser.ParseFile",				SMC_ParseFile},
	{"SMCParser.OnStart.set",			SMC_SetParseStart},
	{"SMCParser.OnEnd.set",				SMC_SetParseEnd},
	{"SMCParser.OnEnterSection.set",	SMCParser_OnEnterSection_set},
	{"SMCParser.OnLeaveSection.set",	SMCParser_OnLeaveSection_set},
	{"SMCParser.OnKeyValue.set",		SMCParser_OnKeyValue_set},
	{"SMCParser.OnRawLine.set",			SMC_SetRawLine},
	{"SMCParser.GetErrorString",		SMCParser_GetErrorString},

	{NULL,							NULL},
};
