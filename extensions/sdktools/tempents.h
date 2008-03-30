/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#ifndef _INCLUDE_SOURCEMOD_TEMPENTS_H_
#define _INCLUDE_SOURCEMOD_TEMPENTS_H_

#include "extension.h"
#include <irecipientfilter.h>
#include <sh_list.h>
#include <sh_string.h>
#include <stdio.h>

class TempEntityInfo
{
public:
	TempEntityInfo(const char *name, void *me);
public:
	const char *GetName();
	ServerClass *GetServerClass();
	bool IsValidProp(const char *name);
	bool TE_SetEntData(const char *name, int value);
	bool TE_SetEntDataFloat(const char *name, float value);
	bool TE_SetEntDataVector(const char *name, float vector[3]);
	bool TE_SetEntDataFloatArray(const char *name, cell_t *array, int size);
	bool TE_GetEntData(const char *name, int *value);
	bool TE_GetEntDataFloat(const char *name, float *value);
	bool TE_GetEntDataVector(const char *name, float vector[3]);
	void Send(IRecipientFilter &filter, float delay);
private:
	int _FindOffset(const char *name, int *size=NULL);
private:
	void *m_Me;
	ServerClass *m_Sc;
	SourceHook::String m_Name;
};

class TempEntityManager
{
public:
	TempEntityManager() : m_NameOffs(0), m_NextOffs(0), m_GetClassNameOffs(0), m_Loaded(false) {}
public:
	void Initialize();
	bool IsAvailable();
	void Shutdown();
public:
	TempEntityInfo *GetTempEntityInfo(const char *name);
	const char *GetNameFromThisPtr(void *me);
public:
	void DumpList();
	void DumpProps(FILE *fp);
private:
	SourceHook::List<TempEntityInfo *> m_TEList;
	IBasicTrie *m_TempEntInfo;
	void *m_ListHead;
	int m_NameOffs;
	int m_NextOffs;
	int m_GetClassNameOffs;
	bool m_Loaded;
};

struct TEHookInfo
{
	TempEntityInfo *te;
	SourceHook::List<IPluginFunction *> lst;
};

class TempEntHooks : public IPluginsListener
{
public: //IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public:
	void Initialize();
	void Shutdown();
	bool AddHook(const char *name, IPluginFunction *pFunc);
	bool RemoveHook(const char *name, IPluginFunction *pFunc);
	void OnPlaybackTempEntity(IRecipientFilter &filter, float delay, const void *pSender, const SendTable *pST, int classID);
private:
	void _IncRefCounter();
	void _DecRefCounter();
	size_t _FillInPlayers(int *pl_array, IRecipientFilter *pFilter);
private:
	IBasicTrie *m_TEHooks;
	SourceHook::List<TEHookInfo *> m_HookInfo;
	size_t m_HookCount;
};

extern TempEntityManager g_TEManager;
extern TempEntHooks s_TempEntHooks;

#endif //_INCLUDE_SOURCEMOD_TEMPENTS_H_
