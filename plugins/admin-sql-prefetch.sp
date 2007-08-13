/**
 * admin-sql-prefetch.sp
 * Prefetches admins from an SQL database without threading.
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
 *
 * Version: $Id$
 */

/* We like semicolons */
#pragma semicolon 1

#include <sourcemod>

public Plugin:myinfo = 
{
	name = "SQL Admins (Prefetch)",
	author = "AlliedModders LLC",
	description = "Reads all admins from SQL",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnRebuildAdminCache(AdminCachePart:part)
{
	/* First try to get a database connection */
	decl String:error[255];
	new Handle:db = SQL_Connect("default", true, error, sizeof(error));
	if (db == INVALID_HANDLE)
	{
		LogError("Could not connect to database \"default\": %s", error);
		return;
	}
	
	if (part == AdminCache_Overrides)
	{
		FetchOverrides(db);
	} else if (part == AdminCache_Groups) {
		FetchGroups(db);
	} else if (part == AdminCache_Admins) {
		FetchUsers(db);
	}
	
	CloseHandle(db);
}

FetchUserGroups(Handle:hQuery, AdminId:adm, id)
{
	SQL_BindParamInt(hQuery, 0, id);
	if (!SQL_Execute(hQuery))
	{
		decl String:error[255];
		SQL_GetError(hQuery, error, sizeof(error));
		LogError("FetchUserGroups() statement execute failed: %s", error);
		return;
	}
	
	decl String:name[80];
	new GroupId:gid;
	while (SQL_FetchRow(hQuery))
	{
		SQL_FetchString(hQuery, 0, name, sizeof(name));
		
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			/* Group wasn't found, don't bother with it.  */
			continue;
		}
		
		#if defined _DEBUG
		PrintToServer("Found admin group (%d, %d, %d)", id, gid, adm);
		#endif
		
		AdminInheritGroup(adm, gid);
	}
}

FetchUsers(Handle:db)
{
	decl String:query[255], String:error[255];
	new Handle:hQuery, Handle:hGroupQuery;
	
	Format(query, sizeof(query), "SELECT id, authtype, identity, passwd, flags, name FROM sm_admins");
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchUsers() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	/* If this fails it's non-fatal */
	Format(query,
		sizeof(query), 
		"SELECT g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id WHERE ag.admin_id = ? ORDER BY inherit_order ASC");
	if ((hGroupQuery = SQL_PrepareQuery(db, query, error, sizeof(error))) == INVALID_HANDLE)
	{
		LogError("FetchUsers() group query preparation failed: %s", query);
		LogError("Query error: %s", error);
	}
	
	decl String:authtype[16];
	decl String:identity[80];
	decl String:password[80];
	decl String:flags[32];
	decl String:name[80];
	new AdminId:adm, id;
	
	while (SQL_FetchRow(hQuery))
	{
		id = SQL_FetchInt(hQuery, 0);
		SQL_FetchString(hQuery, 1, authtype, sizeof(authtype));
		SQL_FetchString(hQuery, 2, identity, sizeof(identity));
		SQL_FetchString(hQuery, 3, password, sizeof(password));
		SQL_FetchString(hQuery, 4, flags, sizeof(flags));
		SQL_FetchString(hQuery, 5, name, sizeof(name));
		
		/* Use a pre-existing admin if we can */
		if ((adm = FindAdminByIdentity(authtype, identity)) == INVALID_ADMIN_ID)
		{
			adm = CreateAdmin(name);
			if (!BindAdminIdentity(adm, authtype, identity))
			{
				LogError("Could not bind prefetched SQL admin (authtype \"%s\") (identity \"%s\")", authtype, identity);
				continue;
			}
		}
		
		#if defined _DEBUG
		PrintToServer("Found SQL admin (%d,%s,%s,%s,%s,%s):%d", id, authtype, identity, password, flags, name, adm);
		#endif
		
		/* See if this admin wants a password */
		if (password[0] != '\0')
		{
			SetAdminPassword(adm, password);
		}
		
		/* Apply each flag */
		new len = strlen(flags);
		new AdminFlag:flag;
		for (new i=0; i<len; i++)
		{
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			SetAdminFlag(adm, flag, true);
		}
		
		/* Look up groups */
		if (hGroupQuery != INVALID_HANDLE)
		{
			FetchUserGroups(hGroupQuery, adm, id);
		}
	}
	
	CloseHandle(hGroupQuery);
	CloseHandle(hQuery);
}

