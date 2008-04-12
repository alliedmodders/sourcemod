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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_

#include <IExtensionSys.h>
#include <ILibrarySys.h>
#include <sh_list.h>
#include <sh_string.h>
#include <sm_trie_tpl.h>
#include "sm_globals.h"
#include "ShareSys.h"
#include <ISmmAPI.h>
#include <IPluginSys.h>
#include <IRootConsoleMenu.h>
#include "PluginSys.h"

using namespace SourceMod;
using namespace SourceHook;

class CExtension;

struct sm_extnative_t
{
	CExtension *owner;
	const sp_nativeinfo_t *info;
};

/* Replacement native */
struct sm_repnative_t
{
	CExtension *owner;
	sp_nativeinfo_t info;
	SPVM_NATIVE_FUNC original;
};

class CExtension : public IExtension
{
	friend class CExtensionManager;
public:
	virtual ~CExtension();
public: //IExtension
	IExtensionInterface *GetAPI();
	const char *GetFilename();
	IdentityToken_t *GetIdentity();
	ITERATOR *FindFirstDependency(IExtension **pOwner, SMInterface **pInterface);
	bool FindNextDependency(ITERATOR *iter, IExtension **pOwner, SMInterface **pInterface);
	void FreeDependencyIterator(ITERATOR *iter);
	bool IsRunning(char *error, size_t maxlength);
public:
	void SetError(const char *error);
	void AddDependency(IfaceInfo *pInfo);
	void AddChildDependent(CExtension *pOther, SMInterface *iface);
	void AddInterface(SMInterface *pInterface);
	void AddPlugin(IPlugin *pPlugin);
	void RemovePlugin(IPlugin *pPlugin);
	void MarkAllLoaded();
	void AddLibrary(const char *library);
public:
	virtual bool Load(char *error, size_t maxlength);
	virtual bool IsLoaded() =0;
	virtual void Unload() =0;
protected:
	void Initialize(const char *filename, const char *path);
	bool PerformAPICheck(char *error, size_t maxlength);
	void CreateIdentity();
	void DestroyIdentity();
protected:
	IdentityToken_t *m_pIdentToken;
	IExtensionInterface *m_pAPI;
	String m_File;
	String m_Path;
	String m_Error;
	List<IfaceInfo> m_Deps;			/** Dependencies */
	List<IfaceInfo> m_ChildDeps;	/** Children who might depend on us */
	List<SMInterface *> m_Interfaces;
	List<IPlugin *> m_Plugins;
	List<const sp_nativeinfo_t *> m_Natives;
	List<WeakNative> m_WeakNatives;
	List<WeakNative> m_ReplacedNatives;
	List<String> m_Libraries;
	unsigned int unload_code;
	bool m_bFullyLoaded;
};

class CLocalExtension : public CExtension
{
public:
	CLocalExtension(const char *filename);
public:
	bool Load(char *error, size_t maxlength);
	bool IsLoaded();
	void Unload();
	bool IsExternal();
private:
	PluginId m_PlId;
	ILibrary *m_pLib;
};

class CRemoteExtension : public CExtension
{
public:
	CRemoteExtension(IExtensionInterface *pAPI, const char *filename, const char *path);
public:
	bool Load(char *error, size_t maxlength);
	bool IsLoaded();
	void Unload();
	bool IsExternal();
};

class CExtensionManager : 
	public IExtensionManager,
	public SMGlobalClass,
	public IPluginsListener,
	public IRootConsoleCommand
{
public:
	CExtensionManager();
	~CExtensionManager();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IExtensionManager
	IExtension *LoadExtension(const char *path, 
		char *error,
		size_t maxlength);
	bool UnloadExtension(IExtension *pExt);
	IExtension *FindExtensionByFile(const char *file);
	IExtension *FindExtensionByName(const char *ext);
	IExtension *LoadExternal(IExtensionInterface *pInterface,
		const char *filepath,
		const char *filename,
		char *error,
		size_t maxlength);
public: //IPluginsListener
	void OnPluginDestroyed(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const CCommand &command);
public:
	IExtension *LoadAutoExtension(const char *path);
	void BindDependency(IExtension *pOwner, IfaceInfo *pInfo);
	void AddInterface(IExtension *pOwner, SMInterface *pInterface);
	void BindChildPlugin(IExtension *pParent, IPlugin *pPlugin);
	void AddNatives(IExtension *pOwner, const sp_nativeinfo_t *natives);
	void BindAllNativesToPlugin(IPlugin *pPlugin);
	void MarkAllLoaded();
	void AddDependency(IExtension *pSource, const char *file, bool required, bool autoload);
	void TryAutoload();
	void AddLibrary(IExtension *pSource, const char *library);
	bool LibraryExists(const char *library);
	void OverrideNatives(IExtension *myself, const sp_nativeinfo_t *natives);
	void CallOnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax);
public:
	CExtension *GetExtensionFromIdent(IdentityToken_t *ptr);
	void Shutdown();
private:
	CExtension *FindByOrder(unsigned int num);
private:
	List<CExtension *> m_Libs;
	List<sm_repnative_t> m_RepNativeList;
	KTrie<sm_repnative_t> m_RepNatives;
	KTrie<sm_extnative_t> m_ExtNatives;
};

extern CExtensionManager g_Extensions;

#endif //_INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
