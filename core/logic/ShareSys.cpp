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

#include <assert.h>

#include <memory>

#include "ShareSys.h"
#include "ExtensionSys.h"
#include <ILibrarySys.h>
#include "common_logic.h"
#include "PluginSys.h"
#include "HandleSys.h"

using namespace ke;

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

	handlesys->InitAccessDefaults(&sec, NULL);
	sec.ident = GetIdentRoot();

	m_TypeRoot = handlesys->CreateType("Identity", this, 0, &sec, NULL, NULL, NULL);
	m_IfaceType = handlesys->CreateType("Interface", this, 0, NULL, NULL, GetIdentRoot(), NULL);

	/* Initialize our static identity handle */
	m_IdentRoot.ident = handlesys->CreateHandle(m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);

	/* Add the Handle System and others... they are too innocent and pure to do it themselves */
	AddInterface(NULL, handlesys);
	AddInterface(NULL, libsys);
}

void ShareSystem::OnSourceModShutdown()
{
	if (m_CoreType)
	{
		handlesys->RemoveType(m_CoreType, GetIdentRoot());
	}

	handlesys->RemoveType(m_IfaceType, GetIdentRoot());
	handlesys->RemoveType(m_TypeRoot, GetIdentRoot());
}

IdentityType_t ShareSystem::FindIdentType(const char *name)
{
	HandleType_t type;

	if (handlesys->FindHandleType(name, &type))
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

	return handlesys->CreateType(name, this, m_TypeRoot, NULL, NULL, GetIdentRoot(), NULL);
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
	SMInterface *iface;
	IExtension *iface_owner = nullptr;
	bool found = false;
	for (auto iter = m_Interfaces.begin(); iter!=m_Interfaces.end(); iter++)
	{
		IfaceInfo &info = *iter;
		iface = info.iface;
		if (strcmp(iface->GetInterfaceName(), iface_name) == 0)
		{
			if (iface->GetInterfaceVersion() == iface_vers || iface->IsVersionCompatible(iface_vers))
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

	handlesys->FreeHandle(identity->ident, &sec);
	delete identity;
}

void ShareSystem::DestroyIdentType(IdentityType_t type)
{
	handlesys->RemoveType(type, GetIdentRoot());
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
	assert(false);
}

RefPtr<Native> ShareSystem::FindNative(const char *name)
{
	NativeCache::Result r = m_NtvCache.find(name);
	if (!r.found())
		return nullptr;
	return *r;
}

void ShareSystem::BeginBindingFor(CPlugin *pPlugin)
{
	g_mark_serial++;
}

void ShareSystem::BindNativesToPlugin(CPlugin *pPlugin, bool bCoreOnly)
{
	IPluginContext *pContext = pPlugin->GetBaseContext();

	BeginBindingFor(pPlugin);

	uint32_t native_count = pContext->GetNativesNum();
	for (uint32_t i = 0; i < native_count; i++)
	{
		const sp_native_t *native = pContext->GetRuntime()->GetNative(i);
		if (!native)
			continue;

		// If we're already bound, no need to do anything else.
		if (native->status == SP_NATIVE_BOUND)
			continue;

		/* Otherwise, the native must be in our cache. */
		RefPtr<Native> pEntry = FindNative(native->name);
		if (!pEntry)
			continue;

		if (bCoreOnly && pEntry->owner != &g_CoreNatives)
			continue;

		BindNativeToPlugin(pPlugin, native, i, pEntry);
	}
}

void ShareSystem::BindNativeToPlugin(CPlugin *pPlugin, const RefPtr<Native> &entry)
{
	if (!entry->owner)
		return;

	IPluginContext *pContext = pPlugin->GetBaseContext();

	uint32_t i;
	if (pContext->FindNativeByName(entry->name(), &i) != SP_ERROR_NONE)
		return;

	const sp_native_t *native = pContext->GetRuntime()->GetNative(i);
	if (!native)
		return;

	if (native->status == SP_NATIVE_BOUND)
		return;

	BindNativeToPlugin(pPlugin, native, i, entry);
}

void ShareSystem::BindNativeToPlugin(CPlugin *pPlugin, const sp_native_t *native, uint32_t index,
                                     const RefPtr<Native> &pEntry)
{
	uint32_t flags = 0;
	if (pEntry->fake)
	{
		/* This native is not necessarily optional, but we don't guarantee
		 * that its address is long-lived. */
		flags |= SP_NTVFLAG_EPHEMERAL;
	}

	/* We don't bother with dependency crap if the owner is Core. */
	if (pEntry->owner != &g_CoreNatives)
	{
		/* The native is optional, this is a special case */
		if ((native->flags & SP_NTVFLAG_OPTIONAL) == SP_NTVFLAG_OPTIONAL)
		{
			flags |= SP_NTVFLAG_OPTIONAL;

			/* Only add if there is a valid owner. */
			if (!pEntry->owner)
				return;
			pEntry->owner->AddWeakRef(WeakNative(pPlugin, index));
		}
		/* Otherwise, we're a strong dependent and not a weak one */
		else
		{
			// If this plugin is not binding to itself, and it hasn't been marked as a
			// dependency already, then add it now. We use the mark serial to track
			// which plugins we already consider a dependency.
			if (pEntry->owner != pPlugin->ToNativeOwner() &&
			    pEntry->owner->GetMarkSerial() != g_mark_serial)
			{
				pEntry->owner->AddDependent(pPlugin);
				pEntry->owner->SetMarkSerial(g_mark_serial);
			}
		}
	}

	auto rt = pPlugin->GetRuntime();
	if (pEntry->fake)
		rt->UpdateNativeBindingObject(index, pEntry->fake->wrapper, flags, nullptr);
	else
		rt->UpdateNativeBinding(index, pEntry->native->func, flags, nullptr);
}

AlreadyRefed<Native> ShareSystem::AddNativeToCache(CNativeOwner *pOwner, const sp_nativeinfo_t *ntv)
{
	NativeCache::Insert i = m_NtvCache.findForAdd(ntv->name);
	if (i.found())
		return nullptr;

	RefPtr<Native> entry = new Native(pOwner, ntv);
	m_NtvCache.insert(ntv->name, entry);
	return entry.forget();
}

void ShareSystem::ClearNativeFromCache(CNativeOwner *pOwner, const char *name)
{
	NativeCache::Result r = m_NtvCache.find(name);
	if (!r.found())
		return;

	RefPtr<Native> entry(*r);
	if (entry->owner != pOwner)
		return;

	// Clear out the owner bit as a sanity measure.
	entry->owner = NULL;

	m_NtvCache.remove(r);
}

class DynamicNative final : public INativeCallback
{
public:
	DynamicNative(SPVM_FAKENATIVE_FUNC callback, void* data)
		: callback_(callback),
		  data_(data)
	{}

	void AddRef() override {
		refcount_++;
	}
	void Release() override {
		assert(refcount_ > 0);
		if (--refcount_ == 0)
			delete this;
	}
	int Invoke(IPluginContext* ctx, const cell_t* params) override {
		return callback_(ctx, params, data_);
	}

private:
	size_t refcount_ = 0;
	SPVM_FAKENATIVE_FUNC callback_;
	void* data_;
};

AlreadyRefed<Native> ShareSystem::AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	RefPtr<Native> entry(FindNative(name));
	if (entry)
		return nullptr;

	std::unique_ptr<FakeNative> fake(new FakeNative(name, pFunc));
	fake->wrapper = new DynamicNative(func, fake.get());

	CNativeOwner *owner = g_PluginSys.GetPluginByCtx(fake->ctx->GetContext());

	entry = new Native(owner, std::move(fake));
	m_NtvCache.insert(name, entry);

	return entry.forget();
}

void ShareSystem::AddCapabilityProvider(IExtension *myself, IFeatureProvider *provider,
		                                const char *name)
{
	if (m_caps.contains(name))
		return;

	Capability cap;
	cap.ext = myself;
	cap.provider = provider;

	m_caps.insert(name, cap);
}

void ShareSystem::DropCapabilityProvider(IExtension *myself, IFeatureProvider *provider,
		                                 const char *name)
{
	StringHashMap<Capability>::Result r = m_caps.find(name);
	if (!r.found())
		return;
	if (r->value.ext != myself || r->value.provider != provider)
		return;

	m_caps.remove(r);
}

FeatureStatus ShareSystem::TestFeature(IPluginRuntime *pRuntime, FeatureType feature, 
                                       const char *name)
{
	switch (feature)
	{
	case FeatureType_Native:
		return TestNative(pRuntime, name);
	case FeatureType_Capability:
		return TestCap(name);
	default:
		break;
	}

	return FeatureStatus_Unknown;
}

FeatureStatus ShareSystem::TestNative(IPluginRuntime *pRuntime, const char *name)
{
	uint32_t index;

	if (pRuntime->FindNativeByName(name, &index) == SP_ERROR_NONE)
	{
		if (const sp_native_t *native = pRuntime->GetNative(index)) {
			if (native->status == SP_NATIVE_BOUND)
				return FeatureStatus_Available;
			else
				return FeatureStatus_Unknown;
		}
	}

	RefPtr<Native> entry = FindNative(name);
	if (!entry)
		return FeatureStatus_Unknown;

	return FeatureStatus_Unavailable;
}

FeatureStatus ShareSystem::TestCap(const char *name)
{
	StringHashMap<Capability>::Result r = m_caps.find(name);
	if (!r.found())
		return FeatureStatus_Unknown;

	return r->value.provider->GetFeatureStatus(FeatureType_Capability, name);
}
