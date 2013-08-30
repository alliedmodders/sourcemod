/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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
#ifndef _INCLUDE_SOURCEMOD_NATIVE_OWNER_H_
#define _INCLUDE_SOURCEMOD_NATIVE_OWNER_H_

#include <sp_vm_types.h>
#include <sh_list.h>
#include <am-vector.h>
#include "common_logic.h"

struct NativeEntry;
class CPlugin;

using namespace SourceMod;

struct WeakNative
{
	WeakNative(IPlugin *plugin, uint32_t index) : 
		pl(plugin), idx(index), entry(NULL)
	{
		pl = plugin;
		idx = index;
	}
	WeakNative(IPlugin *plugin, uint32_t index, NativeEntry *pEntry) : 
		pl(plugin), idx(index), entry(pEntry)
	{
		pl = plugin;
		idx = index;
	}
	IPlugin *pl;
	uint32_t idx;
	NativeEntry *entry;
};

using namespace SourceHook;

class CNativeOwner
{
public:
	CNativeOwner();
public:
	virtual void DropEverything();
public:
	void AddNatives(const sp_nativeinfo_t *info);
public:
	void SetMarkSerial(unsigned int serial);
	unsigned int GetMarkSerial();
	void PropagateMarkSerial(unsigned int serial);
public:
	void AddDependent(CPlugin *pPlugin);
	void AddWeakRef(const WeakNative & ref);
	void DropRefsTo(CPlugin *pPlugin);
private:
	void DropWeakRefsTo(CPlugin *pPlugin);
	void UnbindWeakRef(const WeakNative & ref);
protected:
	List<CPlugin *> m_Dependents;
	unsigned int m_nMarkSerial;
	List<WeakNative> m_WeakRefs;
	ke::Vector<const sp_nativeinfo_t *> m_natives;
	List<NativeEntry *> m_Natives;
};

extern CNativeOwner g_CoreNatives;

#endif //_INCLUDE_SOURCEMOD_NATIVE_OWNER_H_
