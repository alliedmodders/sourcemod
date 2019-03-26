/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SQL Admin Manager Plugin
 * Adds/managers admins and groups in an SQL database.
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

#include <sourcemod>

/* We like semicolons */
#pragma semicolon 1
#pragma newdecls required

#define CURRENT_SCHEMA_VERSION		1409
#define SCHEMA_UPGRADE_1			1409

int current_version[] = {1, 0, 0, CURRENT_SCHEMA_VERSION};

public Plugin myinfo = 
{
	name = "SQL Admin Manager",
	author = "AlliedModders LLC",
	description = "Manages SQL admins",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("sqladmins.phrases");

	RegAdminCmd("sm_sql_addadmin", Command_AddAdmin, ADMFLAG_ROOT, "Adds an admin to the SQL database");
	RegAdminCmd("sm_sql_deladmin", Command_DelAdmin, ADMFLAG_ROOT, "Removes an admin from the SQL database");
	RegAdminCmd("sm_sql_addgroup", Command_AddGroup, ADMFLAG_ROOT, "Adds a group to the SQL database");
	RegAdminCmd("sm_sql_delgroup", Command_DelGroup, ADMFLAG_ROOT, "Removes a group from the SQL database");
	RegAdminCmd("sm_sql_setadmingroups", Command_SetAdminGroups, ADMFLAG_ROOT, "Sets an admin's groups in the SQL database");
	RegServerCmd("sm_create_adm_tables", Command_CreateTables);
	RegServerCmd("sm_update_adm_tables", Command_UpdateTables);
}

Database Connect()
{
	char error[255];
	Database db;
	
	if (SQL_CheckConfig("admins"))
	{
		db = SQL_Connect("admins", true, error, sizeof(error));
	} else {
		db = SQL_Connect("default", true, error, sizeof(error));
	}
	
	if (db == null)
	{
		LogError("Could not connect to database: %s", error);
	}
	
	return db;
}

void CreateMySQL(int client, Database db)
{
	char queries[][] = 
	{
		"CREATE TABLE sm_admins (id int(10) unsigned NOT NULL auto_increment, authtype enum('steam','name','ip') NOT NULL, identity varchar(65) NOT NULL, password varchar(65), flags varchar(30) NOT NULL, name varchar(65) NOT NULL, immunity int(10) unsigned NOT NULL, PRIMARY KEY (id))",
		"CREATE TABLE sm_groups (id int(10) unsigned NOT NULL auto_increment, flags varchar(30) NOT NULL, name varchar(120) NOT NULL, immunity_level int(1) unsigned NOT NULL, PRIMARY KEY (id))",
		"CREATE TABLE sm_group_immunity (group_id int(10) unsigned NOT NULL, other_id int(10) unsigned NOT NULL,  PRIMARY KEY (group_id, other_id))",
		"CREATE TABLE sm_group_overrides (group_id int(10) unsigned NOT NULL, type enum('command','group') NOT NULL, name varchar(32) NOT NULL, access enum('allow','deny') NOT NULL, PRIMARY KEY (group_id, type, name))",
		"CREATE TABLE sm_overrides (type enum('command','group') NOT NULL, name varchar(32) NOT NULL, flags varchar(30) NOT NULL, PRIMARY KEY (type,name))",
		"CREATE TABLE sm_admins_groups (admin_id int(10) unsigned NOT NULL, group_id int(10) unsigned NOT NULL, inherit_order int(10) NOT NULL, PRIMARY KEY (admin_id, group_id))",
		"CREATE TABLE IF NOT EXISTS sm_config (cfg_key varchar(32) NOT NULL, cfg_value varchar(255) NOT NULL, PRIMARY KEY (cfg_key))"
	};

	for (int i = 0; i < sizeof(queries); i++)
	{
		if (!DoQuery(client, db, queries[i]))
		{
			return;
		}
	}

	char query[256];
	Format(query, 
		sizeof(query), 
		"INSERT INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.%d') ON DUPLICATE KEY UPDATE cfg_value = '1.0.0.%d'",
		CURRENT_SCHEMA_VERSION,
		CURRENT_SCHEMA_VERSION);

	if (!DoQuery(client, db, query))
	{
		return;
	}

	ReplyToCommand(client, "[SM] Admin tables have been created.");
}

