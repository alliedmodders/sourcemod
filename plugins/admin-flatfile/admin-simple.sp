/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Admin File Reader Plugin
 * Reads the admins.cfg file.  Do not compile this directly.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

public void ReadSimpleUsers()
{
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admins_simple.ini");
	
	File file = OpenFile(g_Filename, "rt");
	if (!file)
	{
		ParseError("Could not open file!");
		return;
	}
	
	while (!file.EndOfFile())
	{
		char line[255];
		if (!file.ReadLine(line, sizeof(line)))
			break;
		
		/* Trim comments */
		int len = strlen(line);
		bool ignoring = false;
		for (int i=0; i<len; i++)
		{
			if (ignoring)
			{
				if (line[i] == '"')
					ignoring = false;
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
	
	file.Close();
}

void DecodeAuthMethod(const char[] auth, char method[32], int &offset)
{
	if ((StrContains(auth, "STEAM_") == 0) || (strncmp("0:", auth, 2) == 0) || (strncmp("1:", auth, 2) == 0))
	{
		// Steam2 Id
		strcopy(method, sizeof(method), AUTHMETHOD_STEAM);
		offset = 0;
	}
	else if (!strncmp(auth, "[U:", 3) && auth[strlen(auth) - 1] == ']')
	{
		// Steam3 Id
		strcopy(method, sizeof(method), AUTHMETHOD_STEAM);
		offset = 0;
	}
	else
	{
		if (auth[0] == '!')
		{
			strcopy(method, sizeof(method), AUTHMETHOD_IP);
			offset = 1;
		}
		else
		{
			strcopy(method, sizeof(method), AUTHMETHOD_NAME);
			offset = 0;
		}
	}
}

void ReadAdminLine(const char[] line)
{
	bool is_bound;
	AdminId admin;
	char auth[64];
	char auth_method[32];
	int idx, cur_idx, auth_offset;
	
	if ((cur_idx = BreakString(line, auth, sizeof(auth))) == -1)
	{
		/* This line is bad... we need at least two parameters */
		return;
	}
	
	idx = cur_idx;
	
	/* Check if we can bind beforehand */
	DecodeAuthMethod(auth, auth_method, auth_offset);
	if ((admin = FindAdminByIdentity(auth_method, auth[auth_offset])) == INVALID_ADMIN_ID)
	{
		/* There is no binding, create the admin */
		admin = CreateAdmin();
	}
	else
	{
		is_bound = true;
	}
	
	/* Read flags */
	char flags[64];	
	cur_idx = BreakString(line[idx], flags, sizeof(flags));
	idx += cur_idx;

	/* Read immunity level, if any */
	int level, flag_idx;

	if ((flag_idx = StringToIntEx(flags, level)) > 0)
	{
		admin.ImmunityLevel = level;
		if (flags[flag_idx] == ':')
		{
			flag_idx++;
		}
	}

	if (flags[flag_idx] == '@')
	{
		GroupId gid = FindAdmGroup(flags[flag_idx + 1]);
		if (gid == INVALID_GROUP_ID)
		{
			ParseError("Invalid group detected: %s", flags[flag_idx + 1]);
			return;
		}
		admin.InheritGroup(gid);
	}
	else
	{
		int len = strlen(flags[flag_idx]);
		bool is_default = false;
		for (int i=0; i<len; i++)
		{
			if (!level && flags[flag_idx + i] == '$')
			{
				admin.ImmunityLevel = 1;
			} else {
				AdminFlag flag;
				
				if (!FindFlagByChar(flags[flag_idx + i], flag))
				{
					ParseError("Invalid flag detected: %c", flags[flag_idx + i]);
					continue;
				}
				admin.SetFlag(flag, true);
			}
		}
		
		if (is_default)
		{
			GroupId gid = FindAdmGroup("Default");
			if (gid != INVALID_GROUP_ID)
			{
				admin.InheritGroup(gid);
			}
		}
	}
	
	/* Lastly, is there a password? */
	if (cur_idx != -1)
	{
		char password[64];
		BreakString(line[idx], password, sizeof(password));
		admin.SetPassword(password);
	}
	
	/* Now, bind the identity to something */
	if (!is_bound)
	{
		if (!admin.BindIdentity(auth_method, auth[auth_offset]))
		{
			/* We should never reach here */
			RemoveAdmin(admin);
			ParseError("Failed to bind identity %s (method %s)", auth[auth_offset], auth_method);			
		}
	}
}
