#define OVERRIDE_STATE_NONE			0
#define OVERRIDE_STATE_LEVELS		1
#define OVERRIDE_STATE_OVERRIDES	2

static Handle:g_hOverrideParser = INVALID_HANDLE;
static g_OverrideState = OVERRIDE_STATE_NONE;

public SMCResult:ReadOverrides_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel++;
		return SMCParse_Continue;
	}
	
	if (g_OverrideState == OVERRIDE_STATE_NONE)
	{
		if (StrEqual(name, "Levels"))
		{
			g_OverrideState = OVERRIDE_STATE_LEVELS;
		} else {
			g_IgnoreLevel++;
		}
	} else if (g_OverrideState == OVERRIDE_STATE_LEVELS) {
		if (StrEqual(name, "Overrides"))
		{
			g_OverrideState = OVERRIDE_STATE_OVERRIDES;
		} else {
			g_IgnoreLevel++;
		}
	} else {
		g_IgnoreLevel++;
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOverrides_KeyValue(Handle:smc, 
										const String:key[], 
										const String:value[], 
										bool:key_quotes, 
										bool:value_quotes)
{
	if (g_OverrideState != OVERRIDE_STATE_OVERRIDES
		|| g_IgnoreLevel)
	{
		return SMCParse_Continue;
	}
	
	new AdminFlag:array[AdminFlags_TOTAL];
	new flags_total;
	
	new len = strlen(value);
	for (new i=0; i<len; i++)
	{
		if (value[i] < 'a' || value[i] > 'z')
		{
			ParseError("Invalid flag detected: %c", value[i]);
			continue;
		}
		new val = value[i] - 'a';
		if (!g_FlagsSet[val])
		{
			ParseError("Invalid flag detected: %c", value[i]);
			continue;
		}
		array[flags_total++] = g_FlagLetters[val];
	}
	
	new flags = FlagArrayToBits(array, flags_total);
	
	if (key[0] == '@')
	{
		AddCommandOverride(key[1], Override_CommandGroup, flags);
	} else {
		AddCommandOverride(key, Override_Command, flags);
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadOverrides_EndSection(Handle:smc)
{
	/* If we're ignoring, skip out */
	if (g_IgnoreLevel)
	{
		g_IgnoreLevel--;
		return SMCParse_Continue;
	}
	
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
	InitializeOverrideParser();
	
	BuildPath(Path_SM, g_Filename, sizeof(g_Filename), "configs/admin_levels.cfg");
	
	/* Set states */
	InitGlobalStates();
	g_OverrideState = OVERRIDE_STATE_NONE;
		
	new SMCError:err = SMC_ParseFile(g_hOverrideParser, g_Filename);
	if (err != SMCError_Okay)
	{
		decl String:buffer[64];
		if (SMC_GetErrorString(err, buffer, sizeof(buffer)))
		{
			ParseError("%s", buffer);
		} else {
			ParseError("Fatal parse error");
		}
	}
}
