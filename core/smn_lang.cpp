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

#include "PluginSys.h"
#include "Translator.h"
#include "LibrarySys.h"
#include "sm_stringutil.h"
#include "PlayerManager.h"

static cell_t sm_LoadTranslations(IPluginContext *pCtx, const cell_t *params)
{
	char *filename;
	unsigned int index;
	CPlugin *pl = (CPlugin *)g_PluginSys.FindPluginByContext(pCtx->GetContext());

	pCtx->LocalToString(params[1], &filename);

	/* Check if there is no extension */
	const char *ext = g_LibSys.GetFileExtension(filename);
	if (!ext || (strcmp(ext, "cfg") && strcmp(ext, "txt")))
	{
		/* Append one */
		static char new_file[PLATFORM_MAX_PATH];
		UTIL_Format(new_file, sizeof(new_file), "%s.txt", filename);
		filename = new_file;
	}

	index = g_Translator.FindOrAddPhraseFile(filename);
	pl->AddLangFile(index);

	return 1;
}

static cell_t sm_SetGlobalTransTarget(IPluginContext *pContext, const cell_t *params)
{
	g_SourceMod.SetGlobalTarget(params[1]);

	return 1;
}

static cell_t sm_GetClientLanguage(IPluginContext *pContext, const cell_t *params)
{
	CPlayer *player = g_Players.GetPlayerByIndex(params[1]);
	if (!player || !player->IsConnected())
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}

	return g_Translator.GetClientLanguage(params[1]);
}

static cell_t sm_GetServerLanguage(IPluginContext *pContext, const cell_t *params)
{
	return g_Translator.GetServerLanguage();
}

static cell_t sm_GetLanguageCount(IPluginContext *pContext, const cell_t *params)
{
	return g_Translator.GetLanguageCount();
}

static cell_t sm_GetLanguageInfo(IPluginContext *pContext, const cell_t *params)
{
	const char *code;
	const char *name;

	if (!g_Translator.GetLanguageInfo(params[1], &code, &name))
	{
		return pContext->ThrowNativeError("Invalid language number %d", params[1]);
	}

	pContext->StringToLocalUTF8(params[2], params[3], code, NULL);
	pContext->StringToLocalUTF8(params[4], params[5], name, NULL);

	return 1;
}

REGISTER_NATIVES(langNatives)
{
	{"LoadTranslations",			sm_LoadTranslations},
	{"SetGlobalTransTarget",		sm_SetGlobalTransTarget},
	{"GetClientLanguage",			sm_GetClientLanguage},
	{"GetServerLanguage",			sm_GetServerLanguage},
	{"GetLanguageCount",			sm_GetLanguageCount},
	{"GetLanguageInfo",				sm_GetLanguageInfo},
	{NULL,							NULL},
};
