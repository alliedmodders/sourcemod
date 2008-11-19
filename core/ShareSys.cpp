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

#include "ShareSys.h"
#include "HandleSys.h"
#include "ExtensionSys.h"
#include "LibrarySys.h"
#include "PluginSys.h"
#include "sm_stringutil.h"

ShareSystem g_ShareSys;
static unsigned int g_mark_serial = 0;

ShareSystem::ShareSystem()
{
	m_IdentRoot.ident = 0;
	m_TypeRoot = 0;
	m_IfaceType = 0;
	m_CoreType = 0;
}

IdentityToken_t *ShareSystem::CreateCoreIdentity()
{
	if (!m_CoreType)
	{
		m_CoreType = CreateIdentType("CORE");
	}

	return CreateIdentity(m_CoreType, this);
}

void ShareSystem::Initialize()
{
	TypeAccess sec;

	g_HandleSys.InitAccessDefaults(&sec, NULL);
	sec.ident = GetIdentRoot();

	m_TypeRoot = g_HandleSys.CreateType("Identity", this, 0, &sec, NULL, NULL, NULL);
	m_IfaceType = g_HandleSys.CreateType("Interface", this, 0, NULL, NULL, GetIdentRoot(), NULL);

	/* Initialize our static identity handle */
	m_IdentRoot.ident = g_HandleSys.CreateHandle(m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);

	/* Add the Handle System and others... they are too innocent and pure to do it themselves */
	AddInterface(NULL, &g_HandleSys);
	AddInterface(NULL, &g_LibSys);
}

void ShareSystem::OnSourceModShutdown()
{
	if (m_CoreType)
	{
		g_HandleSys.RemoveType(m_CoreType, GetIdentRoot());
	}

	g_HandleSys.RemoveType(m_IfaceType, GetIdentRoot());
	g_HandleSys.RemoveType(m_TypeRoot, GetIdentRoot());
}

IdentityType_t ShareSystem::FindIdentType(const char *name)
{
	HandleType_t type;

	if (g_HandleSys.FindHandleType(name, &type))
	{
		if (g_HandleSys.TypeCheck(type, m_TypeRoot))
		{
			return type;
		}
	}

	return 0;
}

IdentityType_t ShareSystem::CreateIdentType(const char *name)
{
	if (!m_TypeRoot)
	{
		return 0;
	}

	return g_HandleSys.CreateType(name, this, m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);
}

void ShareSystem::OnHandleDestroy(HandleType_t type, void *object)
{
	/* THIS WILL NEVER BE CALLED FOR ANYTHING WITH THE IDENTITY TYPE */
}

IdentityToken_t *ShareSystem::CreateIdentity(IdentityType_t type, void *ptr)
{
	if (!m_TypeRoot)
	{
		return 0;
	}

	/* :TODO: Cache? */
	IdentityToken_t *pToken = new IdentityToken_t;

	HandleSecurity sec;
	sec.pOwner = sec.pIdentity = GetIdentRoot();

	pToken->ident = g_HandleSys.CreateHandleInt(type, NULL, &sec, NULL, NULL, true);
	pToken->ptr = ptr;
	pToken->type = type;

	return pToken;
}

bool ShareSystem::AddInterface(IExtension *myself, SMInterface *iface)
{
	if (!iface)
	{
		return false;
	}

	IfaceInfo info;

	info.owner = myself;
	info.iface = iface;
	
	m_Interfaces.push_back(info);

	return true;
}

bool ShareSystem::RequestInterface(const char *iface_name, 
								   unsigned int iface_vers, 
								   IExtension *myself, 
								   SMInterface **pIface)
{
	/* See if the interface exists */
	List<IfaceInfo>::iterator iter;
	SMInterface *iface;
	IExtension *iface_owner;
	bool found = false;
	for (iter=m_Interfaces.begin(); iter!=m_Interfaces.end(); iter++)
	{
		IfaceInfo &info = (*iter);
		iface = info.iface;
		if (strcmp(iface->GetInterfaceName(), iface_name) == 0)
		{
			if (iface->GetInterfaceVersion() == iface_vers
				|| iface->IsVersionCompatible(iface_vers))
			{
				iface_owner = info.owner;
				found = true;
				break;
			}
		}
	}

	if (!found)
	{
		return false;
	}

	/* Add a dependency node */
	if (iface_owner)
	{
		IfaceInfo info;
		info.iface = iface;
		info.owner = iface_owner;
		g_Extensions.BindDependency(myself, &info);
	}

	if (pIface)
	{
		*pIface = iface;
	}

	return true;
}

