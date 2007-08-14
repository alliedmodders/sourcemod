/**
 * sql-admin-manager.sp
 * Adds/managers admins and groups in an SQL database.
 *
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
	name = "SQL Admin Manager",
	author = "AlliedModders LLC",
	description = "Manages SQL admins",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("sqladmins.phrases");

	RegAdminCmd("sm_sql_addadmin", Command_AddAdmin, ADMFLAG_ROOT, "Adds an admin to the SQL database");
	RegAdminCmd("sm_sql_deladmin", Command_DelAdmin, ADMFLAG_ROOT, "Removes an admin from the SQL database");
	RegAdminCmd("sm_sql_addgroup", Command_AddGroup, ADMFLAG_ROOT, "Adds a group to the SQL database");
	RegAdminCmd("sm_sql_delgroup", Command_DelGroup, ADMFLAG_ROOT, "Removes a group from the SQL database");
	RegAdminCmd("sm_sql_setadmingroups", Command_SetAdminGroups, ADMFLAG_ROOT, "Sets an admin's groups in the SQL database");
}

Handle:Connect()
{
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
		LogError("Could not connect to database: %s", error);
	}
	
	return db;
}

public Action:Command_SetAdminGroups(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_setadmingroups <authtype> <identity> [group1] ... [group N]");
		return Plugin_Handled;
	}
	
	decl String:authtype[16];
	GetCmdArg(1, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	new Handle:db = Connect();
	if (db == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	decl String:identity[65];
	decl String:safe_identity[140];
	GetCmdArg(2, identity, sizeof(identity));
	SQL_QuoteString(db, identity, safe_identity, sizeof(safe_identity));
	
	decl String:query[255];
	Format(query, 
		sizeof(query),
		"SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'",
		authtype,
		safe_identity);
		
	new Handle:hQuery;
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		return DoError(client, db, query, "Admin lookup query failed");
	}
	
	if (!SQL_FetchRow(hQuery))
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin not found");
		CloseHandle(hQuery);
		CloseHandle(db);
		return Plugin_Handled;
	}
	
	new id = SQL_FetchInt(hQuery, 0);
	
	CloseHandle(hQuery);
	
	/**
	 * First delete all of the user's existing groups.
	 */
	Format(query, sizeof(query), "DELETE FROM sm_admins_groups WHERE admin_id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin group deletion query failed");
	}
	
	if (args < 3)
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin groups reset");
		CloseHandle(db);
		return Plugin_Handled;
	}
	
	decl String:error[256];
	new Handle:hAddQuery, Handle:hFindQuery;
	
	Format(query, sizeof(query), "SELECT id FROM sm_groups WHERE name = ?");
	if ((hFindQuery = SQL_PrepareQuery(db, query, error, sizeof(error))) == INVALID_HANDLE)
	{
		return DoStmtError(client, db, query, error, "Group search prepare failed");
	}
	
	Format(query, 
		sizeof(query), 
		"INSERT INTO sm_admins_groups (admin_id, group_id, inherit_order) VALUES (%d, ?, ?)",
		id);
	if ((hAddQuery = SQL_PrepareQuery(db, query, error, sizeof(error))) == INVALID_HANDLE)
	{
		CloseHandle(hFindQuery);
		return DoStmtError(client, db, query, error, "Add admin group prepare failed");
	}
	
	decl String:name[80];
	new inherit_order = 0;
	for (new i=3; i<=args; i++)
	{
		GetCmdArg(i, name, sizeof(name));
		
		SQL_BindParamString(hFindQuery, 0, name, false);
		if (!SQL_Execute(hFindQuery) || !SQL_FetchRow(hFindQuery))
		{
			ReplyToCommand(client, "[SM] %t", "SQL Group X not found", name);
		} else {
			new gid = SQL_FetchInt(hFindQuery, 0);
			
			SQL_BindParamInt(hAddQuery, 0, gid);
			SQL_BindParamInt(hAddQuery, 1, ++inherit_order);
			if (!SQL_Execute(hAddQuery))
			{
				ReplyToCommand(client, "[SM] %t", "SQL Group X failed to bind", name);
				inherit_order--;
			}
		}
	}
	
	CloseHandle(hAddQuery);
	CloseHandle(hFindQuery);
	CloseHandle(db);
	
	if (inherit_order == 1)
	{
		ReplyToCommand(client, "[SM] %t", "Added group to user");
	} else if (inherit_order > 1) {
		ReplyToCommand(client, "[SM] %t", "Added groups to user", inherit_order);
	}
	
	return Plugin_Handled;
}

