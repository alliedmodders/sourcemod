/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BAT Support Extension
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

/**
 * @brief Implementation of the BAT Support extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
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
