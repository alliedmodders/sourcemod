/**
 * vim: set ts=4 :
 * ======================================================
 * Metamod:Source Stub Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This stub plugin is public domain.
 *
 * Version: $Id$
 */

#include <stdio.h>
#include "stub_mm.h"
#include "stub_util.h"
#include "sm_ext.h"

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);

StubPlugin g_StubPlugin;
IVEngineServer *engine = NULL;
IServerGameDLL *server = NULL;
ISmmPlugin *mmsplugin = &g_StubPlugin;

PLUGIN_EXPOSE(StubPlugin, g_StubPlugin);
bool StubPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

#if defined METAMOD_PLAPI_VERSION
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
#else
	GET_V_IFACE_ANY(serverFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(engineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
#endif

	SH_ADD_HOOK(IServerGameDLL, ServerActivate, server, SH_STATIC(Hook_ServerActivate), true);

	ismm->AddListener(this, this);

	return true;
}

bool StubPlugin::Unload(char *error, size_t maxlen)
{
	SM_UnloadExtension();

	SH_REMOVE_HOOK(IServerGameDLL, ServerActivate, server, SH_STATIC(Hook_ServerActivate), true);

	return true;
}

void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	META_LOG(g_PLAPI, "ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void *StubPlugin::OnMetamodQuery(const char *iface, int *ret)
{
	if (strcmp(iface, SOURCEMOD_NOTICE_EXTENSIONS) == 0)
	{
		BindToSourcemod();
	}

	if (ret != NULL)
	{
		*ret = IFACE_OK;
	}

	return NULL;
}

void StubPlugin::AllPluginsLoaded()
{
	BindToSourcemod();
}

void StubPlugin::BindToSourcemod()
{
	char error[256];

	if (!SM_LoadExtension(error, sizeof(error)))
	{
		char message[512];
		UTIL_Format(message, sizeof(message), "Could not load as a SourceMod extension: %s\n", error);
		engine->LogPrint(message);
	}
}

bool StubPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool StubPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *StubPlugin::GetLicense()
{
	return "Public Domain";
}

const char *StubPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *StubPlugin::GetDate()
{
	return __DATE__;
}

const char *StubPlugin::GetLogTag()
{
	return "STUB";
}

const char *StubPlugin::GetAuthor()
{
	return "AlliedModders LLC";
}

const char *StubPlugin::GetDescription()
{
	return "Sample empty plugin";
}

const char *StubPlugin::GetName()
{
	return "Stub Plugin";
}

const char *StubPlugin::GetURL()
{
	return "http://www.sourcemm.net/";
}

