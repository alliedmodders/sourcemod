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
#ifndef _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IEXTBRIDGE_H_
#define _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IEXTBRIDGE_H_

#include <sp_vm_api.h>
#include <IExtensionSys.h>
#include <sh_vector.h>

struct edict_t;

namespace SourceMod {

using namespace SourceHook;
using namespace SourcePawn;
class SMPlugin;

class IExtensionSys : public IExtensionManager
{
public:
	virtual IExtension *LoadAutoExtension(const char *name, bool bErrorOnMissing=true) = 0;
	virtual void TryAutoload() = 0;
	virtual void Shutdown() = 0;
	virtual IExtension *FindExtensionByFile(const char *name) = 0;
	virtual bool LibraryExists(const char *name) = 0;
	virtual void CallOnCoreMapStart(edict_t *edictList, int edictCount, int maxClients) = 0;
	virtual IExtension *GetExtensionFromIdent(IdentityToken_t *token) = 0;
	virtual void BindChildPlugin(IExtension *ext, SMPlugin *plugin) = 0;
	virtual void AddRawDependency(IExtension *myself, IdentityToken_t *token, void *iface) = 0;
	virtual const CVector<IExtension *> *ListExtensions() = 0;
	virtual void FreeExtensionList(const CVector<IExtension *> *list) = 0;
	virtual void CallOnCoreMapEnd() = 0;
};

class AutoExtensionList
{
public:
	AutoExtensionList(IExtensionSys *extensions)
		: extensions_(extensions), list_(extensions_->ListExtensions())
	{
	}
	~AutoExtensionList()
	{
		extensions_->FreeExtensionList(list_);
	}
	const CVector<IExtension *> *operator ->()
	{
		return list_;
	}
private:
	IExtensionSys *extensions_;
	const CVector<IExtension *> *list_;
};

} // namespace SourceMod

#endif // _INCLUDE_SOURCEMOD_BRIDGE_INCLUDE_IEXTBRIDGE_H_
