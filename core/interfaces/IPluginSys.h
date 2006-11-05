#ifndef _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_

#include <IShareSys.h>

#define SMINTERFACE_PLUGINMANAGER_NAME		"IPluginManager"
#define SMINTERFACE_PLUGINMANAGER_VERSION	1

namespace SourceMod
{
	class IPlugin : public IUnloadableParent
	{
	public:
		UnloadableParentType GetParentType()
		{
			return ParentType_Module;
		}
	};

	enum PluginLifetime
	{
		PluginLifetime_Forever,
		PluginLifetime_Map
	};

	class IPluginManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_PLUGINMANAGER_NAME;
		}

		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_PLUGINMANAGER_VERSION;
		}
	public:
		/**
		 * @brief Attempts to load a plugin.
		 *
		 * @param path		Path and filename of plugin, relative to plugins folder.
		 * @param extended	Whether or not the plugin is a static plugin or optional plugin.
		 * @param debug		Whether or not to default the plugin into debug mode.
		 * @param lifetime	Lifetime of the plugin.
		 * @param error		Buffer to hold any error message.
		 * @param err_max	Maximum length of error message buffer.
		 * @return			A new plugin pointer on success, false otherwise.
		 */
		virtual IPlugin *LoadPlugin(const char *path, 
									bool extended, 
									bool debug,
									PluginLifetime lifetime,
									char error[],
									size_t err_max) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
