/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
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

using namespace SourceHook;

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
	void Send(IRecipientFilter &filter, float delay);
private:
	int _FindOffset(const char *name, int *size=NULL);
private:
	void *m_Me;
	ServerClass *m_Sc;
	String m_Name;
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
public:
	void DumpList();
	void DumpProps(FILE *fp);
private:
	List<TempEntityInfo *> m_TEList;
	IBasicTrie *m_TempEntInfo;
	void *m_ListHead;
	int m_NameOffs;
	int m_NextOffs;
	int m_GetClassNameOffs;
	bool m_Loaded;
};

extern TempEntityManager g_TEManager;

#endif //_INCLUDE_SOURCEMOD_TEMPENTS_H_
