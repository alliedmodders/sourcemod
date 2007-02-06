#define OVERRIDE_STATE_NONE			0
#define OVERRIDE_STATE_LEVELS		1
#define OVERRIDE_STATE_OVERRIDES	2

static Handle:g_hOverrideParser = INVALID_HANDLE;
static g_OverrideState = OVERRIDE_STATE_NONE;

static LogOverrideError(const String:format[], {Handle,String,Float,_}:...)
{
	decl String:buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing admin_levels.cfg:");
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (%d) %s", ++g_ErrorCount, buffer);
}


public SMCResult:ReadOverrides_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_OverrideState == OVERRIDE_STATE_NONE)
	{
		if (StrEqual(name, "Levels"))
		{
			g_OverrideState = OVERRIDE_STATE_LEVELS;
		}
	} else if (g_OverrideState == OVERRIDE_STATE_LEVELS) {
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OVERRIDE_STATE_OVERRIDES;
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOverrides_KeyValue(Handle:smc, 
										const String:key[], 
										const String:value[], 
										bool:key_quotes, 
										bool:value_quotes)
{
	if (g_OverrideState != OVERRIDE_STATE_OVERRIDES)
	{
		return SMCParse_Continue;
	}
	
	new AdminFlag:flag;
	
	if (strlen(value) > 1)
	{
		LogOverrideError("Unrecognized access level: %s", value);
		return SMCParse_Continue;
	}
	
	if (value[0] >= 'a' && value[0] <= 'z')
	{
		flag = g_FlagLetters[value[0] - 'a'];
	}
	
	if (key[0] == '@')
	{
		AddCommandOverride(key[1], Override_CommandGroup, FlagToBit(flag));
	} else {
		AddCommandOverride(key, Override_Command, FlagToBit(flag));
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOverrides_EndSection(Handle:smc)
{
	if (g_OverrideState == OVERRIDE_STATE_LEVELS)
	{
		g_OverrideState = OVERRIDE_STATE_NONE;
	} else if (g_OverrideState == OVERRIDE_STATE_OVERRIDES) {
		/* We're totally done parsing */
		g_OverrideState = OVERRIDE_STATE_LEVELS;
		return SMCParse_Halt;
	}
	
	return SMCParse_Continue;
}

static InitializeOverrideParser()
{
	if (g_hOverrideParser == INVALID_HANDLE)
	{
		g_hOverrideParser = SMC_CreateParser();
		SMC_SetReaders(g_hOverrideParser,
					   ReadOverrides_NewSection,
					   ReadOverrides_KeyValue,
					   ReadOverrides_EndSection);
	}
}

ReadOverrides()
{
	new String:path[PLATFORM_MAX_PATH];
	
	InitializeOverrideParser();
	
	BuildPath(Path_SM, path, sizeof(path), "configs/admin_levels.cfg");
	
	/* Set states */
	g_OverrideState = OVERRIDE_STATE_NONE;
	g_LoggedFileName = false;
	g_ErrorCount = 0;
		
	new SMCError:err = SMC_ParseFile(g_hOverrideParser, path);
	if (err != SMCError_Okay)
	{
		decl String:buffer[64];
		if (SMC_GetErrorString(err, buffer, sizeof(buffer)))
		{
			LogOverrideError("%s", buffer);
		} else {
			LogOverrideError("Fatal parse error");
		}
	}
}