void CreateSQLite(int client, Database db)
{
	char queries[][] = 
	{
		"CREATE TABLE sm_admins (id INTEGER PRIMARY KEY AUTOINCREMENT, authtype varchar(16) NOT NULL CHECK(authtype IN ('steam', 'ip', 'name')), identity varchar(65) NOT NULL, password varchar(65), flags varchar(30) NOT NULL, name varchar(65) NOT NULL, immunity INTEGER NOT NULL)",
		"CREATE TABLE sm_groups (id INTEGER PRIMARY KEY AUTOINCREMENT, flags varchar(30) NOT NULL, name varchar(120) NOT NULL, immunity_level INTEGER NOT NULL)",
		"CREATE TABLE sm_group_immunity (group_id INTEGER NOT NULL, other_id INTEGER NOT NULL, PRIMARY KEY (group_id, other_id))",
		"CREATE TABLE sm_group_overrides (group_id INTEGER NOT NULL, type varchar(16) NOT NULL CHECK (type IN ('command', 'group')), name varchar(32) NOT NULL, access varchar(16) NOT NULL CHECK (access IN ('allow', 'deny')), PRIMARY KEY (group_id, type, name))",
		"CREATE TABLE sm_overrides (type varchar(16) NOT NULL CHECK (type IN ('command', 'group')), name varchar(32) NOT NULL, flags varchar(30) NOT NULL, PRIMARY KEY (type,name))",
		"CREATE TABLE sm_admins_groups (admin_id INTEGER NOT NULL, group_id INTEGER NOT NULL, inherit_order int(10) NOT NULL, PRIMARY KEY (admin_id, group_id))",
		"CREATE TABLE IF NOT EXISTS sm_config (cfg_key varchar(32) NOT NULL, cfg_value varchar(255) NOT NULL, PRIMARY KEY (cfg_key))"
	};

	for (int i = 0; i < sizeof(queries); i++)
	{
		if (!DoQuery(client, db, queries[i]))
		{
			return;
		}
	}

	char query[256];
	Format(query, 
		sizeof(query), 
		"REPLACE INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.%d')",
		CURRENT_SCHEMA_VERSION);

	if (!DoQuery(client, db, query))
	{
		return;
	}

	ReplyToCommand(client, "[SM] Admin tables have been created.");
}

public Action Command_CreateTables(int args)
{
	int client = 0;
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}

	char ident[16];
	db.Driver.GetIdentifier(ident, sizeof(ident));

	if (strcmp(ident, "mysql") == 0)
	{
		CreateMySQL(client, db);
	} else if (strcmp(ident, "sqlite") == 0) {
		CreateSQLite(client, db);
	} else {
		ReplyToCommand(client, "[SM] Unknown driver type '%s', cannot create tables.", ident);
	}

	delete db;

	return Plugin_Handled;
}

bool GetUpdateVersion(int client, Database db, int versions[4])
{
	char query[256];
	DBResultSet rs;

	Format(query, sizeof(query), "SELECT cfg_value FROM sm_config WHERE cfg_key = 'admin_version'");
	if ((rs = SQL_Query(db, query)) == null)
	{
		DoError(client, db, query, "Version lookup query failed");
		return false;
	}
	if (rs.FetchRow())
	{
		char version_string[255];
		rs.FetchString(0, version_string, sizeof(version_string));

		char version_numbers[4][12];
		if (ExplodeString(version_string, ".", version_numbers, 4, 12) == 4)
		{
			for (int i = 0; i < sizeof(version_numbers); i++)
			{
				versions[i] = StringToInt(version_numbers[i]);
			}
		}
	}

	delete rs;

	if (current_version[3] < versions[3])
	{
		ReplyToCommand(client, "[SM] The database is newer than the expected version.");
		return false;
	}

	if (current_version[3] == versions[3])
	{
		ReplyToCommand(client, "[SM] Your tables are already up to date.");
		return false;
	}

	return true;
}

