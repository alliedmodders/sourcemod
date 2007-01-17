#ifndef _INCLUDE_SOURCEMOD_MM_API_H_
#define _INCLUDE_SOURCEMOD_MM_API_H_

#include <ISmmPlugin.h>
#include <eiface.h>

/**
 * @file Contains wrappers around required Metamod:Source API exports
 */

class SourceMod_Core : public ISmmPlugin
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AllPluginsLoaded();
public:
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();
};

extern SourceMod_Core g_SourceMod_Core;
extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
extern IServerGameClients *serverClients;
extern ISmmPluginManager *g_pMMPlugins;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_SOURCEMOD_MM_API_H_
