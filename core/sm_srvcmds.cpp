#include "sm_srvcmds.h"
#include "sm_version.h"
#include "sm_stringutil.h"

ConVarAccessor g_ConCmdAccessor;

void ConVarAccessor::OnSourceModStartup(bool late)
{
	ConCommandBaseMgr::OneTimeInit(&g_ConCmdAccessor);
}

bool ConVarAccessor::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);

	return true;
}

inline const char *StatusToStr(PluginStatus st)
{
	switch (st)
	{
	case Plugin_Running:
		return "Running";
	case Plugin_Paused:
		return "Paused";
	case Plugin_Error:
		return "Error";
	case Plugin_Uncompiled:
		return "Uncompiled";
	case Plugin_BadLoad:
		return "Bad Load";
	default:
		assert(false);
		return "-";
	}
}

#define IS_STR_FILLED(var) (var[0] != '\0')
CON_COMMAND(sm, "SourceMod Menu")
{
	int argnum = engine->Cmd_Argc();

	if (argnum >= 2)
	{
		const char *cmd = engine->Cmd_Argv(1);
		if (!strcmp("plugins", cmd))
		{
			if (argnum >= 3)
			{
				const char *cmd2 = engine->Cmd_Argv(2);
				if (!strcmp("list", cmd2))
				{
					char buffer[256];
					unsigned int id = 1;
					int plnum = g_PluginSys.GetPluginCount();

					if (!plnum)
					{
						META_CONPRINT("[SM] No plugins loaded\n");
						return;
					} else {
						META_CONPRINTF("[SM] Displaying %d plugin%s:\n", g_PluginSys.GetPluginCount(), (plnum > 1) ? "s" : "");
					}

					IPluginIterator *iter = g_PluginSys.GetPluginIterator();
					for (; iter->MorePlugins(); iter->NextPlugin(), id++)
					{
						IPlugin *pl = iter->GetPlugin();
						assert(pl->GetStatus() != Plugin_Created);
						int len = 0;
						const sm_plugininfo_t *info = pl->GetPublicInfo();

						len += UTIL_Format(buffer, sizeof(buffer), "  %02d <%s>", id, StatusToStr(pl->GetStatus()));
						len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " \"%s\"", (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
						if (IS_STR_FILLED(info->version))
						{
							len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
						}
						if (IS_STR_FILLED(info->author))
						{
							UTIL_Format(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
						}
						META_CONPRINTF("%s\n", buffer);
					}

					iter->Release();
					return;
				} else if (!strcmp("load", cmd2)) {
					if (argnum < 4)
					{
						META_CONPRINT("Usage: sm plugins load <file>\n");
						return;
					}

					char error[128];
					const char *filename = engine->Cmd_Argv(3);
					IPlugin *pl = g_PluginSys.LoadPlugin(filename, false, PluginType_MapUpdated, error, sizeof(error));

					if (pl)
					{
						META_CONPRINTF("Loaded plugin %s successfully.\n", filename);
					} else {
						META_CONPRINTF("Plugin %s failed to load: %s.\n", filename, error);
					}

					return;
				} else if (!strcmp("unload", cmd2)) {
					if (argnum < 4)
					{
						META_CONPRINT("Usage: sm plugins unload <#>\n");
						return;
					}

					int id = 1;
					int num = atoi(engine->Cmd_Argv(3));
					if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
					{
						META_CONPRINT("Plugin index not found.\n");
						return;
					}

					IPluginIterator *iter = g_PluginSys.GetPluginIterator();
					for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}
					IPlugin *pl = iter->GetPlugin();

					char name[64];
					const sm_plugininfo_t *info = pl->GetPublicInfo();
					strcpy(name, (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());

					if (g_PluginSys.UnloadPlugin(pl))
					{
						META_CONPRINTF("Plugin %s unloaded successfully.\n", name);
					} else {
						META_CONPRINTF("Failed to unload plugin %s.\n", name);
					}

					iter->Release();
					return;
				} else if (!strcmp("info", cmd2)) {
					if (argnum < 4)
					{
						META_CONPRINT("Usage: sm plugins info <#>\n");
						return;
					}

					int id = 1;
					int num = atoi(engine->Cmd_Argv(3));
					if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
					{
						META_CONPRINT("Plugin index not found.\n");
						return;
					}

					IPluginIterator *iter = g_PluginSys.GetPluginIterator();
					for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}

					IPlugin *pl = iter->GetPlugin();
					const sm_plugininfo_t *info = pl->GetPublicInfo();

					META_CONPRINTF("  Filename: %s\n", pl->GetFilename());
					if (IS_STR_FILLED(info->name))
					{
						META_CONPRINTF("  Title: %s\n", info->name);
					}
					if (IS_STR_FILLED(info->author))
					{
						META_CONPRINTF("  Author: %s\n", info->author);
					}
					if (IS_STR_FILLED(info->version))
					{
						META_CONPRINTF("  Version: %s\n", info->version);
					}
					if (IS_STR_FILLED(info->description))
					{
						META_CONPRINTF("  Description: %s\n", info->description);
					}
					if (IS_STR_FILLED(info->url))
					{
						META_CONPRINTF("  URL: %s\n", info->url);
					}
					//:TODO: write if it's in debug mode, or do it inside LIST ?

					iter->Release();
					return;
				}
			}

			META_CONPRINT(" SourceMod Plugin Menu:\n");
			META_CONPRINT("    list   - Show loaded plugins\n");
			META_CONPRINT("    load   - Load a plugin\n");
			META_CONPRINT("    unload - Unload a plugin\n");
			META_CONPRINT("    info   - Information about a plugin\n");
			return;
		} else if (!strcmp("version", cmd)) {
			META_CONPRINT(" SourceMod Version Information:\n");
			META_CONPRINTF("    SourceMod Version: \"%s\"\n", SOURCEMOD_VERSION);
			META_CONPRINTF("    JIT Version: %s (%s)\n", g_pVM->GetVMName(), g_pVM->GetVersionString());
			META_CONPRINTF("    JIT Settings: %s\n", g_pVM->GetCPUOptimizations());
			META_CONPRINTF("    http://www.sourcemod.net/\n");
			return;
		}
	}
	META_CONPRINT(" SourceMod Menu:\n");
	META_CONPRINT(" Usage: sm <command> [arguments]\n");
	META_CONPRINT("    plugins - Plugins menu\n");
	META_CONPRINT("    version - Display version information\n");
}