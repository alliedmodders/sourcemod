/**
 * admin-flatfile.sp
 * Manages the standard flat files for admins.  This is the file to compile.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Admin Auth",
	author = "AlliedModders LLC",
	description = "Authenticates Admins",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnClientAuthorized(client, const String:auth[])
{
	if (StrEqual(auth, "BOT")
		|| StrEqual(auth, "STEAM_ID_LAN"))
	{
		return;
	}
	
	new AdminId:id
	if ((id = FindAdminByIdentity("steam", auth)) == INVALID_ADMIN_ID)
	{
		return;
	}
	
	decl String:buffer[256], String:account[64];
	
	FormatUserLogText(client, buffer, sizeof(buffer));
	
	if (GetAdminUsername(id, account, sizeof(account)))
	{
		LogMessage("%s authenticated to account \"%s\"", buffer, account);
	} else {
		LogMessage("%s authenticated to an anonymous account", buffer);
	}
	
	SetUserAdmin(client, id);
}