void UpdateSQLite(int client, Database db)
{
	char query[512];
	DBResultSet rs;

	Format(query, sizeof(query), "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'sm_config'");
	if ((rs = SQL_Query(db, query)) == null)
	{
		DoError(client, db, query, "Table lookup query failed");
		return;
	}

	bool found = rs.FetchRow();

	delete rs;

	int versions[4];
	if (found)
	{
		if (!GetUpdateVersion(client, db, versions))
		{
			return;
		}
	}

	/* We only know about one upgrade path right now... 
	 * 0 => 1
	 */
	if (versions[3] < SCHEMA_UPGRADE_1)
	{
		char queries[][] = 
		{
			"ALTER TABLE sm_admins ADD immunity INTEGER DEFAULT 0 NOT NULL",
			"CREATE TABLE _sm_groups_temp (id INTEGER PRIMARY KEY AUTOINCREMENT, flags varchar(30) NOT NULL, name varchar(120) NOT NULL, immunity_level INTEGER DEFAULT 0 NOT NULL)",
			"INSERT INTO _sm_groups_temp (id, flags, name) SELECT id, flags, name FROM sm_groups",
			"UPDATE _sm_groups_temp SET immunity_level = 2 WHERE id IN (SELECT g.id FROM sm_groups g WHERE g.immunity = 'global')",
			"UPDATE _sm_groups_temp SET immunity_level = 1 WHERE id IN (SELECT g.id FROM sm_groups g WHERE g.immunity = 'default')",
			"DROP TABLE sm_groups",
			"ALTER TABLE _sm_groups_temp RENAME TO sm_groups",
			"CREATE TABLE IF NOT EXISTS sm_config (cfg_key varchar(32) NOT NULL, cfg_value varchar(255) NOT NULL, PRIMARY KEY (cfg_key))"
		};

		for (int i = 0; i < sizeof(queries); i++)
		{
			if (!DoQuery(client, db, queries[i]))
			{
				return;
			}
		}

		Format(query, 
			sizeof(query), 
			"REPLACE INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.%d')",
			SCHEMA_UPGRADE_1);

		if (!DoQuery(client, db, query))
		{
			return;
		}

		versions[3] = SCHEMA_UPGRADE_1;
	}

	ReplyToCommand(client, "[SM] Your tables are now up to date.");
}

void UpdateMySQL(int client, Database db)
{
	char query[512];
	DBResultSet rs;
	
	Format(query, sizeof(query), "SHOW TABLES");
	if ((rs = SQL_Query(db, query)) == null)
	{
		DoError(client, db, query, "Table lookup query failed");
		return;
	}

	char table[64];
	bool found = false;
	while (rs.FetchRow())
	{
		rs.FetchString(0, table, sizeof(table));
		if (strcmp(table, "sm_config") == 0)
		{
			found = true;
		}
	}
	delete rs;

	int versions[4];

	if (found && !GetUpdateVersion(client, db, versions))
	{
		return;
	}

	/* We only know about one upgrade path right now... 
	 * 0 => 1
	 */
	if (versions[3] < SCHEMA_UPGRADE_1)
	{
		char queries[][] = 
		{
			"CREATE TABLE IF NOT EXISTS sm_config (cfg_key varchar(32) NOT NULL, cfg_value varchar(255) NOT NULL, PRIMARY KEY (cfg_key))",
			"ALTER TABLE sm_admins ADD immunity INT UNSIGNED NOT NULL",
			"ALTER TABLE sm_groups ADD immunity_level INT UNSIGNED NOT NULL",
			"UPDATE sm_groups SET immunity_level = 2 WHERE immunity = 'default'",
			"UPDATE sm_groups SET immunity_level = 1 WHERE immunity = 'global'",
			"ALTER TABLE sm_groups DROP immunity"
		};

		for (int i = 0; i < sizeof(queries); i++)
		{
			if (!DoQuery(client, db, queries[i]))
			{
				return;
			}
		}

		char upgr[48];
		Format(upgr, sizeof(upgr), "1.0.0.%d", SCHEMA_UPGRADE_1);

		Format(query, sizeof(query), "INSERT INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '%s') ON DUPLICATE KEY UPDATE cfg_value = '%s'", upgr, upgr);
		if (!DoQuery(client, db, query))
		{
			return;
		}

		versions[3] = SCHEMA_UPGRADE_1;
	}

	ReplyToCommand(client, "[SM] Your tables are now up to date.");
}

