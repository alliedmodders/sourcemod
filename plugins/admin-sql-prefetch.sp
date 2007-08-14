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
	new Handle:db;
	
	if (SQL_CheckConfig("admins"))
	{
		db = SQL_Connect("admins", true, error, sizeof(error));
	} else {
		db = SQL_Connect("default", true, error, sizeof(error));
	}
	
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
	
	Format(query, sizeof(query), "SELECT id, authtype, identity, password, flags, name FROM sm_admins");
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
	
	Format(query, sizeof(query), "SELECT immunity, flags, name FROM sm_groups");

	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	/* Now start fetching groups */
	decl String:immunity[16];
	decl String:flags[32];
	decl String:name[128];
	while (SQL_FetchRow(hQuery))
	{
		SQL_FetchString(hQuery, 0, immunity, sizeof(immunity));
		SQL_FetchString(hQuery, 1, flags, sizeof(flags));
		SQL_FetchString(hQuery, 2, name, sizeof(name));
		
#if defined _DEBUG
		PrintToServer("Adding group (%s, %s, %s)", immunity, flags, name);
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
	}
	
	CloseHandle(hQuery);
	
	/** 
	 * Get immunity in a big lump.  This is a nasty query but it gets the job done.
	 */
	new len = 0;
	len += Format(query[len], sizeof(query)-len, "SELECT g1.name, g2.name FROM sm_group_immunity gi");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g1 ON g1.id = gi.group_id ");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");
	
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	while (SQL_FetchRow(hQuery))
	{
		decl String:group1[80];
		decl String:group2[80];
		new GroupId:gid1, GroupId:gid2;
		
		SQL_FetchString(hQuery, 0, group1, sizeof(group1));
		SQL_FetchString(hQuery, 1, group2, sizeof(group2));
		
		if (((gid1 = FindAdmGroup(group1)) == INVALID_GROUP_ID)
			|| (gid2 = FindAdmGroup(group2)) == INVALID_GROUP_ID)
		{
			continue;
		}
		
		SetAdmGroupImmuneFrom(gid1, gid2);
#if defined _DEBUG
		PrintToServer("SetAdmGroupImmuneFrom(%d, %d)", gid1, gid2);
#endif
	}
	
	CloseHandle(hQuery);
	
	/**
	 * Fetch overrides in a lump query.
	 */
	Format(query, sizeof(query), "SELECT g.name, go.type, go.name, go.access FROM sm_group_overrides go LEFT JOIN sm_groups g ON go.group_id = g.id");
	
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	decl String:type[16];
	decl String:cmd[64];
	decl String:access[16];
	while (SQL_FetchRow(hQuery))
	{
		SQL_FetchString(hQuery, 0, name, sizeof(name));
		SQL_FetchString(hQuery, 1, type, sizeof(type));
		SQL_FetchString(hQuery, 2, cmd, sizeof(cmd));
		SQL_FetchString(hQuery, 3, access, sizeof(access));
		
		new GroupId:gid;
		if ((gid = FindAdmGroup(name)) == INVALID_GROUP_ID)
		{
			continue;
		}
				
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
