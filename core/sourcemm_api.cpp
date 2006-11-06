#include <oslink.h>
#include "sourcemm_api.h"
#include "sm_version.h"

SourceMod_Core g_SourceMod;

PLUGIN_EXPOSE(SourceMod, g_SourceMod);

bool SourceMod_Core::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	return true;
}

bool SourceMod_Core::Unload(char	*error, size_t maxlen)
{
	return true;
}

bool SourceMod_Core::Pause(char *error, size_t maxlen)
{
	return true;
}

bool SourceMod_Core::Unpause(char *error, size_t maxlen)
{
	return true;
}

void SourceMod_Core::AllPluginsLoaded()
{
}

const char *SourceMod_Core::GetAuthor()
{
	return "AlliedModders, LLC";
}

const char *SourceMod_Core::GetName()
{
	return "SourceMod";
}

const char *SourceMod_Core::GetDescription()
{
	return "Extensible administration and scripting system";
}

const char *SourceMod_Core::GetURL()
{
	return "http://www.sourcemod.net/";
}

const char *SourceMod_Core::GetLicense()
{
	return "See LICENSE.txt";
}

const char *SourceMod_Core::GetVersion()
{
	return SOURCEMOD_VERSION;
}

const char *SourceMod_Core::GetDate()
{
	return __DATE__;
}

const char *SourceMod_Core::GetLogTag()
{
	return "SRCMOD";
}