void ShareSystem::AddNatives(IExtension *myself, const sp_nativeinfo_t *natives)
{
	CNativeOwner *pOwner;

	pOwner = g_Extensions.GetNativeOwner(myself);

	pOwner->AddNatives(natives);
}

void ShareSystem::DestroyIdentity(IdentityToken_t *identity)
{
	HandleSecurity sec;

	sec.pOwner = GetIdentRoot();
	sec.pIdentity = GetIdentRoot();

	g_HandleSys.FreeHandle(identity->ident, &sec);
	delete identity;
}

void ShareSystem::DestroyIdentType(IdentityType_t type)
{
	g_HandleSys.RemoveType(type, GetIdentRoot());
}

void ShareSystem::RemoveInterfaces(IExtension *pExtension)
{
	List<IfaceInfo>::iterator iter = m_Interfaces.begin();

	while (iter != m_Interfaces.end())
	{
		if ((*iter).owner == pExtension)
		{
			iter = m_Interfaces.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void ShareSystem::AddDependency(IExtension *myself, const char *filename, bool require, bool autoload)
{
	g_Extensions.AddDependency(myself, filename, require, autoload);
}

void ShareSystem::RegisterLibrary(IExtension *myself, const char *name)
{
	g_Extensions.AddLibrary(myself, name);
}

void ShareSystem::OverrideNatives(IExtension *myself, const sp_nativeinfo_t *natives)
{
	unsigned int i;
	NativeEntry *pEntry;
	CNativeOwner *pOwner;

	pOwner = g_Extensions.GetNativeOwner(myself);

	for (i = 0; natives[i].func != NULL && natives[i].name != NULL; i++)
	{
		if ((pEntry = FindNative(natives[i].name)) == NULL)
		{
			continue;
		}

		if (pEntry->owner != g_pCoreNatives)
		{
			continue;
		}

		if (pEntry->replacement.owner != NULL)
		{
			continue;
		}

		/* Now it's safe to add the override */
		pEntry->replacement.func = natives[i].func;
		pEntry->replacement.owner = pOwner;
		pOwner->AddReplacedNative(pEntry);
	}
}

NativeEntry *ShareSystem::FindNative(const char *name)
{
	NativeEntry **ppEntry;

	if ((ppEntry = m_NtvCache.retrieve(name)) == NULL)
	{
		return NULL;
	}

	return *ppEntry;
}

void ShareSystem::BindNativesToPlugin(CPlugin *pPlugin, bool bCoreOnly)
{
	NativeEntry *pEntry;
	sp_native_t *native;
	uint32_t i, native_count;
	IPluginContext *pContext;

	pContext = pPlugin->GetBaseContext();

	/* Generate a new serial ID, mark our dependencies with it. */
	g_mark_serial++;
	pPlugin->PropogateMarkSerial(g_mark_serial);

	native_count = pContext->GetNativesNum();
	for (i = 0; i < native_count; i++)
	{
		if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
		{
			continue;
		}

		/* If we're bound, check if there is a replacement available.  
		 * If not, this native is totally finalized.
		 */
		if (native->status == SP_NATIVE_BOUND)
		{
			pEntry = (NativeEntry *)native->user;
			assert(pEntry != NULL);
			if (pEntry->replacement.owner == NULL
				|| (pEntry->replacement.owner != NULL 
				&&  pEntry->replacement.func == native->pfn))
			{
				continue;
			}
		}
		/* Otherwise, the native must be in our cache. */
		else if ((pEntry = FindNative(native->name)) == NULL)
		{
			continue;
		}

		if (bCoreOnly && pEntry->owner != g_pCoreNatives)
		{
			continue;
		}

		BindNativeToPlugin(pPlugin, native, i, pEntry);
	}
}

void ShareSystem::BindNativeToPlugin(CPlugin *pPlugin, NativeEntry *pEntry)
{
	uint32_t i;
	sp_native_t *native;
	IPluginContext *pContext;

	pContext = pPlugin->GetBaseContext();

	if (pContext->FindNativeByName(pEntry->name, &i) != SP_ERROR_NONE)
	{
		return;
	}
	if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
	{
		return;
	}

	if (native->status == SP_NATIVE_BOUND)
	{
		return;
	}

	BindNativeToPlugin(pPlugin, native, i, pEntry);
}

void ShareSystem::BindNativeToPlugin(CPlugin *pPlugin, 
									 sp_native_t *native,
									 uint32_t index,
									 NativeEntry *pEntry)
{
	/* Mark as bound... we do the rest next. */
	native->status = SP_NATIVE_BOUND;
	native->user = pEntry;

	/* See if a replacement is available. */
	if (pEntry->replacement.owner != NULL)
	{
		/* Perform a replacement bind. */
		native->pfn = pEntry->replacement.func;
		pEntry->replacement.owner->AddWeakRef(WeakNative(pPlugin, index, pEntry));
	}
	else
	{
		/* Perform a normal bind. */
		native->pfn = pEntry->func;

		/* We don't bother with dependency crap if the owner is Core. */
		if (pEntry->owner != g_pCoreNatives)
		{
			/* The native is optional, this is a special case */
			if ((native->flags & SP_NTVFLAG_OPTIONAL) == SP_NTVFLAG_OPTIONAL)
			{
				/* Only add if there is a valid owner. */
				if (pEntry->owner != NULL)
				{
					pEntry->owner->AddWeakRef(WeakNative(pPlugin, index));
				}
				else
				{
					native->status = SP_NATIVE_UNBOUND;
				}
			}
			/* Otherwise, we're a strong dependent and not a weak one */
			else
			{
				/* See if this has already been marked as a dependent.
				 * If it has, it means this relationship has already occurred, 
				 * and there is no reason to do it again.
				 */
				if (pEntry->owner != pPlugin 
					&& pEntry->owner->GetMarkSerial() != g_mark_serial)
				{
					/* This has not been marked as a dependency yet */
					//pPlugin->AddDependency(pEntry->owner);
					pEntry->owner->AddDependent(pPlugin);
					pEntry->owner->SetMarkSerial(g_mark_serial);
				}
			}
		}
	}
}

NativeEntry *ShareSystem::AddNativeToCache(CNativeOwner *pOwner, const sp_nativeinfo_t *ntv)
{
	NativeEntry *pEntry;

	if ((pEntry = FindNative(ntv->name)) == NULL)
	{
		pEntry = new NativeEntry;

		pEntry->owner = pOwner;
		pEntry->name = ntv->name;
		pEntry->func = ntv->func;
		pEntry->replacement.func = NULL;
		pEntry->replacement.owner = NULL;
		pEntry->fake = NULL;

		m_NtvCache.insert(ntv->name, pEntry);

		return pEntry;
	}

	if (pEntry->owner != NULL)
	{
		return NULL;
	}

	pEntry->owner = pOwner;
	pEntry->func = ntv->func;
	pEntry->name = ntv->name;

	return pEntry;
}

void ShareSystem::ClearNativeFromCache(CNativeOwner *pOwner, const char *name)
{
	NativeEntry *pEntry;

	if ((pEntry = FindNative(name)) == NULL)
	{
		return;
	}

	if (pEntry->owner != pOwner)
	{
		return;
	}

	if (pEntry->fake != NULL)
	{
		g_pSourcePawn2->DestroyFakeNative(pEntry->func);
		delete pEntry->fake;
		pEntry->fake = NULL;
	}

	pEntry->func = NULL;
	pEntry->name = NULL;
	pEntry->owner = NULL;
	pEntry->replacement.func = NULL;
	pEntry->replacement.owner = NULL;
}

NativeEntry *ShareSystem::AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	FakeNative *pFake;
	NativeEntry *pEntry;
	SPVM_NATIVE_FUNC gate;

	if ((pEntry = FindNative(name)) != NULL && pEntry->owner != NULL)
	{
		return NULL;
	}

	pFake = new FakeNative;

	if ((gate = g_pSourcePawn2->CreateFakeNative(func, pFake)) == NULL)
	{
		delete pFake;
		return NULL;
	}

	if (pEntry == NULL)
	{
		pEntry = new NativeEntry;
		m_NtvCache.insert(name, pEntry);
	}

	pFake->call = pFunc;
	pFake->ctx = pFunc->GetParentContext();
	strncopy(pFake->name, name, sizeof(pFake->name));
	
	pEntry->fake = pFake;
	pEntry->func = gate;
	pEntry->name = pFake->name;
	pEntry->owner = g_PluginSys.GetPluginByCtx(pFake->ctx->GetContext());
	pEntry->replacement.func = NULL;
	pEntry->replacement.owner = NULL;

	return pEntry;
}
