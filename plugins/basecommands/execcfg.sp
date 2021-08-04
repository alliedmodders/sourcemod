/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod Basecommands Plugin
 * Provides exec cfg functionality
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

Menu g_ConfigMenu = null;

void PerformExec(int client, char[] path)
{
	if (!FileExists(path))
	{
		ReplyToCommand(client, "[SM] %t", "Config not found", path[4]);
		return;
	}

	ShowActivity2(client, "[SM] ", "%t", "Executed config", path[4]);

	LogAction(client, -1, "\"%L\" executed config (file \"%s\")", client, path[4]);

	ServerCommand("exec \"%s\"", path[4]);
}

public void AdminMenu_ExecCFG(TopMenu topmenu, 
					  TopMenuAction action,
					  TopMenuObject object_id,
					  int param,
					  char[] buffer,
					  int maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Exec CFG", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		g_ConfigMenu.Display(param, MENU_TIME_FOREVER);
	}
}

public int MenuHandler_ExecCFG(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu)
		{
			hTopMenu.Display(param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		char path[256];
		
		menu.GetItem(param2, path, sizeof(path));
	
		PerformExec(param1, path);
	}
	else if (action == MenuAction_Display)
	{
		char title[128];
		Format(title, sizeof(title), "%T", "Choose Config", param1);

		Panel panel = view_as<Panel>(param2);
		panel.SetTitle(title);
	}

	return 0;
}

public Action Command_ExecCfg(int client, int args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_execcfg <filename>");
		return Plugin_Handled;
	}

	char path[64] = "cfg/";
	GetCmdArg(1, path[4], sizeof(path)-4);

	PerformExec(client, path);

	return Plugin_Handled;
}

SMCParser config_parser;
void ParseConfigs()
{
	if (!config_parser)
		config_parser = new SMCParser();
	
	config_parser.OnEnterSection = NewSection;
	config_parser.OnLeaveSection = EndSection;
	config_parser.OnKeyValue = KeyValue;
	
	if (g_ConfigMenu != null)
	{
		delete g_ConfigMenu;
	}
	
	g_ConfigMenu = new Menu(MenuHandler_ExecCFG, MenuAction_Display);
	g_ConfigMenu.SetTitle("%T", "Choose Config", LANG_SERVER);
	g_ConfigMenu.ExitBackButton = true;
	
	char configPath[256];
	BuildPath(Path_SM, configPath, sizeof(configPath), "configs/adminmenu_cfgs.txt");
	
	if (!FileExists(configPath))
	{
		LogError("Unable to locate exec config file, no maps loaded.");
			
		return;		
	}
	
	int line;
	SMCError err = config_parser.ParseFile(configPath, line);
	if (err != SMCError_Okay)
	{
		char error[256];
		SMC_GetErrorString(err, error, sizeof(error));
		LogError("Could not parse file (line %d, file \"%s\"):", line, configPath);
		LogError("Parser encountered error: %s", error);
	}
	
	return;
}

public SMCResult NewSection(SMCParser smc, const char[] name, bool opt_quotes)
{
	return SMCParse_Continue;
}

public SMCResult KeyValue(SMCParser smc, const char[] key, const char[] value, bool key_quotes, bool value_quotes)
{
	g_ConfigMenu.AddItem(key, value);

	return SMCParse_Continue;
}

public SMCResult EndSection(SMCParser smc)
{
	return SMCParse_Continue;
}
