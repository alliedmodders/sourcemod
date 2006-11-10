#ifndef _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
#define _INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_

#include <IPluginSys.h>
#include "sm_globals.h"

struct ContextPair
{
	ContextPair() : base(NULL), ctx(NULL)
	{
	};
	IPluginContext *base;
	sp_context_t *ctx;
};

class CPlugin : public IPlugin
{
public:
	virtual PluginLifetime GetLifetime() const;
	virtual SourcePawn::IPluginContext *GetBaseContext() const;
	virtual sp_context_t *GetContext() const;
	virtual const sm_plugininfo_t *GetPublicInfo() const;
	virtual const char *GetFilename() const;
	virtual bool IsDebugging() const;
	virtual PluginStatus GetStatus() const;
	virtual bool SetPauseState(bool paused);
	virtual void SetLockForUpdates(bool lock_status);
	virtual bool GetLockForUpdates() const;
	virtual unsigned int GetSerial() const;
	virtual const sp_plugin_t *GetPluginStructure() const;
public:
	static CPlugin *CreatePlugin(const char *file, 
								bool debug_default, 
								PluginLifetime life, 
								char *error, 
								size_t maxlen);
protected:
	void UpdateInfo();
private:
	ContextPair m_ctx_current;
	ContextPair m_ctx_backup;
	PluginLifetime m_lifetime;
	bool m_debugging;
	char m_filename[PLATFORM_MAX_PATH+1];
	PluginStatus m_status;
	bool m_lock;
	unsigned int m_serial;
	sm_plugininfo_t m_info;
	sp_plugin_t *m_plugin;
};

#endif //_INCLUDE_SOURCEMOD_PLUGINSYSTEM_H_
