// vim: set ts=4 sw=4 tw=99 noet:
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_ISCRIPTMANAGER_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_ISCRIPTMANAGER_H_

#include <sp_vm_api.h>
#include <IPluginSys.h>
#include <sh_vector.h>
#include <sh_string.h>

namespace SourceMod {

using namespace SourceHook;
using namespace SourcePawn;

class IChangeableForward;

enum LibraryAction
{
	LibraryAction_Removed,
	LibraryAction_Added
};

struct AutoConfig
{
	SourceHook::String autocfg;
	SourceHook::String folder;
	bool create;
};

class SMPlugin : public IPlugin
{
public:
	virtual size_t GetConfigCount() = 0;
	virtual AutoConfig *GetConfig(size_t i) = 0;
	virtual void AddLibrary(const char *name) = 0;
	virtual void AddConfig(bool create, const char *cfg, const char *folder) = 0;
	virtual void EvictWithError(PluginStatus status, const char *fmt, ...) = 0;
};

class IScriptManager
{
public:
	virtual void LoadAll(const char *config_path, const char *plugins_path) = 0;
	virtual void RefreshAll() = 0;
	virtual void Shutdown() = 0;
	virtual IdentityToken_t *GetIdentity() = 0;
	virtual void SyncMaxClients(int maxClients) = 0;
	virtual void AddPluginsListener(IPluginsListener *listener) = 0;
	virtual void RemovePluginsListener(IPluginsListener *listener) = 0;
	virtual IPluginIterator *GetPluginIterator() = 0;
	virtual void OnLibraryAction(const char *name, LibraryAction action) = 0;
	virtual bool LibraryExists(const char *name) = 0;
	virtual SMPlugin *FindPluginByOrder(unsigned num) = 0;
	virtual SMPlugin *FindPluginByIdentity(IdentityToken_t *ident) = 0;
	virtual SMPlugin *FindPluginByContext(IPluginContext *ctx) = 0;
	virtual SMPlugin *FindPluginByContext(sp_context_t *ctx) = 0;
	virtual SMPlugin *FindPluginByConsoleArg(const char *text) = 0;
	virtual SMPlugin *FindPluginByHandle(Handle_t hndl, HandleError *errp) = 0;
	virtual bool UnloadPlugin(IPlugin *plugin) = 0;
	virtual const CVector<SMPlugin *> *ListPlugins() = 0;
	virtual void FreePluginList(const CVector<SMPlugin *> *list) = 0;
	virtual void AddFunctionsToForward(const char *name, IChangeableForward *fwd) = 0;
};

class AutoPluginList
{
public:
	AutoPluginList(IScriptManager *scripts)
		: scripts_(scripts), list_(scripts->ListPlugins())
	{
	}
	~AutoPluginList()
	{
		scripts_->FreePluginList(list_);
	}
	const CVector<SMPlugin *> *operator ->()
	{
		return list_;
	}
private:
	IScriptManager *scripts_;
	const CVector<SMPlugin *> *list_;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_ISCRIPTMANAGER_H_
