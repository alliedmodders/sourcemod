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

#ifndef _INCLUDE_SOURCEMOD_SHARESYSTEM_H_
#define _INCLUDE_SOURCEMOD_SHARESYSTEM_H_

#include <IShareSys.h>
#include <IHandleSys.h>
#include <am-string.h>
#include <am-utility.h>
#include <am-refcounting.h>
#include <sh_list.h>
#include <sm_stringhashmap.h>
#include <sm_namehashset.h>
#include "common_logic.h"
#include "Native.h"

using namespace SourceHook;

namespace SourceMod
{
	struct IdentityToken_t
	{
		Handle_t ident = 0;
		void *ptr = nullptr;
		IdentityType_t type = 0;
		size_t num_handles = 0;
		bool warned_handle_usage = false;
	};
};

struct IfaceInfo
{
	bool operator ==(const IfaceInfo &info)
	{
		return (info.iface == iface && info.owner == owner);
	}
	SMInterface *iface;
	IExtension *owner;
};

class CNativeOwner;
class CPlugin;

struct Capability
{
	IExtension *ext;
	IFeatureProvider *provider;
};

class ShareSystem :
	public IShareSys,
	public SMGlobalClass,
	public IHandleTypeDispatch
{
	friend class CNativeOwner;
public:
	ShareSystem();
public: //IShareSys
	bool AddInterface(IExtension *myself, SMInterface *pIface);
	bool RequestInterface(const char *iface_name, 
		unsigned int iface_vers,
		IExtension *myself,
		SMInterface **pIface);
	void AddNatives(IExtension *myself, const sp_nativeinfo_t *natives);
	IdentityType_t CreateIdentType(const char *name);
	IdentityType_t FindIdentType(const char *name);
	IdentityToken_t *CreateIdentity(IdentityType_t type, void *ptr);
	void DestroyIdentType(IdentityType_t type);
	void DestroyIdentity(IdentityToken_t *identity);
	void AddDependency(IExtension *myself, const char *filename, bool require, bool autoload);
	void RegisterLibrary(IExtension *myself, const char *name);
	void OverrideNatives(IExtension *myself, const sp_nativeinfo_t *natives);
	void AddCapabilityProvider(IExtension *myself, IFeatureProvider *provider,
		                       const char *name);
	void DropCapabilityProvider(IExtension *myself, IFeatureProvider *provider,
		                        const char *name);
public: //SMGlobalClass
	/* Pre-empt in case anything tries to register idents early */
	void Initialize();
	void OnSourceModShutdown();
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public:
	IdentityToken_t *CreateCoreIdentity();
	void RemoveInterfaces(IExtension *pExtension);
	FeatureStatus TestFeature(IPluginRuntime *pRuntime, FeatureType feature, const char *name);
	FeatureStatus TestNative(IPluginRuntime *pRuntime, const char *name);
	FeatureStatus TestCap(const char *name);
public:
	inline IdentityToken_t *GetIdentRoot()
	{
		return &m_IdentRoot;
	}
public:
	void BeginBindingFor(CPlugin *pPlugin);
	void BindNativesToPlugin(CPlugin *pPlugin, bool bCoreOnly);
	void BindNativeToPlugin(CPlugin *pPlugin, const ke::RefPtr<Native> &pEntry);
	ke::AlreadyRefed<Native> AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func);
	ke::RefPtr<Native> FindNative(const char *name);
private:
	ke::AlreadyRefed<Native> AddNativeToCache(CNativeOwner *pOwner, const sp_nativeinfo_t *ntv);
	void ClearNativeFromCache(CNativeOwner *pOwner, const char *name);
	void BindNativeToPlugin(CPlugin *pPlugin, const sp_native_t *ntv,  uint32_t index, const ke::RefPtr<Native> &pEntry);
private:
	typedef NameHashSet<ke::RefPtr<Native>, Native> NativeCache;

	List<IfaceInfo> m_Interfaces;
	HandleType_t m_TypeRoot;
	IdentityToken_t m_IdentRoot;
	HandleType_t m_IfaceType;
	IdentityType_t m_CoreType;
	NativeCache m_NtvCache;
	StringHashMap<Capability> m_caps;
};

extern ShareSystem g_ShareSys;

#endif //_INCLUDE_SOURCEMOD_SHARESYSTEM_H_
