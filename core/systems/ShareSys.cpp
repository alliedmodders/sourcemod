/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

ShareSystem g_ShareSys;

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

void ShareSystem::OnSourceModStartup(bool late)
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
	g_Extensions.AddNatives(myself, natives);
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
		} else {
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