public Action Command_UpdateTables(int args)
{
	int client = 0;
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}

	char ident[16];
	db.Driver.GetIdentifier(ident, sizeof(ident));

	if (strcmp(ident, "mysql") == 0)
	{
		UpdateMySQL(client, db);
	} else if (strcmp(ident, "sqlite") == 0) {
		UpdateSQLite(client, db);
	} else {
		ReplyToCommand(client, "[SM] Unknown driver type, cannot upgrade.");
	}

	delete db;

	return Plugin_Handled;
}

public Action Command_SetAdminGroups(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_setadmingroups <authtype> <identity> [group1] ... [group N]");
		return Plugin_Handled;
	}
	
	char authtype[16];
	GetCmdArg(1, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	char identity[65];
	char safe_identity[140];
	GetCmdArg(2, identity, sizeof(identity));
	db.Escape(identity, safe_identity, sizeof(safe_identity));
	
	char query[255];
	Format(query, 
		sizeof(query),
		"SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'",
		authtype,
		safe_identity);
		
	DBResultSet rs;
	if ((rs = SQL_Query(db, query)) == null)
	{
		return DoError(client, db, query, "Admin lookup query failed");
	}
	
	if (!rs.FetchRow())
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin not found");
		delete rs;
		delete db;
		return Plugin_Handled;
	}
	
	int id = rs.FetchInt(0);
	
	delete rs;
	
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
		delete db;
		ReplyToCommand(client, "[SM] %t", "SQL Admin groups reset");
		return Plugin_Handled;
	}
	
	char error[256];
	DBStatement hAddQuery, hFindQuery;
	
	Format(query, sizeof(query), "SELECT id FROM sm_groups WHERE name = ?");
	if ((hFindQuery = SQL_PrepareQuery(db, query, error, sizeof(error))) == null)
	{
		return DoStmtError(client, db, query, error, "Group search prepare failed");
	}
	
	Format(query, 
		sizeof(query), 
		"INSERT INTO sm_admins_groups (admin_id, group_id, inherit_order) VALUES (%d, ?, ?)",
		id);
	if ((hAddQuery = SQL_PrepareQuery(db, query, error, sizeof(error))) == null)
	{
		delete hFindQuery;
		return DoStmtError(client, db, query, error, "Add admin group prepare failed");
	}
	
	char name[80];
	int inherit_order = 0;
	for (int i = 3; i <= args; i++)
	{
		GetCmdArg(i, name, sizeof(name));
		
		hFindQuery.BindString(0, name, false);
		if (!SQL_Execute(hFindQuery) || !SQL_FetchRow(hFindQuery))
		{
			ReplyToCommand(client, "[SM] %t", "SQL Group X not found", name);
		} else {
			int gid = SQL_FetchInt(hFindQuery, 0);
			
			hAddQuery.BindInt(0, gid);
			hAddQuery.BindInt(1, ++inherit_order);
			if (!SQL_Execute(hAddQuery))
			{
				ReplyToCommand(client, "[SM] %t", "SQL Group X failed to bind", name);
				inherit_order--;
			}
		}
	}
	
	delete hAddQuery;
	delete hFindQuery;
	delete db;
	
	if (inherit_order == 1)
	{
		ReplyToCommand(client, "[SM] %t", "Added group to user");
	} else if (inherit_order > 1) {
		ReplyToCommand(client, "[SM] %t", "Added groups to user", inherit_order);
	}
	
	return Plugin_Handled;
}

