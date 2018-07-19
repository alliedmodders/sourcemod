/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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
#include "Translator.h"
#include <IPlayerHelpers.h>
#include <IPluginSys.h>
#include <ISourceMod.h>
#include <am-string.h>

static cell_t sm_TranslationPhraseExists(IPluginContext *pCtx, const cell_t *params)
{
	IPlugin *pl = pluginsys->FindPluginByContext(pCtx->GetContext());
	IPhraseCollection *collection = pl->GetPhrases();
	
	char *phrase;
	pCtx->LocalToString(params[1], &phrase);

	return collection->TranslationPhraseExists(phrase);
}

static cell_t sm_IsTranslatedForLanguage(IPluginContext *pCtx, const cell_t *params)
{
	IPlugin *pl = pluginsys->FindPluginByContext(pCtx->GetContext());
	IPhraseCollection *collection = pl->GetPhrases();
	
	char *phrase;
	pCtx->LocalToString(params[1], &phrase);
	
	int langid = params[2];

	Translation trans;
	return (collection->FindTranslation(phrase, langid, &trans) == Trans_Okay);
}

static cell_t sm_LoadTranslations(IPluginContext *pCtx, const cell_t *params)
{
	char *filename, *ext;
	char buffer[PLATFORM_MAX_PATH];
	IPlugin *pl = pluginsys->FindPluginByContext(pCtx->GetContext());

	pCtx->LocalToString(params[1], &filename);
	ke::SafeStrcpy(buffer, sizeof(buffer), filename);

	/* Make sure there is no extension */
	if ((ext = strstr(buffer, ".txt")) != NULL
		|| (ext = strstr(buffer, ".cfg")) != NULL)
	{
		/* Simple heuristic -- just see if it's at the end and terminate if so */
		if ((unsigned)(ext - buffer) == strlen(buffer) - 4)
		{
			*ext = '\0';
		}
	}

	pl->GetPhrases()->AddPhraseFile(buffer);

	return 1;
}

static cell_t sm_SetGlobalTransTarget(IPluginContext *pContext, const cell_t *params)
{
	g_pSM->SetGlobalTarget(params[1]);

	return 1;
}

static cell_t sm_GetClientLanguage(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
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

static cell_t sm_SetClientLanguage(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *player = playerhelpers->GetGamePlayer(params[1]);
	
	if (!player || !player->IsConnected())
	{
		return pContext->ThrowNativeError("Invalid client index %d", params[1]);
	}
	
	player->SetLanguageId(params[2]);

	return 1;
}

static cell_t sm_GetLanguageByCode(IPluginContext *pContext, const cell_t *params)
{
	char *code;
	unsigned int langid;

	pContext->LocalToString(params[1], &code);

	if (g_Translator.GetLanguageByCode(code, &langid))
		return langid;

	return -1;
}

static cell_t sm_GetLanguageByName(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	unsigned int langid;

	pContext->LocalToString(params[1], &name);

	if (g_Translator.GetLanguageByName(name, &langid))
		return langid;

	return -1;
}

REGISTER_NATIVES(langNatives)
{
	{"IsTranslatedForLanguage",		sm_IsTranslatedForLanguage},
	{"TranslationPhraseExists",		sm_TranslationPhraseExists},
	{"LoadTranslations",			sm_LoadTranslations},
	{"SetGlobalTransTarget",		sm_SetGlobalTransTarget},
	{"GetClientLanguage",			sm_GetClientLanguage},
	{"GetServerLanguage",			sm_GetServerLanguage},
	{"GetLanguageCount",			sm_GetLanguageCount},
	{"GetLanguageInfo",				sm_GetLanguageInfo},
	{"SetClientLanguage",			sm_SetClientLanguage},
	{"GetLanguageByCode",			sm_GetLanguageByCode},
	{"GetLanguageByName",			sm_GetLanguageByName},
	{NULL,							NULL},
};
