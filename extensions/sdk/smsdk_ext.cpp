#include <stdio.h>
#include "smsdk_ext.h"

IShareSys *g_pShareSys = NULL;
IdentityToken_t *myself = NULL;
IHandleSys *g_pHandleSys = NULL;

PLATFORM_EXTERN_C IExtensionInterface *GetSMExtAPI()
{
	return g_pExtensionIface;
}

SDKExtension::SDKExtension()
{
#if defined SMEXT_CONF_METAMOD
	m_SourceMMLoaded = false;
	m_WeAreUnloaded = false;
	m_WeGotPauseChange = false;
#endif
}

bool SDKExtension::OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t err_max, bool late)
{
	g_pShareSys = sys;
	myself = me->GetIdentity();

#if defined SMEXT_CONF_METAMOD
	if (!m_SourceMMLoaded)
	{
		if (error)
		{
			snprintf(error, err_max, "Metamod attach failed");
		}
		return false;
	}
#endif

	SM_GET_IFACE(HANDLESYSTEM, g_pHandleSys);

	return SDK_OnLoad(error, err_max, late);
}

bool SDKExtension::IsMetamodExtension()
{
#if defined SMEXT_CONF_METAMOD
	return true;
#else
	return false;
#endif
}

void SDKExtension::OnExtensionPauseChange(bool state)
{
	m_WeGotPauseChange = true;
	SDK_OnPauseChange(state);
}

void SDKExtension::OnExtensionsAllLoaded()
{
	SDK_OnAllLoaded();
}

void SDKExtension::OnExtensionUnload()
{
	m_WeAreUnloaded = true;
	SDK_OnUnload();
}

const char *SDKExtension::GetExtensionAuthor()
{
	return SMEXT_CONF_AUTHOR;
}

const char *SDKExtension::GetExtensionDateString()
{
	return SMEXT_CONF_DATESTRING;
}

const char *SDKExtension::GetExtensionDescription()
{
	return SMEXT_CONF_DESCRIPTION;
}

const char *SDKExtension::GetExtensionVerString()
{
	return SMEXT_CONF_VERSION;
}

const char *SDKExtension::GetExtensionName()
{
	return SMEXT_CONF_NAME;
}

const char *SDKExtension::GetExtensionTag()
{
	return SMEXT_CONF_LOGTAG;
}

const char *SDKExtension::GetExtensionURL()
{
	return SMEXT_CONF_URL;
}

bool SDKExtension::SDK_OnLoad(char *error, size_t err_max, bool late)
{
	return true;
}

void SDKExtension::SDK_OnUnload()
{
}

void SDKExtension::SDK_OnPauseChange(bool paused)
{
}

void SDKExtension::SDK_OnAllLoaded()
{
}

#if defined SMEXT_CONF_METAMOD

PluginId g_PLID = 0;
ISmmPlugin *g_PLAPI = NULL;
SourceHook::ISourceHook *g_SHPtr = NULL;
ISmmAPI *g_SMAPI = NULL;

IVEngineServer *engine = NULL;
IServerGameDLL *gamedll = NULL;

SMM_API void *PL_EXPOSURE(const char *name, int *code)
{
	if (name && !strcmp(name, PLAPI_NAME))
	{
		if (code)
		{
			*code = IFACE_OK;
		}
		return static_cast<void *>(g_pExtensionIface);
	}

	if (code)
	{
		*code = IFACE_FAILED;
	}

	return NULL;
}

bool SDKExtension::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(serverFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(engineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);

	m_SourceMMLoaded = true;

	return SDK_OnMetamodLoad(error, maxlen, late);
}

bool SDKExtension::Unload(char *error, size_t maxlen)
{
	if (!m_WeAreUnloaded)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be unloaded by SourceMod.");
		}
		return false;
	}

	return SDK_OnMetamodUnload(error, maxlen);
}

bool SDKExtension::Pause(char *error, size_t maxlen)
{
	if (!m_WeGotPauseChange)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be paused by SourceMod.");
		}
		return false;
	}

	m_WeGotPauseChange = false;

	return SDK_OnMetamodPauseChange(true, error, maxlen);
}

bool SDKExtension::Unpause(char *error, size_t maxlen)
{
	if (!m_WeGotPauseChange)
	{
		if (error)
		{
			snprintf(error, maxlen, "This extension must be unpaused by SourceMod.");
		}
		return false;
	}

	m_WeGotPauseChange = false;

	return SDK_OnMetamodPauseChange(false, error, maxlen);
}

const char *SDKExtension::GetAuthor()
{
	return GetExtensionAuthor();
}

const char *SDKExtension::GetDate()
{
	return GetExtensionDateString();
}

const char *SDKExtension::GetDescription()
{
	return GetExtensionDescription();
}

const char *SDKExtension::GetLicense()
{
	return SMEXT_CONF_LICENSE;
}

const char *SDKExtension::GetLogTag()
{
	return GetExtensionTag();
}

const char *SDKExtension::GetName()
{
	return GetExtensionName();
}

const char *SDKExtension::GetURL()
{
	return GetExtensionURL();
}

const char *SDKExtension::GetVersion()
{
	return GetExtensionVerString();
}

bool SDKExtension::SDK_OnMetamodLoad(char *error, size_t err_max, bool late)
{
	return true;
}

bool SDKExtension::SDK_OnMetamodUnload(char *error, size_t err_max)
{
	return true;
}

bool SDKExtension::SDK_OnMetamodPauseChange(bool paused, char *error, size_t err_max)
{
	return true;
}

#endif