FetchGroups(Handle:db)
{
	decl String:query[255];
	new Handle:hQuery;
	
	Format(query, sizeof(query), "SELECT id, immunity, flags, name, groups_immune FROM sm_groups");

	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	/* We cache basic group info so we can do reverse lookups */
	new Handle:groups = CreateArray(3);
	
	/* Now start fetching groups */
	decl String:immunity[16];
	decl String:flags[32];
	decl String:name[128];
	decl String:grp_immunity[256];
	while (SQL_FetchRow(hQuery))
	{
		new id = SQL_FetchInt(hQuery, 0);
		SQL_FetchString(hQuery, 1, immunity, sizeof(immunity));
		SQL_FetchString(hQuery, 2, flags, sizeof(flags));
		SQL_FetchString(hQuery, 3, name, sizeof(name));
		SQL_FetchString(hQuery, 4, grp_immunity, sizeof(grp_immunity));
		
#if defined _DEBUG
		PrintToServer("Adding group (%d, %s, %s, %s, %s)", id, immunity, flags, name, grp_immunity);
#endif
		
		/* Find or create the group */
		new GroupId:gid;
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			gid = CreateAdmGroup(name);
		}
		
		/* Add flags from the database to the group */
		new num_flag_chars = strlen(flags);
		for (new i=0; i<num_flag_chars; i++)
		{
			decl AdminFlag:flag;
			if (!FindFlagByChar(flags[i], flag))
			{
				continue;
			}
			SetAdmGroupAddFlag(gid, flag, true);
		}
		
		/* Set the immunity level this group has */
		if (StrEqual(immunity, "all"))
		{
			SetAdmGroupImmunity(gid, Immunity_Default, true);
			SetAdmGroupImmunity(gid, Immunity_Global, true);
		} else if (StrEqual(immunity, "default")) {
			SetAdmGroupImmunity(gid, Immunity_Default, true);
		} else if (StrEqual(immunity, "global")) {
			SetAdmGroupImmunity(gid, Immunity_Global, true);
		}
		
		new Handle:immunity_list = UTIL_ExplodeNumberString(grp_immunity);
		
		/* Now, save all this for later */
		decl data[3];
		data[0] = id;
		data[1] = _:gid;
		data[2] = _:immunity_list;
		
		PushArrayArray(groups, data);
	}
	
	CloseHandle(hQuery);
	
	/* Second pass - resolve immunity and group overrides */
	new num_groups = GetArraySize(groups);
	for (new i=0; i<num_groups; i++)
	{
		decl data[3];
		GetArrayArray(groups, i, data);
		
		new id = data[0];
		new GroupId:gid = GroupId:data[1];
		new Handle:immunity_list = Handle:data[2];
		
		/* Query to get per-group override list */
		Format(query, 
			sizeof(query),
			"SELECT type, name, access FROM sm_group_overrides WHERE group_id = %d",
			id);
		
		/* Fetch the override query results */
		if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
		{
			decl String:error[255];
			SQL_GetError(db, error, sizeof(error));
			LogError("FetchOverrides() query failed: %s", query);
			LogError("Query error: %s", error);
		} else {
			decl String:type[16];
			decl String:cmd[64];
			decl String:access[16];
			while (SQL_FetchRow(hQuery))
			{
				SQL_FetchString(hQuery, 0, type, sizeof(type));
				SQL_FetchString(hQuery, 1, cmd, sizeof(cmd));
				SQL_FetchString(hQuery, 2, access, sizeof(access));
				
				new OverrideType:o_type = Override_Command;
				if (StrEqual(type, "group"))
				{
					o_type = Override_CommandGroup;
				}
				
				new OverrideRule:o_rule = Command_Deny;
				if (StrEqual(access, "allow"))
				{
					o_rule = Command_Allow;
				}
				
				#if defined _DEBUG
				PrintToServer("AddAdmGroupCmdOverride(%d, %s, %d, %d)", gid, cmd, o_type, o_rule);
				#endif
				
				AddAdmGroupCmdOverride(gid, cmd, o_type, o_rule);
			}
			CloseHandle(hQuery);
		}
		
		/* Lastly, resolve the immunity list if any */
		new immunity_count;
		if (immunity_list != INVALID_HANDLE 
			&& ((immunity_count = GetArraySize(immunity_list)) > 0))
		{
			for (new j=0; j<immunity_count; j++)
			{
				new other_id = GetArrayCell(immunity_list, j);
				
				if (other_id == id)
				{
					continue;
				}
				
				/* See if we can find this other ID */
				new GroupId:other_gid;
				if ((other_gid = UTIL_FindGroupInCache(groups, other_id)) != INVALID_GROUP_ID)
				{
					#if defined _DEBUG
					PrintToServer("SetAdmGroupImmuneFrom(%d, %d)", gid, other_gid);
					#endif
					SetAdmGroupImmuneFrom(gid, other_gid);
				}
			}
		}
	}
}

