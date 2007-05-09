/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod BAT Support Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include <IAdminSystem.h>
#include <IPlayerHelpers.h>
#include "extension.h"

BatSupport g_BatSupport;		/**< Global singleton for your extension's main interface */
IAdminSystem *admins = NULL;
IPlayerManager *players = NULL;
SMEXT_LINK(&g_BatSupport);

bool BatSupport::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	SM_GET_IFACE(ADMINSYS, admins);
	SM_GET_IFACE(PLAYERMANAGER, players);

	players->AddClientListener(this);

	return true;
}

void BatSupport::SDK_OnUnload()
{
	players->RemoveClientListener(this);

	List<AdminInterfaceListner *>::iterator iter;
	AdminInterfaceListner *hook;

	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		hook = (*iter);
		hook->OnAdminInterfaceUnload();
	}

	/* In case plugins don't do this */
	m_hooks.clear();
}

bool BatSupport::SDK_OnMetamodLoad(char *error, size_t maxlength, bool late)
{
	g_SMAPI->AddListener(this, this);

	return true;
}

void BatSupport::OnClientAuthorized(int client, const char *authstring)
{
	List<AdminInterfaceListner *>::iterator iter;
	AdminInterfaceListner *hook;

	for (iter=m_hooks.begin(); iter!=m_hooks.end(); iter++)
	{
		hook = (*iter);
		hook->Client_Authorized(client);
	}
}

const char *BatSupport::GetModName()
{
	return "SourceMod";
}

int BatSupport::GetInterfaceVersion()
{
	return ADMININTERFACE_VERSION;
}

void *BatSupport::OnMetamodQuery(const char *iface, int *ret)
{
	if (strcmp(iface, "AdminInterface") == 0)
	{
		AdminInterface *pThis = this;
		if (ret)
		{
			*ret = IFACE_OK;
		}
		return pThis;
	}

	if (ret)
	{
		*ret = IFACE_FAILED;
	}
	
	return NULL;
}

bool BatSupport::RegisterFlag(const char *Class,const char *Flag,const char *Description)
{
	/* No empty flags */
	if (Flag[0] == '\0')
	{
		g_pSM->LogError(myself, "BAT AdminInterface support tried to register a blank flag");
		return false;
	}

	/* We only support up to 6 custom flags for SourceMod */
	if (m_flags.size() >= 6)
	{
		g_pSM->LogError(myself, "BAT AdminInterface support reached maximum number of custom flags");
		return false;
	}

	List<CustomFlag>::iterator iter;
	for (iter=m_flags.begin(); iter!=m_flags.end(); iter++)
	{
		CustomFlag &cf = (*iter);
		/* Ignore already registered, in case plugin is reloading */
		if (cf.name.compare(Flag) == 0)
		{
			return true;
		}
	}

	g_pSM->LogMessage(myself, 
		"BAT AdminInterface support registered Admin_Custom%d (class \"%s\") (flag \"%s\") (descr \"%s\")",
		m_flags.size() + 1,
		Class,
		Flag,
		Description);

	unsigned int f = (unsigned int)Admin_Custom1;
	f += m_flags.size();

	CustomFlag cf;
	cf.bit = (1<<f);
	cf.flag = (AdminFlag)f;
	cf.name.assign(Flag);

	m_flags.push_back(cf);

	return true;
}

bool BatSupport::IsClient(int id)
{
	IGamePlayer *pPlayer = players->GetGamePlayer(id);
	
	if (!pPlayer)
	{
		return false;
	}

	if (!pPlayer->IsConnected())
	{
		return false;
	}

	if (pPlayer->IsFakeClient())
	{
		return false;
	}

	return true;
}

void BatSupport::AddEventListner(AdminInterfaceListner *ptr)
{
	m_hooks.push_back(ptr);
}

void BatSupport::RemoveListner(AdminInterfaceListner *ptr)
{
	m_hooks.remove(ptr);
}

bool BatSupport::HasFlag(int id,const char *Flag)
{
	IGamePlayer *pPlayer = players->GetGamePlayer(id);

	if (!pPlayer || !pPlayer->IsConnected())
	{
		return false;
	}

	AdminId admin = pPlayer->GetAdminId();
	if (admin == INVALID_ADMIN_ID)
	{
		return false;
	}

	FlagBits bits = admins->GetAdminFlags(admin, Access_Effective);

	/* Root has it all... except for immunity */
	if ((strcmp(Flag, "immunity") != 0)
	    && ((bits & ADMFLAG_ROOT) == ADMFLAG_ROOT))
	{
		return true;
	}

	if (!strcmp(Flag, "any"))
	{
		return ((bits & ~ADMFLAG_RESERVATION) != 0);
	} else if (!strcmp(Flag, "kick")) {
		return ((bits & ADMFLAG_KICK) == ADMFLAG_KICK);
	} else if (!strcmp(Flag, "slap")) {
		return ((bits & ADMFLAG_SLAY) == ADMFLAG_SLAY);
	} else if (!strcmp(Flag, "slay")) {
		return ((bits & ADMFLAG_SLAY) == ADMFLAG_SLAY);
	} else if (!strcmp(Flag, "ban")) {
		return ((bits & ADMFLAG_BAN) == ADMFLAG_BAN);
	} else if (!strcmp(Flag, "chat")) {
		return ((bits & ADMFLAG_CHAT) == ADMFLAG_CHAT);
	} else if (!strcmp(Flag, "rcon")) {
		return ((bits & ADMFLAG_RCON) == ADMFLAG_RCON);
	} else if (!strcmp(Flag, "map")) {
		return ((bits & ADMFLAG_CHANGEMAP) == ADMFLAG_CHANGEMAP);
	} else if (!strcmp(Flag, "reservedslots")) {
		return ((bits & ADMFLAG_RESERVATION) == ADMFLAG_RESERVATION);
	} else if (!strcmp(Flag, "immunuty")) {
		/* This is a bit different... */
		unsigned int count = admins->GetAdminGroupCount(admin);
		for (unsigned int i=0; i<count; i++)
		{
			GroupId gid = admins->GetAdminGroup(admin, i, NULL);
			if (admins->GetGroupGenericImmunity(gid, Immunity_Default)
				|| admins->GetGroupGenericImmunity(gid, Immunity_Global))
			{
				return true;
			}
		}
		return false;
	}

	List<CustomFlag>::iterator iter;
	for (iter=m_flags.begin(); iter!=m_flags.end(); iter++)
	{
		CustomFlag &cf = (*iter);
		if (cf.name.compare(Flag) == 0)
		{
			return ((bits & cf.bit) == cf.bit);
		}
	}

	return false;
}
