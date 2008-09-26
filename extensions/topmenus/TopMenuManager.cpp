/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod TopMenus Extension
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

#include "TopMenuManager.h"
#include "TopMenu.h"

TopMenuManager g_TopMenus;
bool is_server_activated = false;

const char *TopMenuManager::GetInterfaceName()
{
	return SMINTERFACE_TOPMENUS_NAME;
}

unsigned int TopMenuManager::GetInterfaceVersion()
{
	return SMINTERFACE_TOPMENUS_VERSION;
}

ITopMenu *TopMenuManager::CreateTopMenu(ITopMenuObjectCallbacks *callbacks)
{
	TopMenu *pMenu = new TopMenu(callbacks);

	m_TopMenus.push_back(pMenu);

	return pMenu;
}

void TopMenuManager::OnClientConnected(int client)
{
	List<TopMenu *>::iterator iter;

	for (iter = m_TopMenus.begin(); iter != m_TopMenus.end(); iter++)
	{
		(*iter)->OnClientConnected(client);
	}
}

void TopMenuManager::OnClientDisconnected(int client)
{
	List<TopMenu *>::iterator iter;

	for (iter = m_TopMenus.begin(); iter != m_TopMenus.end(); iter++)
	{
		(*iter)->OnClientDisconnected(client);
	}
}

void TopMenuManager::OnServerActivated(int max_clients)
{
	if (is_server_activated)
	{
		return;
	}

	List<TopMenu *>::iterator iter;

	for (iter = m_TopMenus.begin(); iter != m_TopMenus.end(); iter++)
	{
		(*iter)->OnServerActivated(max_clients);
	}

	is_server_activated = true;
}

void TopMenuManager::OnPluginUnloaded(IPlugin *plugin)
{
	List<TopMenu *>::iterator iter = m_TopMenus.begin();

	while (iter != m_TopMenus.end())
	{
		if ((*iter)->OnIdentityRemoval(plugin->GetIdentity()))
		{
			iter++;
		}
		else
		{
			delete (*iter);
			iter = m_TopMenus.erase(iter);
		}
	}
}

void TopMenuManager::DestroyTopMenu(ITopMenu *topmenu)
{
	TopMenu *pMenu = (TopMenu *)topmenu;

	m_TopMenus.remove(pMenu);

	delete pMenu;
}

void TopMenuManager::OnMaxPlayersChanged( int newvalue )
{
	List<TopMenu *>::iterator iter;

	for (iter = m_TopMenus.begin(); iter != m_TopMenus.end(); iter++)
	{
		(*iter)->OnMaxPlayersChanged(newvalue);
	}
}
