PerformExec(client, String:path[])
{
	if (!FileExists(path))
	{
		ReplyToCommand(client, "[SM] %t", "Config not found", path[4]);
		return;
	}

	ShowActivity(client, "%t", "Executed config", path[4]);

	LogAction(client, -1, "\"%L\" executed config (file \"%s\")", client, path[4]);

	ServerCommand("exec \"%s\"", path[4]);
}

public AdminMenu_ExecCFG(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Exec CFG", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayMenu(g_ConfigList, param, MENU_TIME_FOREVER);
	}
}

public MenuHandler_ExecCFG(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:path[256];
		
		GetMenuItem(menu, param2, path, sizeof(path));
	
		PerformExec(param1, path);
	}
}

public Action:Command_ExecCfg(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_execcfg <filename>");
		return Plugin_Handled;
	}

	new String:path[64] = "cfg/";
	GetCmdArg(1, path[4], sizeof(path)-4);

	PerformExec(client, path);

	return Plugin_Handled;
}

ParseConfigs()
{
	new Handle:list = SMC_CreateParser();
	SMC_SetReaders(list, NewSection, KeyValue, EndSection);
	SMC_SetParseEnd(list, ParseEnd);
	
	new Handle:menu = CreateMenu(MenuHandler_ExecCFG);
	SetMenuTitle(menu, "Choose Config");
	SetMenuExitBackButton(menu, true);
	
	decl String:configPath[256];
	BuildPath(Path_SM, configPath, sizeof(configPath), "configs/menu_configs.cfg");
	
	if (!FileExists(configPath))
	{
		LogError("Unable to locate exec config file, no maps loaded.");
			
		return;		
	}
	
	g_ConfigList = menu;
	
	SMC_ParseFile(list, configPath);
	
	return;
}

public SMCResult:NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{

}

public SMCResult:KeyValue(Handle:smc, const String:key[], const String:value[], bool:key_quotes, bool:value_quotes)
{
	AddMenuItem(g_ConfigList, key, value);
}

public SMCResult:EndSection(Handle:smc)
{
	
}

public ParseEnd(Handle:smc, bool:halted, bool:failed)
{
	if (halted || failed)
		LogError("Reading of configs file failed");
		
	CloseHandle(smc);
}