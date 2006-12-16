#include "sm_srvcmds.h"

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
						assert(iter->GetPlugin()->GetStatus() != Plugin_Created);
						int len = 0;
						const sm_plugininfo_t *info = iter->GetPlugin()->GetPublicInfo();

						len += snprintf(&buffer[len], sizeof(buffer)-len, "  %02d <%s>", id, "status"); //:TODO: status
						len += snprintf(&buffer[len], sizeof(buffer)-len, " \"%s\"", (info->name) ? info->name : iter->GetPlugin()->GetFilename());
						if (info->version)
						{
							len += snprintf(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
						}
						if (info->author)
						{
							snprintf(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
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

					char error[100];
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

					IPlugin *pl = NULL;
					int id = 1;
					int num = atoi(engine->Cmd_Argv(3));
					if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
					{
						META_CONPRINT("Plugin index not found.\n");
						return;
					}

					IPluginIterator *iter = g_PluginSys.GetPluginIterator();
					for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}
					pl = iter->GetPlugin();

					char name[64];
					const sm_plugininfo_t *info = pl->GetPublicInfo();
					strcpy(name, (info->name) ? info->name : pl->GetFilename());

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

					IPlugin *pl = NULL;
					int id = 1;
					int num = atoi(engine->Cmd_Argv(3));
					if (num < 1 || num > (int)g_PluginSys.GetPluginCount())
					{
						META_CONPRINT("Plugin index not found.\n");
						return;
					}

					IPluginIterator *iter = g_PluginSys.GetPluginIterator();
					for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}
					pl = iter->GetPlugin();
					const sm_plugininfo_t *info = pl->GetPublicInfo();

					META_CONPRINTF("  Filename: %s\n", pl->GetFilename());
					if (info->name)
					{
						META_CONPRINTF("  Title: %s\n", info->name);
					}
					if (info->author)
					{
						META_CONPRINTF("  Author: %s\n", info->author);
					}
					if (info->version)
					{
						META_CONPRINTF("  Version: %s\n", info->version);
					}
					if (info->description)
					{
						META_CONPRINTF("  Description: %s\n", info->description);
					}
					if (info->url)
					{
						META_CONPRINTF("  URL: %s\n", info->url);
					}
					//:TODO: write if it's in debug mode, or do it inside LIST ?

					iter->Release();
					return;
				}
			}
			//:TODO: print plugins cmd list
		}
	}
	//:TODO: print cmd list or something
}