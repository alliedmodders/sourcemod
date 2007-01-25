/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_SHARESYSTEM_H_
#define _INCLUDE_SOURCEMOD_SHARESYSTEM_H_

#include <IShareSys.h>
#include <IHandleSys.h>
#include <sh_list.h>
#include "sm_globals.h"
#include "sourcemod.h"

using namespace SourceHook;

namespace SourceMod
{
	struct IdentityToken_t
	{
		Handle_t ident;
	};
};

struct IfaceInfo
{
	SMInterface *iface;
	IExtension *owner;
};

class ShareSystem : 
	public IShareSys,
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	ShareSystem();
public: //IShareSys
	bool AddInterface(IExtension *myself, SMInterface *pIface);
	bool RequestInterface(const char *iface_name, 
		unsigned int iface_vers,
		IExtension *mysql,
		SMInterface **pIface);
	void AddNatives(IExtension *myself, const sp_nativeinfo_t *natives);
	IdentityType_t CreateIdentType(const char *name);
	IdentityType_t FindIdentType(const char *name);
	IdentityToken_t *CreateIdentity(IdentityType_t type);
	void DestroyIdentType(IdentityType_t type);
	void DestroyIdentity(IdentityToken_t *identity);
	void AddDependency(IExtension *myself, const char *filename, bool require, bool autoload);
public: //SMGlobalClass
	/* Pre-empt in case anything tries to register idents early */
	void OnSourceModStartup(bool late);
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
private:
	List<IfaceInfo> m_Interfaces;
	HandleType_t m_TypeRoot;
	IdentityToken_t m_IdentRoot;
	HandleType_t m_IfaceType;
	IdentityType_t m_CoreType;
};

extern ShareSystem g_ShareSys;

#endif //_INCLUDE_SOURCEMOD_SHARESYSTEM_H_
