new Handle:g_BeaconTimers[MAXPLAYERS+1] = {INVALID_HANDLE, ...};

new g_beamSprite;
new g_haloSprite;

SetupBeacon()
{
	HookEvent("player_death", Event_BeaconPlayerDeath, EventHookMode_Post);
	HookEvent("round_end", Event_BeaconRoundEnd, EventHookMode_PostNoCopy);	

	PrecacheSound("ambient/tones/elev1.wav", true);
	//AddFileToDownloadsTable("sound/ambient/tones/elev1.wav");

        g_beamSprite = PrecacheModel("materials/sprites/laser.vmt");
	g_haloSprite = PrecacheModel("materials/sprites/halo01.vmt");	
}

CreateBeacon(client)
{
	g_BeaconTimers[client] = CreateTimer(2.0, Timer_Beacon, client, TIMER_REPEAT);	
}

KillBeacon(client)
{
	KillTimer(g_BeaconTimers[client]);
	g_BeaconTimers[client] = INVALID_HANDLE;
}

KillAllBeacons()
{
	new maxclients = GetMaxClients();
	for (new i = 1; i <= maxclients; i++)
	{
		if (g_BeaconTimers[i] != INVALID_HANDLE)
		{
			KillBeacon(i);
		}
	}
}

PerformBeacon(client, target)
{
	LogAction(client, target, "\"%L\" set a beacon on \"%L\"", client, target);
	
	if (g_BeaconTimers[target] == INVALID_HANDLE)
	{
		CreateBeacon(target);
		LogAction(client, target, "\"%L\" set a beacon on \"%L\"", client, target);
	}
	else
	{
		KillBeacon(target);
		LogAction(client, target, "\"%L\" removed a beacon on \"%L\"", client, target);
	}
}

DisplayBeacon(client, r, g, b, a)
{
	new Float:vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 5;

	TE_Start("BeamRingPoint");
	TE_WriteVector("m_vecCenter", vec);
	TE_WriteFloat("m_flStartRadius", 20.0);
	TE_WriteFloat("m_flEndRadius", 400.0);
	TE_WriteNum("m_nModelIndex", g_beamSprite);
	TE_WriteNum("m_nHaloIndex", g_haloSprite);
	TE_WriteNum("m_nStartFrame", 0);
	TE_WriteNum("m_nFrameRate", 0);
	TE_WriteFloat("m_fLife", 1.0);
	TE_WriteFloat("m_fWidth", 5.0);
	TE_WriteFloat("m_fEndWidth", 5.0);
	TE_WriteFloat("m_fAmplitude", 0.0);
	TE_WriteNum("r", r);
	TE_WriteNum("g", g);
	TE_WriteNum("b", b);
	TE_WriteNum("a", a);
	TE_WriteNum("m_nSpeed", 50);
	TE_WriteNum("m_nFlags", 0);
	TE_WriteNum("m_nFadeLength", 0);
	TE_SendToAll();
}

public Action:Timer_Beacon(Handle:timer, any:client)
{
        if (!IsClientInGame(client) || !IsPlayerAlive(client))
	{
		KillBeacon(client);		
		return Plugin_Handled;
        }
	
	new team = GetClientTeam(client);
	
        if (team == 2)
	{
		DisplayBeacon(client, 255, 75, 75, 255);
	}
	else if (team == 3)
	{
		DisplayBeacon(client, 75, 75, 255, 255);
	}
	
	// Create a double ring, if we are the repeating timer.
	if (g_BeaconTimers[client] == timer)
	{
		CreateTimer(0.2, Timer_Beacon, client);

		new Float:vec[3];
		GetClientEyePosition(client, vec);
		EmitAmbientSound("ambient/tones/elev1.wav", vec, client, SNDLEVEL_RAIDSIREN);	
	}
		
	return Plugin_Handled;
}

public Action:Event_BeaconPlayerDeath(Handle:event,const String:name[],bool:dontBroadcast)
{
	new client = GetClientOfUserId(GetEventInt(event, "userid"));
	if (g_BeaconTimers[client] != INVALID_HANDLE)
	{
		KillBeacon(client);
	}

	return Plugin_Continue;
}

public Action:Event_BeaconRoundEnd(Handle:event,const String:name[],bool:dontBroadcast)
{
	KillAllBeacons();
	
        return Plugin_Handled;
}

DisplayBeaconMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Beacon);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Beacon player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public AdminMenu_Beacon(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Beacon player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBeaconMenu(param);
	}
}

public MenuHandler_Beacon(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32];
		new userid, target;
		
		GetMenuItem(menu, param2, info, sizeof(info));
		userid = StringToInt(info);

		if ((target = GetClientOfUserId(userid)) == 0)
		{
			PrintToChat(param1, "[SM] %t", "Player no longer available");
		}
		else if (!CanUserTarget(param1, target))
		{
			PrintToChat(param1, "[SM] %t", "Unable to target");
		}
		else
		{
			new String:name[32];
			GetClientName(target, name, sizeof(name));
			
			PerformBeacon(param1, target);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled beacon on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBeaconMenu(param1);
		}
	}
}

public Action:Command_Beacon(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_beacon <#userid|name>");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[MAXPLAYERS], target_count, bool:tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_ALIVE,
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
	
	for (new i = 0; i < target_count; i++)
	{
		PerformBeacon(client, target_list[i]);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
