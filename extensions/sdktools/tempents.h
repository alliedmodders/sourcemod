/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
