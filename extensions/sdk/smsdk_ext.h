#ifndef _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_

#include "smsdk_config.h"
#include <IExtensionSys.h>
#include <IHandleSys.h>
#include <sp_vm_api.h>
#include <sm_platform.h>

#if defined SMEXT_CONF_METAMOD
#include <ISmmPlugin.h>
#include <eiface.h>
#endif

using namespace SourceMod;

class SDKExtension : 
#if defined SMEXT_CONF_METAMOD
	public ISmmPlugin,
#endif
	public IExtensionInterface
{
public:
	SDKExtension();
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param err_max	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t err_max, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	virtual void SDK_OnPauseChange(bool paused);

#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param err_max		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(char *error, size_t err_max, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param err_max		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char *error, size_t err_max);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param err_max		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t err_max);
#endif

public: //IExtensionInterface
	virtual bool OnExtensionLoad(IExtension *me, IShareSys *sys, char *error, size_t err_max, bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();
	virtual bool IsMetamodExtension();
	virtual void OnExtensionPauseChange(bool state);
	virtual const char *GetExtensionName();
	virtual const char *GetExtensionURL();
	virtual const char *GetExtensionTag();
	virtual const char *GetExtensionAuthor();
	virtual const char *GetExtensionVerString();
	virtual const char *GetExtensionDescription();
	virtual const char *GetExtensionDateString();
#if defined SMEXT_CONF_METAMOD
public: //ISmmPlugin
	virtual bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlength, bool late);
	virtual const char *GetAuthor();
	virtual const char *GetName();
	virtual const char *GetDescription();
	virtual const char *GetURL();
	virtual const char *GetLicense();
	virtual const char *GetVersion();
	virtual const char *GetDate();
	virtual const char *GetLogTag();
	virtual bool Unload(char *error, size_t maxlen);
	virtual bool Pause(char *error, size_t maxlen);
	virtual bool Unpause(char *error, size_t maxlen);
private:
	bool m_SourceMMLoaded;
	bool m_WeAreUnloaded;
	bool m_WeGotPauseChange;
#endif
};

extern SDKExtension *g_pExtensionIface;

extern IShareSys *g_pShareSys;
extern IExtension *myself;
extern IHandleSys *g_pHandleSys;

#if defined SMEXT_CONF_METAMOD
PLUGIN_GLOBALVARS();
extern IVEngineServer *engine;
extern IServerGameDLL *gamedll;
#endif

#define SM_MKIFACE(name) SMINTERFACE_##name##_NAME, SMINTERFACE_##name##_VERSION
#define SM_GET_IFACE(prefix,addr) \
	if (!g_pShareSys->RequestInterface(SM_MKIFACE(prefix), myself, (SMInterface **)&addr)) { \
	 if (error) { \
		snprintf(error, err_max, "Could not find interface: %s", SMINTERFACE_##prefix##_NAME); \
	 } \
	 return false; \
	}

#endif //_INCLUDE_SOURCEMOD_EXTENSION_BASESDK_H_
