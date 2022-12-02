/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_OUTPUT_H_
#define _INCLUDE_SOURCEMOD_OUTPUT_H_

#include <jit/jit_helpers.h>
#include <jit/x86/x86_macros.h>
#include "sh_list.h"
#include "sh_stack.h"
#include "sm_trie_tpl.h"
#include "CDetour/detours.h"

extern ISourcePawnEngine *spengine;

struct OutputNameStruct;

/**
 * This is a function specific hook that corresponds to an entity classname
 * and outputname. There can be many of these for each classname/output combo
 */
struct omg_hooks 
{
	cell_t entity_ref;
	bool only_once;
	IPluginFunction *pf;
	OutputNameStruct *m_parent;
	bool in_use;
	bool delete_me;
};

/**
 * This represents an output belonging to a specific classname
 */
struct OutputNameStruct
{
	SourceHook::List<omg_hooks *> hooks;
	char Name[50];
};

/** 
 * This represents an entity classname
 */
struct ClassNameStruct
{
	//Trie mapping outputname to a OutputNameStruct
	//KTrie<OutputNameStruct *> OutputList;
	IBasicTrie *OutputList;

	ClassNameStruct()
	{
		OutputList = adtfactory->CreateBasicTrie();
	}

	~ClassNameStruct()
	{
		OutputList->Destroy();
	}
};

class EntityOutputManager : public IPluginsListener
{
public:
	EntityOutputManager();
public:
	void Init();
	void Shutdown();
	bool IsEnabled();

	bool FireEventDetour(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);

	void OnPluginDestroyed(IPlugin *plugin);

	OutputNameStruct *FindOutputPointer(const char *classname, const char *outputname, bool create);

	void CleanUpHook(omg_hooks *hook);

	omg_hooks *NewHook();

	void OnHookAdded();
	void OnHookRemoved();

private:
	bool enabled;

	// Patch/unpatch the server dll
	void InitFireEventDetour();
	void ShutdownFireEventDetour();

	//These create/delete the allocated memory and write into it
	bool CreateFireEventDetour();
	void DeleteFireEventDetour();

	const char *FindOutputName(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, const char **entity_classname);

	// Maps classname to a ClassNameStruct
	IBasicTrie *ClassNames;

	SourceHook::CStack<omg_hooks *> FreeHooks; //Stores hook pointers to avoid calls to new

	int HookCount;

	patch_t info_restore;
	void *info_address;
	void *info_callback;
};

void TempDetour(void *pOutput, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);

extern EntityOutputManager g_OutputManager;

extern sp_nativeinfo_t g_EntOutputNatives[];

#endif //_INCLUDE_SOURCEMOD_OUTPUT_H_
