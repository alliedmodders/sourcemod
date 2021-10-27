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
#include "NativeOwner.h"
#include "ShareSys.h"
#include "PluginSys.h"

CNativeOwner::CNativeOwner() : m_nMarkSerial(0)
{
}

void CNativeOwner::SetMarkSerial(unsigned int serial)
{
	m_nMarkSerial = serial;
}

unsigned int CNativeOwner::GetMarkSerial()
{
	return m_nMarkSerial;
}

void CNativeOwner::AddDependent(CPlugin *pPlugin)
{
	if (m_Dependents.find(pPlugin) == m_Dependents.end())
		m_Dependents.push_back(pPlugin);
}

void CNativeOwner::AddWeakRef(const WeakNative & ref)
{
	m_WeakRefs.push_back(ref);
}

void CNativeOwner::AddNatives(const sp_nativeinfo_t *natives)
{
	for (const sp_nativeinfo_t *native = natives; native->func && native->name; native++)
		g_ShareSys.AddNativeToCache(this, native);

	m_natives.push_back(natives);
}

void CNativeOwner::UnbindWeakRef(const WeakNative &ref)
{
	IPluginContext *pContext;

	pContext = ref.pl->GetBaseContext();
	pContext->GetRuntime()->UpdateNativeBinding(
	  ref.idx,
	  nullptr,
	  SP_NTVFLAG_OPTIONAL,
	  nullptr);
}

void CNativeOwner::DropEverything()
{
	List<WeakNative>::iterator iter;

	/* Unbind and remove all weak references to us */
	iter = m_WeakRefs.begin();
	while (iter != m_WeakRefs.end())
	{
		UnbindWeakRef((*iter));
		iter = m_WeakRefs.erase(iter);
	}

	/* Strip all of our natives from the cache */
	for (size_t i = 0; i < m_natives.size(); i++) {
		const sp_nativeinfo_t *natives = m_natives[i];
		for (const sp_nativeinfo_t *native = natives; native->func && native->name; native++)
			g_ShareSys.ClearNativeFromCache(this, native->name);
	}
	m_natives.clear();

	for (size_t i = 0; i < m_fakes.size(); i++)
		g_ShareSys.ClearNativeFromCache(this, m_fakes[i]->name());
	m_fakes.clear();
}

void CNativeOwner::DropWeakRefsTo(CPlugin *pPlugin)
{
	List<WeakNative>::iterator iter;

	iter = m_WeakRefs.begin();
	while (iter != m_WeakRefs.end())
	{
		WeakNative & ref = (*iter);

		if (ref.pl == pPlugin)
		{
			iter = m_WeakRefs.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void CNativeOwner::DropRefsTo(CPlugin *pPlugin)
{
	m_Dependents.remove(pPlugin);
	DropWeakRefsTo(pPlugin);
}
