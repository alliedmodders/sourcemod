#include <oslink.h>
#include "mm_api.h"
#include "sm_version.h"

SourceMod g_SourceMod;

PLUGIN_EXPOSE(SourceMod, g_SourceMod);

bool SourceMod::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	return true;
}

bool SourceMod::Unload(char	*error, size_t maxlen)
{
	return true;
}

bool SourceMod::Pause(char *error, size_t maxlen)
{
	return true;
}

bool SourceMod::Unpause(char *error, size_t maxlen)
{
	return true;
}

void SourceMod::AllPluginsLoaded()
{
}

const char *SourceMod::GetAuthor()
{
	return "AlliedModders, LLC";
}

const char *SourceMod::GetName()
{
	return "SourceMod";
}

const char *SourceMod::GetDescription()
{
	return "Extensible administration and scripting system";
}

const char *SourceMod::GetURL()
{
	return "http://www.sourcemod.net/";
}

const char *SourceMod::GetLicense()
{
	return "See LICENSE.txt";
}

const char *SourceMod::GetVersion()
{
	return SOURCEMOD_VERSION;
}

const char *SourceMod::GetDate()
{
	return __DATE__;
}

const char *SourceMod::GetLogTag()
{
	return "SRCMOD";
}