public Action:Command_DelGroup(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_addgroup <name>");
		return Plugin_Handled;
	}

	new Handle:db = Connect();
	if (db == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	new len;
	decl String:name[80];
	decl String:safe_name[180];
	GetCmdArgString(name, sizeof(name));
	
	/* Strip quotes in case the user tries to use them */
	len = strlen(name);
	if (len > 1 && (name[0] == '"' && name[len-1] == '"'))
	{
		name[--len] = '\0';
		SQL_QuoteString(db, name[1], safe_name, sizeof(safe_name));
	} else {
		SQL_QuoteString(db, name, safe_name, sizeof(safe_name));
	}
	
	decl String:query[256];
	
	/* Delete admin inheritance for this group */
	Format(query, 
		sizeof(query),
		"DELETE FROM sm_admins_groups WHERE group_id IN (SELECT id FROM sm_groups WHERE name = '%s')",
		safe_name);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin group deletion query failed");
	}
	
	/* Delete group overrides */
	Format(query, 
		sizeof(query),
		"DELETE FROM sm_group_overrides WHERE group_id IN (SELECT id FROM sm_groups WHERE name = '%s')",
		safe_name);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group override deletion query failed");
	}
	
	/* Finally delete the group */
	Format(query, sizeof(query), "DELETE FROM sm_groups WHERE name = '%s'", safe_name);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group deletion query failed");
	}
	
	if (SQL_GetAffectedRows(db))
	{
		ReplyToCommand(client, "[SM] %t", "SQL Group deleted");
	} else {
		ReplyToCommand(client, "[SM] %t", "SQL Group not found");
	}
	
	CloseHandle(db);
	
	return Plugin_Handled;
}

public Action:Command_AddGroup(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_addgroup <name> <flags> [immunity]");
		ReplyToCommand(client, "[SM] %t", "Invalid immunity");
		return Plugin_Handled;
	}

	new String:immunity[32] = "none";
	if (args >= 3)
	{
		GetCmdArg(3, immunity, sizeof(immunity));
		if (!StrEqual(immunity, "none")
			&& !StrEqual(immunity, "global")
			&& !StrEqual(immunity, "default"))
		{
			ReplyToCommand(client, "[SM] %t", "Invalid immunity");
			return Plugin_Handled;
		}
	}
	
	new Handle:db = Connect();
	if (db == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	decl String:name[64];
	decl String:safe_name[64];
	GetCmdArg(1, name, sizeof(name));
	SQL_QuoteString(db, name, safe_name, sizeof(safe_name));
	
	new Handle:hQuery;
	decl String:query[256];
	Format(query, sizeof(query), "SELECT id FROM sm_groups WHERE name = '%s'", safe_name);
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		return DoError(client, db, query, "Group retrieval query failed");
	}
	
	if (SQL_GetRowCount(hQuery) > 0)
	{
		ReplyToCommand(client, "[SM] %t", "SQL Group already exists");
		CloseHandle(hQuery);
		CloseHandle(db);
		return Plugin_Handled;
	}
	
	CloseHandle(hQuery);
	
	decl String:flags[30];
	decl String:safe_flags[64];
	GetCmdArg(2, flags, sizeof(safe_flags));
	SQL_QuoteString(db, flags, safe_flags, sizeof(safe_flags));
	
	Format(query, 
		sizeof(query),
		"INSERT INTO sm_groups (immunity, flags, name) VALUES ('%s', '%s', '%s')",
		immunity,
		safe_flags,
		safe_name);
	
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group insertion query failed");
	}
	
	ReplyToCommand(client, "[SM] %t", "SQL Group added");
	
	CloseHandle(db);
		
	return Plugin_Handled;
}	