public Action Command_DelGroup(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_delgroup <name>");
		return Plugin_Handled;
	}

	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	char name[80];
	char safe_name[180];
	GetCmdArgString(name, sizeof(name));
	
	/* Strip quotes in case the user tries to use them */
	int len = strlen(name);
	if (len > 1 && (name[0] == '"' && name[len-1] == '"'))
	{
		name[--len] = '\0';
		db.Escape(name[1], safe_name, sizeof(safe_name));
	} else {
		db.Escape(name, safe_name, sizeof(safe_name));
	}
	
	char query[256];
	
	DBResultSet rs;
	Format(query, sizeof(query), "SELECT id FROM sm_groups WHERE name = '%s'", safe_name);
	if ((rs = SQL_Query(db, query)) == null)
	{
		return DoError(client, db, query, "Group retrieval query failed");
	}
	
	if (!rs.FetchRow())
	{
		ReplyToCommand(client, "[SM] %t", "SQL Group not found");
		delete rs;
		delete db;
		return Plugin_Handled;
	}
	
	int id = rs.FetchInt(0);
	
	delete rs;
	
	/* Delete admin inheritance for this group */
	Format(query, sizeof(query), "DELETE FROM sm_admins_groups WHERE group_id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin group deletion query failed");
	}
	
	/* Delete group overrides */
	Format(query, sizeof(query), "DELETE FROM sm_group_overrides WHERE group_id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group override deletion query failed");
	}
	
	/* Delete immunity */
	Format(query, sizeof(query), "DELETE FROM sm_group_immunity WHERE group_id = %d OR other_id = %d", id, id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group immunity deletion query failed");
	}
	
	/* Finally delete the group */
	Format(query, sizeof(query), "DELETE FROM sm_groups WHERE id = %d", id);
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group deletion query failed");
	}
	
	ReplyToCommand(client, "[SM] %t", "SQL Group deleted");
	
	delete db;
	return Plugin_Handled;
}

public Action Command_AddGroup(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_addgroup <name> <flags> [immunity]");
		return Plugin_Handled;
	}

	int immunity;
	if (args >= 3)
	{
		char arg3[32];
		GetCmdArg(3, arg3, sizeof(arg3));
		if (!StringToIntEx(arg3, immunity))
		{
			ReplyToCommand(client, "[SM] %t", "Invalid immunity");
			return Plugin_Handled;
		}
	}
	
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	char name[64];
	char safe_name[64];
	GetCmdArg(1, name, sizeof(name));
	db.Escape(name, safe_name, sizeof(safe_name));
	
	DBResultSet rs;
	char query[256];
	Format(query, sizeof(query), "SELECT id FROM sm_groups WHERE name = '%s'", safe_name);
	if ((rs = SQL_Query(db, query)) == null)
	{
		return DoError(client, db, query, "Group retrieval query failed");
	}
	
	if (rs.RowCount > 0)
	{
		ReplyToCommand(client, "[SM] %t", "SQL Group already exists");
		delete rs;
		delete db;
		return Plugin_Handled;
	}
	
	delete rs;
	
	char flags[30];
	char safe_flags[64];
	GetCmdArg(2, flags, sizeof(safe_flags));
	db.Escape(flags, safe_flags, sizeof(safe_flags));
	
	Format(query, 
		sizeof(query),
		"INSERT INTO sm_groups (flags, name, immunity_level) VALUES ('%s', '%s', '%d')",
		safe_flags,
		safe_name,
		immunity);
	
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Group insertion query failed");
	}
	
	ReplyToCommand(client, "[SM] %t", "SQL Group added");
	
	delete db;
	return Plugin_Handled;
}	

public Action Command_DelAdmin(int client, int args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_deladmin <authtype> <identity>");
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	char authtype[16];
	GetCmdArg(1, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	char identity[65];
	char safe_identity[140];
	GetCmdArg(2, identity, sizeof(identity));
	db.Escape(identity, safe_identity, sizeof(safe_identity));
	
	char query[255];
	Format(query, 
		sizeof(query),
		"SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'",
		authtype,
		safe_identity);
		
	DBResultSet rs;
	if ((rs = SQL_Query(db, query)) == null)
	{
		delete db;
		return DoError(client, db, query, "Admin lookup query failed");
	}
	
	if (!rs.FetchRow())
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin not found");
		delete rs;
		delete db;
		return Plugin_Handled;
	}
	
	int id = rs.FetchInt(0);
	
	delete rs;
	
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
	
	delete db;
	
	ReplyToCommand(client, "[SM] %t", "SQL Admin deleted");
	
	return Plugin_Handled;
}

