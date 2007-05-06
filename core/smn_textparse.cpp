/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include "TextParsers.h"
#include "HandleSys.h"

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

	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes)
	{
		cell_t result = SMCParse_Continue;

		if (new_section)
		{
			new_section->PushCell(handle);
			new_section->PushString(name);
			new_section->PushCell(opt_quotes ? 1 : 0);
			new_section->Execute(&result);
		}

		return (SMCParseResult)result;
	}

	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes)
	{
		cell_t result = SMCParse_Continue;

		if (key_value)
		{
			key_value->PushCell(handle);
			key_value->PushString(key);
			key_value->PushString(value);
			key_value->PushCell(key_quotes ? 1 : 0);
			key_value->PushCell(value_quotes ? 1 : 0);
			key_value->Execute(&result);
		}

		return (SMCParseResult)result;
	}

	SMCParseResult ReadSMC_LeavingSection()
	{
		cell_t result = SMCParse_Continue;

		if (end_section)
		{
			end_section->PushCell(handle);
			end_section->Execute(&result);
		}

		return (SMCParseResult)result;
	}

	SMCParseResult ReadSMC_RawLine(const char *line, unsigned int curline)
	{
		cell_t result = SMCParse_Continue;

		if (raw_line)
		{
			raw_line->PushCell(handle);
			raw_line->PushString(line);
			raw_line->PushCell(curline);
			raw_line->Execute(&result);
		}

		return (SMCParseResult)result;
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
		g_HandleSys.InitAccessDefaults(NULL, &sec);
		sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
		sec.access[HandleAccess_Read] = 0;

		g_TypeSMC = g_HandleSys.CreateType("SMCParser", this, 0, NULL, &sec, g_pCoreIdent, NULL);
	}

	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_TypeSMC, g_pCoreIdent);
	}

	void OnHandleDestroy(HandleType_t type, void *object)
	{
		ParseInfo *parse = (ParseInfo *)object;
		delete parse;
	}
};

TextParseGlobals g_TextParseGlobals;

static cell_t SMC_CreateParser(IPluginContext *pContext, const cell_t *params)
{
	ParseInfo *pInfo = new ParseInfo();

	Handle_t hndl = g_HandleSys.CreateHandle(g_TypeSMC, pInfo, pContext->GetIdentity(), g_pCoreIdent, NULL);

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
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	ParseInfo *parse;

	if ((err=g_HandleSys.ReadHandle(hndl, g_TypeSMC, NULL, (void **)&parse))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid SMC Parse Handle %x (error %d)", hndl, err);
	}

	parse->parse_start = pContext->GetFunctionById((funcid_t)params[2]);

	return 1;
}

static cell_t SMC_SetParseEnd(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	ParseInfo *parse;

	if ((err=g_HandleSys.ReadHandle(hndl, g_TypeSMC, NULL, (void **)&parse))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid SMC Parse Handle %x (error %d)", hndl, err);
	}

	parse->parse_end = pContext->GetFunctionById((funcid_t)params[2]);

	return 1;
}

static cell_t SMC_SetReaders(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	ParseInfo *parse;

	if ((err=g_HandleSys.ReadHandle(hndl, g_TypeSMC, NULL, (void **)&parse))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid SMC Parse Handle %x (error %d)", hndl, err);
	}

	parse->new_section = pContext->GetFunctionById((funcid_t)params[2]);
	parse->key_value = pContext->GetFunctionById((funcid_t)params[3]);
	parse->end_section = pContext->GetFunctionById((funcid_t)params[4]);

	return 1;
}

static cell_t SMC_SetRawLine(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	ParseInfo *parse;

	if ((err=g_HandleSys.ReadHandle(hndl, g_TypeSMC, NULL, (void **)&parse))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid SMC Parse Handle %x (error %d)", hndl, err);
	}

	parse->raw_line = pContext->GetFunctionById((funcid_t)params[2]);

	return 1;
}

static cell_t SMC_ParseFile(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	ParseInfo *parse;

	if ((err=g_HandleSys.ReadHandle(hndl, g_TypeSMC, NULL, (void **)&parse))
		!= HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid SMC Parse Handle %x (error %d)", hndl, err);
	}

	char *file;
	pContext->LocalToString(params[2], &file);

	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_Game, path, sizeof(path), "%s", file);

	unsigned int line = 0, col = 0;
	SMCParseError p_err = g_TextParser.ParseFile_SMC(path, parse, &line, &col);

	cell_t *c_line, *c_col;
	pContext->LocalToPhysAddr(params[3], &c_line);
	pContext->LocalToPhysAddr(params[4], &c_col);

	*c_line = line;
	*c_col = col;

	return (cell_t)p_err;
}

static cell_t SMC_GetErrorString(IPluginContext *pContext, const cell_t *params)
{
	const char *str = g_TextParser.GetSMCErrorString((SMCParseError)params[1]);

	if (!str)
	{
		return 0;
	}

	pContext->StringToLocal(params[2], params[3], str);

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
	{NULL,							NULL},
};