public Action:Command_DelAdmin(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_deladmin <authtype> <identity>");
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	decl String:authtype[16];
	GetCmdArg(1, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	new Handle:db = Connect();
	if (db == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	decl String:identity[65];
	decl String:safe_identity[140];
	GetCmdArg(2, identity, sizeof(identity));
	SQL_QuoteString(db, identity, safe_identity, sizeof(safe_identity));
	
	decl String:query[255];
	Format(query, 
		sizeof(query),
		"SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'",
		authtype,
		safe_identity);
		
	new Handle:hQuery;
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		return DoError(client, db, query, "Admin lookup query failed");
	}
	
	if (!SQL_FetchRow(hQuery))
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin not found");
		CloseHandle(hQuery);
		CloseHandle(db);
		return Plugin_Handled;
	}
	
	new id = SQL_FetchInt(hQuery, 0);
	
	CloseHandle(hQuery);
	
	/* Delete group bindings */
	Format(query, sizeof(query), "DELETE FROM sm_admins_groups WHERE admin_id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin group deletion query failed");
	}
	
	Format(query, sizeof(query), "DELETE FROM sm_admins WHERE id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin deletion query failed");
	}
	
	CloseHandle(db);
	
	ReplyToCommand(client, "[SM] %t", "SQL Admin deleted");
	
	return Plugin_Handled;
}

public Action:Command_AddAdmin(client, args)
{
	if (args < 4)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_addadmin <alias> <authtype> <identity> <flags> [password]");
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	decl String:authtype[16];
	GetCmdArg(2, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	decl String:identity[65];
	decl String:safe_identity[140];
	GetCmdArg(3, identity, sizeof(identity));
	
	decl String:query[256];
	new Handle:hQuery;
	new Handle:db = Connect();
	if (db == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	SQL_QuoteString(db, identity, safe_identity, sizeof(safe_identity));
	
	Format(query, sizeof(query), "SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'", authtype, identity);
	if ((hQuery = SQL_Query(db, query)) == INVALID_HANDLE)
	{
		return DoError(client, db, query, "Admin retrieval query failed");
	}
	
	if (SQL_GetRowCount(hQuery) > 0)
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin already exists");
		CloseHandle(hQuery);
		CloseHandle(db);
		return Plugin_Handled;
	}
	
	CloseHandle(hQuery);
	
	decl String:alias[64];
	decl String:safe_alias[140];
	GetCmdArg(1, alias, sizeof(alias));
	SQL_QuoteString(db, alias, safe_alias, sizeof(safe_alias));
	
	decl String:flags[30];
	decl String:safe_flags[64];
	GetCmdArg(4, flags, sizeof(flags));
	SQL_QuoteString(db, flags, safe_flags, sizeof(safe_flags));
	
	decl String:password[32];
	decl String:safe_password[80];
	if (args > 4)
	{
		GetCmdArg(5, password, sizeof(password));
		SQL_QuoteString(db, password, safe_password, sizeof(safe_password));
	} else {
		safe_password[0] = '\0';
	}
	
	new len = 0;
	len += Format(query[len], sizeof(query)-len, "INSERT INTO sm_admins (authtype, identity, password, flags, name) VALUES");
	if (safe_password[0] == '\0')
	{
		len += Format(query[len], sizeof(query)-len, " ('%s', '%s', NULL, '%s', '%s')", authtype, safe_identity, safe_flags, safe_alias);
	} else {
		len += Format(query[len], sizeof(query)-len, " ('%s', '%s', '%s', '%s', '%s')", authtype, safe_identity, safe_password, safe_flags, safe_alias);
	}
	
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin insertion query failed");
	}
	
	ReplyToCommand(client, "[SM] %t", "SQL Admin added");
	
	CloseHandle(db);
		
	return Plugin_Handled;
}

stock Action:DoError(client, Handle:db, const String:query[], const String:msg[])
{
		decl String:error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("%s: %s", msg, error);
		LogError("Query dump: %s", query);
		CloseHandle(db);
		ReplyToCommand(client, "[SM] %t", "Failed to query database");
		return Plugin_Handled;
}

stock Action:DoStmtError(client, Handle:db, const String:query[], const String:error[], const String:msg[])
{
		LogError("%s: %s", msg, error);
		LogError("Query dump: %s", query);
		CloseHandle(db);
		ReplyToCommand(client, "[SM] %t", "Failed to query database");
		return Plugin_Handled;
}

