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

#ifndef _INCLUDE_SOURCEMOD_SHARESYSTEM_H_
#define _INCLUDE_SOURCEMOD_SHARESYSTEM_H_

#include <IShareSys.h>
#include <IHandleSys.h>
#include <sh_list.h>
#include <sm_trie_tpl.h>
#include "sm_globals.h"
#include "sourcemod.h"

using namespace SourceHook;

namespace SourceMod
{
	struct IdentityToken_t
	{
		Handle_t ident;
		void *ptr;
		IdentityType_t type;
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
struct NativeEntry;

struct ReplaceNative
{
	CNativeOwner *owner;
	SPVM_NATIVE_FUNC func;
};

struct FakeNative
{
	char name[64];
	IPluginContext *ctx;
	IPluginFunction *call;
};

struct NativeEntry
{
	CNativeOwner *owner;
	SPVM_NATIVE_FUNC func;
	const char *name;
	ReplaceNative replacement;
	FakeNative *fake;
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
public: //SMGlobalClass
	/* Pre-empt in case anything tries to register idents early */
	void Initialize();
	void OnSourceModShutdown();
public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public:
	IdentityToken_t *CreateCoreIdentity();
	void RemoveInterfaces(IExtension *pExtension);
public:
	inline IdentityToken_t *GetIdentRoot()
	{
		return &m_IdentRoot;
	}
public:
	void BindNativesToPlugin(CPlugin *pPlugin, bool bCoreOnly);
	void BindNativeToPlugin(CPlugin *pPlugin, NativeEntry *pEntry);
	NativeEntry *AddFakeNative(IPluginFunction *pFunc, const char *name, SPVM_FAKENATIVE_FUNC func);
	NativeEntry *FindNative(const char *name);
private:
	NativeEntry *AddNativeToCache(CNativeOwner *pOwner, const sp_nativeinfo_t *ntv);
	void ClearNativeFromCache(CNativeOwner *pOwner, const char *name);
	void BindNativeToPlugin(CPlugin *pPlugin, 
		sp_native_t *ntv, 
		uint32_t index, 
		NativeEntry *pEntry);
private:
	List<IfaceInfo> m_Interfaces;
	HandleType_t m_TypeRoot;
	IdentityToken_t m_IdentRoot;
	HandleType_t m_IfaceType;
	IdentityType_t m_CoreType;
	KTrie<NativeEntry *> m_NtvCache;
};

extern ShareSystem g_ShareSys;

#endif //_INCLUDE_SOURCEMOD_SHARESYSTEM_H_
