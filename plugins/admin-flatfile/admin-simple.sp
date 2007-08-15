/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Reads the admins.cfg file.  Do not compile this directly.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

public ReadSimpleUsers()
{
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admins_simple.ini");
	
	new Handle:file = OpenFile(g_Filename, "rt");
	if (file == INVALID_HANDLE)
	{
		ParseError("Could not open file!");
		return;
	}
	
	while (!IsEndOfFile(file))
	{
		decl String:line[255];
		if (!ReadFileLine(file, line, sizeof(line)))
		{
			break;
		}
		
		/* Trim comments */
		new len = strlen(line);
		new bool:ignoring = false;
		for (new i=0; i<len; i++)
		{
			if (ignoring)
			{
				if (line[i] == '"')
				{
					ignoring = false;
				}
			} else {
				if (line[i] == '"')
				{
					ignoring = true;
				} else if (line[i] == ';') {
					line[i] = '\0';
					break;
				} else if (line[i] == '/'
							&& i != len - 1
							&& line[i+1] == '/')
				{
					line[i] = '\0';
					break;
				}
			}
		}
		
		TrimString(line);
		
		if ((line[0] == '/' && line[1] == '/')
			|| (line[0] == ';' || line[0] == '\0'))
		{
			continue;
		}
	
		ReadAdminLine(line);
	}
	
	CloseHandle(file);
}

ReadAdminLine(const String:line[])
{
	new String:auth[64];
	new cur_idx = BreakString(line, auth, sizeof(auth));
	new idx = cur_idx;
	
	if (cur_idx == -1)
	{
		/* This line is bad... we need at least two parameters */
		return;
	}
	
	/* Create the admin */
	new AdminId:admin = CreateAdmin(auth);
	
	/* Read flags */
	new String:flags[64];	
	cur_idx = BreakString(line[idx], flags, sizeof(flags));
	idx += cur_idx;
	
	if (flags[0] == '@')
	{
		new GroupId:gid = FindAdmGroup(flags[1]);
		if (gid == INVALID_GROUP_ID)
		{
			ParseError("Invalid group detected: %s", flags[1]);
			return;
		}
		AdminInheritGroup(admin, gid);
	} else {
		new len = strlen(flags);
		new bool:is_default = false;
		for (new i=0; i<len; i++)
		{
			if (flags[i] == '$')
			{
				is_default = true;
			} else {
				new AdminFlag:flag;
				
				if (!FindFlagByChar(flags[i], flag))
				{
					ParseError("Invalid flag detected: %c", flags[i]);
					continue;
				}
				SetAdminFlag(admin, flag, true);
			}
		}
		
		if (is_default)
		{
			new GroupId:gid = FindAdmGroup("Default");
			if (gid != INVALID_GROUP_ID)
			{
				AdminInheritGroup(admin, gid);
			}
		}
	}
	
	/* Lastly, is there a password? */
	if (cur_idx != -1)
	{
		decl String:password[64];
		BreakString(line[idx], password, sizeof(password));
		SetAdminPassword(admin, password);
	}
	
	/* Now, bind the identity to something */
	if (StrContains(auth, "STEAM_") == 0)
	{
		BindAdminIdentity(admin, AUTHMETHOD_STEAM, auth);
	} else {
		if (auth[0] == '!')
		{
			BindAdminIdentity(admin, AUTHMETHOD_IP, auth[1]);
		} else {
			BindAdminIdentity(admin, AUTHMETHOD_NAME, auth);
		}
	}
}
