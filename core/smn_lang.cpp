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

REGISTER_NATIVES(langNativeS)
{
	{"LoadTranslations",			sm_LoadTranslations},
	{"SetGlobalTransTarget",		sm_SetGlobalTransTarget},
	{NULL,							NULL},
};
