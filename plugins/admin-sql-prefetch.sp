/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SQL Admins Plugin (Prefetch)
 * Prefetches admins from an SQL database without threading.
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
	
	if (db == null)
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
	
	delete db;
}

FetchUsers(Handle:db)
{
	decl String:query[255], String:error[255];
	new Handle:hQuery;

	Format(query, sizeof(query), "SELECT id, authtype, identity, password, flags, name, immunity FROM sm_admins");
	if ((hQuery = SQL_Query(db, query)) == null)
	{
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchUsers() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}

	decl String:authtype[16];
	decl String:identity[80];
	decl String:password[80];
	decl String:flags[32];
	decl String:name[80];
	new immunity;
	new AdminId:adm, id;
	new GroupId:gid;

	/* Keep track of a mapping from admin DB IDs to internal AdminIds to
	 * enable group lookups en masse */
	new Handle:htAdmins = CreateTrie();
	decl String:key[16];
	
	while (SQL_FetchRow(hQuery))
	{
		id = SQL_FetchInt(hQuery, 0);
		IntToString(id, key, sizeof(key));
		SQL_FetchString(hQuery, 1, authtype, sizeof(authtype));
		SQL_FetchString(hQuery, 2, identity, sizeof(identity));
		SQL_FetchString(hQuery, 3, password, sizeof(password));
		SQL_FetchString(hQuery, 4, flags, sizeof(flags));
		SQL_FetchString(hQuery, 5, name, sizeof(name));
		immunity = SQL_FetchInt(hQuery, 6);
		
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

		SetTrieValue(htAdmins, key, adm);
		
		#if defined _DEBUG
		PrintToServer("Found SQL admin (%d,%s,%s,%s,%s,%s,%d):%d", id, authtype, identity, password, flags, name, immunity, adm);
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

		SetAdminImmunityLevel(adm, immunity);
	}

	new Handle:hGroupQuery;
	Format(query, sizeof(query), "SELECT ag.admin_id AS id, g.name FROM sm_admins_groups ag JOIN sm_groups g ON ag.group_id = g.id  ORDER BY id, inherit_order ASC");
	if ((hGroupQuery = SQL_Query(db, query)) == null)
	{
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchUsers() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}

	decl String:group[80];
	while (SQL_FetchRow(hGroupQuery))
	{
		IntToString(SQL_FetchInt(hGroupQuery, 0), key, sizeof(key));
		SQL_FetchString(hGroupQuery, 1, group, sizeof(group));

		if (GetTrieValue(htAdmins, key, adm))
		{
			if ((gid = FindAdmGroup(group)) == INVALID_GROUP_ID)
			{
				/* Group wasn't found, don't bother with it.  */
				continue;
			}

			AdminInheritGroup(adm, gid);
		}
	}
	
	delete hQuery;
	delete hGroupQuery;
	delete htAdmins;
}

FetchGroups(Handle:db)
{
	decl String:query[255];
	new Handle:hQuery;
	
	Format(query, sizeof(query), "SELECT flags, name, immunity_level FROM sm_groups");

	if ((hQuery = SQL_Query(db, query)) == null)
	{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("FetchGroups() query failed: %s", query);
		LogError("Query error: %s", error);
		return;
	}
	
	/* Now start fetching groups */
	decl String:flags[32];
	decl String:name[128];
	new immunity;
	while (SQL_FetchRow(hQuery))
	{
		SQL_FetchString(hQuery, 0, flags, sizeof(flags));
		SQL_FetchString(hQuery, 1, name, sizeof(name));
		immunity = SQL_FetchInt(hQuery, 2);
		
#if defined _DEBUG
		PrintToServer("Adding group (%d, %s, %s)", immunity, flags, name);
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
		SetAdmGroupImmunityLevel(gid, immunity);
	}
	
	delete hQuery;
	
	/** 
	 * Get immunity in a big lump.  This is a nasty query but it gets the job done.
	 */
	new len = 0;
	len += Format(query[len], sizeof(query)-len, "SELECT g1.name, g2.name FROM sm_group_immunity gi");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g1 ON g1.id = gi.group_id ");
	len += Format(query[len], sizeof(query)-len, " LEFT JOIN sm_groups g2 ON g2.id = gi.other_id");
	
	if ((hQuery = SQL_Query(db, query)) == null)
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
	
	delete hQuery;
	
	/**
	 * Fetch overrides in a lump query.
	 */
	Format(query, sizeof(query), "SELECT g.name, go.type, go.name, go.access FROM sm_group_overrides go LEFT JOIN sm_groups g ON go.group_id = g.id");
	
	if ((hQuery = SQL_Query(db, query)) == null)
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
	
	delete hQuery;
}

FetchOverrides(Handle:db)
{
	decl String:query[255];
	new Handle:hQuery;
	
	Format(query, sizeof(query), "SELECT type, name, flags FROM sm_overrides");

	if ((hQuery = SQL_Query(db, query)) == null)
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
	
	delete hQuery;
}