FetchOverrides(Handle:db)
{
	decl String:query[255];
	new Handle:hQuery;
	
	Format(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");

	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchOverrides() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	decl String:type[64];
	decl String:name[64];
	decl String:flags[32];
	new flag_bits;
	while (SQL_FetchRow(hQuery))
	{
		SQL_FetchString(hQuery, 0, type, sizeof(type));
		SQL_FetchString(hQuery, 1, name, sizeof(name));
		SQL_FetchString(hQuery, 2, flags, sizeof(flags));
		
#if defined _DEBUG
		PrintToServer("Adding override (%s, %s, %s)", type, name, flags);
#endif
		
		flag_bits = ReadFlagString(flags);
		if (StrEqual(type, "command"))
		{
			AddCommandOverride(name, Override_Command, flag_bits);
		} else if (StrEqual(type, "group")) {
			AddCommandOverride(name, Override_CommandGroup, flag_bits);
		}
	}
	
	CloseHandle(hQuery);
}

stock GroupId:UTIL_FindGroupInCache(Handle:group_list, id)
{
	new size = GetArraySize(group_list);
	
	for (new i=0; i<size; i++)
	{
		decl data[3];
		GetArrayArray(group_list, i, data);
		
		if (data[0] == id)
		{
			return GroupId:data[1];
		}
	}
	
	return INVALID_GROUP_ID;
}

/**
 * Breaks a comma-delimited string into a list of numbers, returning a new 
 * dynamic array.
 *
 * @param text				The string to split.
 * @return					A new array Handle which must be closed, or 
 *							INVALID_HANDLE on no strings found.
 */
stock Handle:UTIL_ExplodeNumberString(const String:text[])
{
	new Handle:array = INVALID_HANDLE;
	decl String:buffer[32];
	new reloc_idx, idx;
	
	while ((idx = SplitString(text[reloc_idx], ",", buffer, sizeof(buffer))) != -1)
	{
		reloc_idx += idx;
		if (text[reloc_idx] == '\0')
		{
			break;
		}
		if (array == INVALID_HANDLE)
		{
			array = CreateArray();
		}
		PushArrayCell(array, StringToInt(buffer));
	}
	
	if (text[reloc_idx] != '\0')
	{
		if (array == INVALID_HANDLE)
		{
			array = CreateArray();
		}
		PushArrayCell(array, StringToInt(text[reloc_idx]));
	}
	
	return array;
}

