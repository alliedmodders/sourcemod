#define GROUP_STATE_NONE		0
#define GROUP_STATE_GROUPS		1
#define GROUP_STATE_INGROUP		2
#define GROUP_STATE_OVERRIDES	3
#define GROUP_PASS_FIRST		1
#define GROUP_PASS_SECOND		2

static Handle:g_hGroupParser = INVALID_HANDLE;
static GroupId:g_CurGrp = INVALID_GROUP_ID;
static g_GroupState = GROUP_STATE_NONE;
static g_GroupPass = 0

static LevelGroupError(const String:buffer[], {Handle,String,Float,_}:...)
{
	decl String:buffer[512];
	
	if (!g_LoggedFileName)
	{
		LogError("Error(s) detected parsing admin_groups.cfg:");
		g_LoggedFileName = true;
	}
	
	VFormat(buffer, sizeof(buffer), format, 2);
	
	LogError(" (%d) %s", ++g_ErrorCount, buffer);
}

public SMCResult:ReadGroups_NewSection(Handle:smc, const String:name[], bool:opt_quotes)
{
	if (g_GroupState == GROUP_STATE_NONE)
	{
		if (StrEqual(name, "Groups"))
		{
			g_GroupState = GROUP_STATE_GROUPS;
		}
	} else if (g_GroupState == GROUP_STATE_GROUPS) {
		if ((g_CurGrp = CreateAdmGroup(name)) == INVALID_GROUP_ID)
		{
			g_CurGrp = FindAdmGroup(name);
		}
		g_GroupState = GROUP_STATE_INGROUP;
	} else if (g_GroupState == GROUP_STATE_INGROUP) {
		if (StrEqual(name, "Overrides"))
		{
			g_GroupState = GROUP_STATE_OVERRIDES;
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadGroups_KeyValue(Handle:smc, 
										const String:key[], 
										const String:value[], 
										bool:key_quotes, 
										bool:value_quotes)
{
	if (g_CurGrp == INVALID_GROUP_ID)
	{
		return SMCParse_Continue;
	}
	
	new AdminFlag:flag;
	
	if (StrEqual(key, "flags"))
	{
		new len = strlen(value);
		for (new i=0; i<len; i++)
		{
			if (value[i] < 'a' || value[i] > 'z')
			{
				continue;
			}
			flag = g_FlagLetters[value[i] - 'a'];
			if (flag == Admin_None)
			{
				continue;
			}
			SetAdmGroupAddFlag(g_CurGrp, flag, true);
		}
	} else if (StrEqual(key, "immunity")) {
		if (StrEqual(value, "*"))
		{
			SetAdmGroupImmunity(g_CurGrp, Immunity_Global, true);
		} else if (StrEqual(value, "$")) {
			SetAdmGroupImmunity(g_CurGrp, Immunity_Default, true);
		}
	}
	
	return SMCParse_Continue;
}

public SMCResult:ReadGroups_EndSection(Handle:smc)
{
	if (g_GroupState == GROUP_STATE_OVERRIDES)
	{
		g_GroupState = GROUP_STATE_INGROUP;
	} else if (g_GroupState == GROUP_STATE_INGROUP) {
		g_GroupState = GROUP_STATE_GROUPS;
		g_CurGrp = INVALID_GROUP_ID;
	} else if (g_GroupState == GROUP_STATE_GROUPS) {
		g_GroupState = GROUP_STATE_NONE;
	}
	
	return SMCParse_Continue;
}

static InitializeGroupParser()
{
	if (g_hGroupParser == INVALID_HANDLE)
	{
		g_hGroupParser = SMC_CreateParser();
		SMC_SetReaders(g_hGroupParser,
					   ReadGroup_NewSection,
					   ReadGroup_KeyValue,
					   ReadGroup_EndSection);
	}
}

ReadGroups()
{
	new String:path[PLATFORM_MAX_PATH];
	
	InitializeGroupParser();
	
	BuildPath(Path_SM, path, sizeof(path), "configs/admin_groups.cfg");
	
	/* Set states */
	g_GroupState = GROUP_STATE_NONE;
	g_LoggedFileName = false;
	g_ErrorCount = 0;
	g_CurGrp = INVALID_GROUP_ID;
	g_GroupPass = GROUP_PASS_FIRST;
		
	new SMCError:err = SMC_ParseFile(g_hGroupParser, path);
	if (err != SMCError_Okay)
	{
		decl String:buffer[64];
		if (SMC_GetErrorString(err, buffer, sizeof(buffer)))
		{
			LogGroupError("%s", buffer);
		} else {
			LogGroupError("Fatal parse error");
		}
	}
}

