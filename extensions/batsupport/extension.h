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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief BAT Support extension code header.
 */

#include "smsdk_ext.h"
#include "BATInterface.h"
#include <sh_list.h>
#include <sh_string.h>

using namespace SourceHook;

struct CustomFlag
{
	String name;
	AdminFlag flag;
	FlagBits bit;
};

class BatSupport : 
	public SDKExtension,
	public IMetamodListener,
	public AdminInterface,
	public IClientListener
{
public: // SDKExtension
	bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	void SDK_OnUnload();
	bool SDK_OnMetamodLoad(char *error, size_t maxlength, bool late);
public: // IMetamodListener
	void *OnMetamodQuery(const char *iface, int *ret);
public: // AdminInterface
	bool RegisterFlag(const char *Class, const char *Flag, const char *Description);
	bool IsClient(int id);
	bool HasFlag(int id, const char *Flag);
	int GetInterfaceVersion();
	const char* GetModName();
	void AddEventListner(AdminInterfaceListner *ptr);
	void RemoveListner(AdminInterfaceListner *ptr);
public: // IClientListener
	void OnClientAuthorized(int client, const char *authstring);
private:
	List<AdminInterfaceListner *> m_hooks;
	List<CustomFlag> m_flags;
};

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
