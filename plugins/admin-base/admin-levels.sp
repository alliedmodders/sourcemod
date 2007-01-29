#define LEVEL_STATE_NONE		0
#define LEVEL_STATE_LEVELS		1
#define LEVEL_STATE_FLAGS		2

new Handle:g_hLevelParser = INVALID_HANDLE;
new g_LevelState = LEVEL_STATE_NONE;

/* :TODO: log line numbers? */

InitializeLevelParser()
{
	if (g_hLevelParser == INVALID_HANDLE)
	{
		g_hLevelParser = SMC_CreateParser();
		SMC_SetReaders(g_hLevelParser, 
				   	ReadLevels_NewSection,
				   	ReadLevels_KeyValue,
				   	ReadLevels_EndSection);
	}
}

LoadDefaultLetters()
{
	g_FlagLetters['a'-'a'] = Admin_Reservation;
	g_FlagLetters['b'-'a'] = Admin_Kick;
	g_FlagLetters['c'-'a'] = Admin_Ban;
	g_FlagLetters['d'-'a'] = Admin_Unban;
	g_FlagLetters['e'-'a'] = Admin_Slay;
	g_FlagLetters['f'-'a'] = Admin_Changemap;
	g_FlagLetters['g'-'a'] = Admin_Convars;
	g_FlagLetters['h'-'a'] = Admin_Configs;
	g_FlagLetters['i'-'a'] = Admin_Chat;
	g_FlagLetters['j'-'a'] = Admin_Vote;
	g_FlagLetters['h'-'a'] = Admin_Password;
	g_FlagLetters['i'-'a'] = Admin_RCON;
	g_FlagLetters['j'-'a'] = Admin_Cheats;
	g_FlagLetters['z'-'a'] = Admin_Root;
}

stock LogLevelError(const String:format[], {Handle,String,Float,_}:...)
{
	decl String:buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing admin_level.cfg:");
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (%d) %s", ++g_ErrorCount, buffer);
}

public SMCResult:ReadLevels_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_LevelState == LEVEL_STATE_NONE)
	{
		if (StrEqual(name, "Levels"))
		{
			g_LevelState = LEVEL_STATE_LEVELS;
		}
	} else if (g_LevelState == LEVEL_STATE_LEVELS) {
		if (StrEqual(name, "Flags"))
		{
			g_LevelState = LEVEL_STATE_FLAGS;
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadLevels_KeyValue(Handle:smc, const String:key[], const String:value[], bool:key_quotes, bool:value_quotes)
{
	if (g_LevelState == LEVEL_STATE_FLAGS)
	{
		new chr = value[0];
		
		if (chr < 'a' || chr > 'z')
		{
			LogLevelError("Unrecognized character: \"%s\"", value);
			return SMCParse_Continue;
		}
		
		chr -= 'a';
		
		new AdminFlag:flag = Admin_None;
		
		if (StrEqual(key, "reservation"))
		{
			flag = Admin_Reservation;
		} else if (StrEqual(key, "kick")) {
			flag = Admin_Kick;
		} else if (StrEqual(key, "ban")) {
			flag = Admin_Ban;
		} else if (StrEqual(key, "unban")) {
			flag = Admin_Unban;
		} else if (StrEqual(key, "slay")) {
			flag = Admin_Slay;
		} else if (StrEqual(key, "changemap")) {
			flag = Admin_Changemap;
		} else if (StrEqual(key, "cvars")) {
			flag = Admin_Convars;
		} else if (StrEqual(key, "configs")) {
			flag = Admin_Configs;
		} else if (StrEqual(key, "chat")) {
			flag = Admin_Chat;
		} else if (StrEqual(key, "vote")) {
			flag = Admin_Vote;
		} else if (StrEqual(key, "password")) {
			flag = Admin_Password;
		} else if (StrEqual(key, "rcon")) {
			flag = Admin_RCON;
		} else if (StrEqual(key, "cheats")) {
			flag = Admin_Cheats;
		} else if (StrEqual(key, "root")) {
			flag = Admin_Root;
		} else {
			LogLevelError("Unrecognized flag type: %s", key);
		}
		
		g_FlagLetters[chr] = flag;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadLevels_EndSection(Handle:smc)
{
	if (g_LevelState == LEVEL_STATE_FLAGS)
	{
		g_LevelState = LEVEL_STATE_LEVELS;
	} else if (g_LevelState == LEVEL_STATE_LEVELS) {
		g_LevelState = LEVEL_STATE_NONE;
	}
}

RefreshLevels()
{
	new String:path[PLATFORM_MAX_PATH];
	
	LoadDefaultLetters();
	InitializeLevelParser();
	
	BuildPath(Path_SM, path, sizeof(path), "configs/admin_levels.cfg");
	
	/* Set states */
	g_LevelState = LEVEL_STATE_NONE;
	g_LoggedFileName = false;
	g_ErrorCount = 0;
		
	new SMCError:err = SMC_ParseFile(g_hLevelParser, path);
	if (err != SMCError_Okay)
	{
		decl String:buffer[64];
		if (SMC_GetErrorString(err, buffer, sizeof(buffer)))
		{
			LogLevelError("%s", buffer);
		} else {
			LogLevelError("Fatal parse error");
		}
	}
}
