/**
 * vim: set ts=4 sw=4 tw=99 noet :
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
#include <IPluginSys.h>
#include <IPlayerHelpers.h>
#include <sh_string.h>
#include <sh_list.h>
#include "CellArray.h"
#include "AutoHandleRooter.h"

using namespace SourceHook;
using namespace SourceMod;

class PlayerLogicHelpers : 
	public SMGlobalClass,
	public IPluginsListener,
	public ICommandTargetProcessor
{
	struct SimpleMultiTargetFilter
	{
		IPlugin *plugin;
		SourceHook::String pattern;
		IPluginFunction *fun;
		SourceHook::String phrase;
		bool phraseIsML;

		SimpleMultiTargetFilter(IPlugin *plugin, const char *pattern, IPluginFunction *fun,
		                        const char *phrase, bool phraseIsML)
		  : plugin(plugin), pattern(pattern), fun(fun), phrase(phrase), phraseIsML(phraseIsML)
		{
		}
	};

	List<SimpleMultiTargetFilter *> simpleMultis;
	bool filterEnabled;

public:
	void AddMultiTargetFilter(IPlugin *plugin, const char *pattern, IPluginFunction *fun,
	                          const char *phrase, bool phraseIsML)
	{
		SimpleMultiTargetFilter *smtf = new SimpleMultiTargetFilter(plugin, pattern, fun, phrase,
		                                                            phraseIsML);

		simpleMultis.push_back(smtf);

		if (!filterEnabled) {
			playerhelpers->RegisterCommandTargetProcessor(this);
			filterEnabled = true;
		}
	}

	void RemoveMultiTargetFilter(const char *pattern, IPluginFunction *fun)
	{
		List<SimpleMultiTargetFilter *>::iterator iter = simpleMultis.begin();

		while (iter != simpleMultis.end()) {
			if ((*iter)->fun == fun && strcmp((*iter)->pattern.c_str(), pattern) == 0) {
				delete (*iter);
				iter = simpleMultis.erase(iter);
				break;
			}
			iter++;
		}
	}

	PlayerLogicHelpers()
	 : filterEnabled(false)
	{
	}

public: //ICommandTargetProcessor
	bool ProcessCommandTarget(cmd_target_info_t *info)
	{
		List<SimpleMultiTargetFilter *>::iterator iter;

		for (iter = simpleMultis.begin(); iter != simpleMultis.end(); iter++) {
			SimpleMultiTargetFilter *smtf = (*iter);
			if (strcmp(smtf->pattern.c_str(), info->pattern) == 0) {
				CellArray *array = new CellArray(1);
				HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
				Handle_t hndl = handlesys->CreateHandleEx(htCellArray, array, &sec, NULL, NULL);
				AutoHandleCloner ahc(hndl, sec);
				if (ahc.getClone() == BAD_HANDLE) {
					smcore.LogError("[SM] Could not allocate a handle (%s, %d)", __FILE__, __LINE__);
					delete array;
					return false;
				}

				smtf->fun->PushString(info->pattern);
				smtf->fun->PushCell(ahc.getClone());
				cell_t result = 0;
				if (smtf->fun->Execute(&result) != SP_ERROR_NONE || !result)
					return false;

				IGamePlayer *pAdmin = info->admin
				                      ? playerhelpers->GetGamePlayer(info->admin)
				                      : NULL;

				info->num_targets = 0;
				for (size_t i = 0; i < array->size(); i++) {
					cell_t client = *array->at(i);
					IGamePlayer *pClient = playerhelpers->GetGamePlayer(client);
					if (pClient == NULL || !pClient->IsConnected())
						continue;
					if (playerhelpers->FilterCommandTarget(pAdmin, pClient, info->flags) ==
					    COMMAND_TARGET_VALID)
					{
						info->targets[info->num_targets++] = client;
						if (info->num_targets >= unsigned(info->max_targets))
							break;
					}
				}

				info->reason = info->num_targets > 0
				               ? COMMAND_TARGET_VALID
				               : COMMAND_TARGET_EMPTY_FILTER;
				if (info->num_targets) {
					smcore.strncopy(info->target_name, smtf->phrase.c_str(), info->target_name_maxlength);
					info->target_name_style = smtf->phraseIsML
					                          ? COMMAND_TARGETNAME_ML
					                          : COMMAND_TARGETNAME_RAW;
				}

				return true;
			}
		}

		return false;
	}

public: //SMGlobalClass
	void OnSourceModAllInitialized()
	{
		pluginsys->AddPluginsListener(this);
	}

	void OnSourceModShutdown()
	{
		pluginsys->RemovePluginsListener(this);
		if (filterEnabled) {
			playerhelpers->UnregisterCommandTargetProcessor(this);
			filterEnabled = false;
		}
	}

public: //IPluginsListener

	void OnPluginDestroyed(IPlugin *plugin)
	{
		List<SimpleMultiTargetFilter *>::iterator iter = simpleMultis.begin();

		while (iter != simpleMultis.end()) {
			if ((*iter)->plugin != plugin) {
				iter++;
			} else {
				delete (*iter);
				iter = simpleMultis.erase(iter);
			}
		}
	}
} s_PlayerLogicHelpers;

static cell_t
AddMultiTargetFilter(IPluginContext *ctx, const cell_t *params)
{
	IPluginFunction *fun = ctx->GetFunctionById(funcid_t(params[2]));
	if (fun == NULL)
		return ctx->ThrowNativeError("Invalid function id (%X)", params[2]);

	char *pattern;
	char *phrase;

	ctx->LocalToString(params[1], &pattern);
	ctx->LocalToString(params[3], &phrase);

	bool phraseIsML = !!params[4];
	IPlugin *plugin = pluginsys->FindPluginByContext(ctx->GetContext());

	s_PlayerLogicHelpers.AddMultiTargetFilter(plugin, pattern, fun, phrase, phraseIsML);

	return 1;
}

static cell_t
RemoveMultiTargetFilter(IPluginContext *ctx, const cell_t *params)
{
	IPluginFunction *fun = ctx->GetFunctionById(funcid_t(params[2]));
	if (fun == NULL)
		return ctx->ThrowNativeError("Invalid function id (%X)", params[2]);

	char *pattern;

	ctx->LocalToString(params[1], &pattern);

	s_PlayerLogicHelpers.RemoveMultiTargetFilter(pattern, fun);

	return 1;
}

REGISTER_NATIVES(playernatives)
{
	{"AddMultiTargetFilter",	AddMultiTargetFilter},
	{"RemoveMultiTargetFilter",	RemoveMultiTargetFilter},
	{NULL,						NULL}
};