public Action Command_AddAdmin(int client, int args)
{
	if (args < 4)
	{
		ReplyToCommand(client, "[SM] Usage: sm_sql_addadmin <alias> <authtype> <identity> <flags> [immunity] [password]");
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}
	
	char authtype[16];
	GetCmdArg(2, authtype, sizeof(authtype));
	
	if (!StrEqual(authtype, "steam")
		&& !StrEqual(authtype, "ip")
		&& !StrEqual(authtype, "name"))
	{
		ReplyToCommand(client, "[SM] %t", "Invalid authtype");
		return Plugin_Handled;
	}

	int immunity;
	if (args >= 5)
	{
		char arg5[32];
		GetCmdArg(5, arg5, sizeof(arg5));
		if (!StringToIntEx(arg5, immunity))
		{
			ReplyToCommand(client, "[SM] %t", "Invalid immunity");
			return Plugin_Handled;
		}
	}
	
	char identity[65];
	char safe_identity[140];
	GetCmdArg(3, identity, sizeof(identity));
	
	char query[256];
	Database db = Connect();
	if (db == null)
	{
		ReplyToCommand(client, "[SM] %t", "Could not connect to database");
		return Plugin_Handled;
	}
	
	db.Escape(identity, safe_identity, sizeof(safe_identity));

	DBResultSet rs;
	
	Format(query, sizeof(query), "SELECT id FROM sm_admins WHERE authtype = '%s' AND identity = '%s'", authtype, identity);
	if ((rs = SQL_Query(db, query)) == null)
	{
		return DoError(client, db, query, "Admin retrieval query failed");
	}
	
	if (rs.RowCount > 0)
	{
		ReplyToCommand(client, "[SM] %t", "SQL Admin already exists");
		delete rs;
		delete db;
		return Plugin_Handled;
	}
	
	delete rs;
	
	char alias[64];
	char safe_alias[140];
	GetCmdArg(1, alias, sizeof(alias));
	db.Escape(alias, safe_alias, sizeof(safe_alias));
	
	char flags[30];
	char safe_flags[64];
	GetCmdArg(4, flags, sizeof(flags));
	db.Escape(flags, safe_flags, sizeof(safe_flags));
	
	char password[32];
	char safe_password[80];
	if (args >= 6)
	{
		GetCmdArg(6, password, sizeof(password));
		db.Escape(password, safe_password, sizeof(safe_password));
	} else {
		safe_password[0] = '\0';
	}
	
	int len = Format(query, sizeof(query), "INSERT INTO sm_admins (authtype, identity, password, flags, name, immunity) VALUES");
	if (safe_password[0] == '\0')
	{
		len += Format(query[len], sizeof(query)-len, " ('%s', '%s', NULL, '%s', '%s', %d)", authtype, safe_identity, safe_flags, safe_alias, immunity);
	} else {
		len += Format(query[len], sizeof(query)-len, " ('%s', '%s', '%s', '%s', '%s', %d)", authtype, safe_identity, safe_password, safe_flags, safe_alias, immunity);
	}
	
	if (!SQL_FastQuery(db, query))
	{
		return DoError(client, db, query, "Admin insertion query failed");
	}
	
	ReplyToCommand(client, "[SM] %t", "SQL Admin added");
	
	delete db;
	return Plugin_Handled;
}

stock bool DoQuery(int client, Database db, const char[] query)
{
	if (!SQL_FastQuery(db, query))
	{
		char error[255];
		SQL_GetError(db, error, sizeof(error));
		LogError("Query failed: %s", error);
		LogError("Query dump: %s", query);
		ReplyToCommand(client, "[SM] %t", "Failed to query database");
		return false;
	}

	return true;
}

stock Action DoError(int client, Database db, const char[] query, const char[] msg)
{
	char error[255];
	SQL_GetError(db, error, sizeof(error));
	LogError("%s: %s", msg, error);
	LogError("Query dump: %s", query);
	delete db;
	ReplyToCommand(client, "[SM] %t", "Failed to query database");
	return Plugin_Handled;
}

stock Action DoStmtError(int client, Database db, const char[] query, const char[] error, const char[] msg)
{
	LogError("%s: %s", msg, error);
	LogError("Query dump: %s", query);
	delete db;
	ReplyToCommand(client, "[SM] %t", "Failed to query database");
	return Plugin_Handled;
}
