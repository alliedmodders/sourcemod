#ifndef _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_

#include <IShareSys.h>
#include <sp_vm_api.h>

#define SMINTERFACE_PLUGINSYSTEM_NAME		"IPluginManager"
#define SMINTERFACE_PLUGINSYSTEM_VERSION	1

#define SM_CONTEXTVAR_USER		3

namespace SourceMod
{
	class IPlugin;

	/** 
	 * @brief Encapsulates plugin public information.
	 */
	typedef struct sm_plugininfo_s
	{
		const char *name;
		const char *author;
		const char *description;
		const char *version;
		const char *url;
	} sm_plugininfo_t;


	/**
	 * @brief Describes the usability status of a plugin.
	 */
	enum PluginStatus
	{
		Plugin_Running=0,		/* Plugin is running */
		/* All states below are unexecutable */
		Plugin_Paused,			/* Plugin is loaded but paused */
		/* All states below do not have all natives */
		Plugin_Loaded,			/* Plugin has passed loading and can be finalized */
		Plugin_Error,			/* Plugin has a blocking error */
		Plugin_Created,			/* Plugin is created but not initialized */
		Plugin_Uncompiled,		/* Plugin is not yet compiled by the JIT */
		Plugin_BadLoad,			/* Plugin failed to load */
	};


	/**
	 * @brief Describes the object lifetime of a plugin.
	 */
	enum PluginType
	{
		PluginType_Private,			/* Plugin is privately managed and receives no forwards */
		PluginType_MapUpdated,		/* Plugin will never be unloaded unless for updates on mapchange */
		PluginType_MapOnly,			/* Plugin will be removed at mapchange */
		PluginType_Global,			/* Plugin will never be unloaded or updated */
	};

	class IPluginFunction;

	/**
	 * @brief Encapsulates a run-time plugin as maintained by SourceMod.
	 */
	class IPlugin
	{
	public:
		virtual ~IPlugin()
		{
		}

		/**
		 * @brief Returns the lifetime of a plugin.
		 */
		virtual PluginType GetType() const =0;

		/**
		 * @brief Returns the current API context being used in the plugin.
		 *
		 * @return	Pointer to an IPluginContext, or NULL if not loaded.
		 */
		virtual SourcePawn::IPluginContext *GetBaseContext() const =0;

		/**
		 * @brief Returns the context structure being used in the plugin.
		 *
		 * @return	Pointer to an sp_context_t, or NULL if not loaded.
		 */
		virtual sp_context_t *GetContext() const =0;

		/**
		 * @brief Returns the plugin file structure.
		 *
		 * @return	Pointer to an sp_plugin_t, or NULL if not loaded.
		 */
		virtual const sp_plugin_t *GetPluginStructure() const =0;

		/**
		 * @brief Returns information about the plugin by reference.
		 *
		 * @return			Pointer to a sm_plugininfo_t object, NULL if plugin is not loaded.
		 */
		virtual const sm_plugininfo_t *GetPublicInfo() const =0;

		/**
		 * @brief Returns the plugin filename (relative to plugins dir).
		 */
		virtual const char *GetFilename() const =0;

		/**
		 * @brief Returns true if a plugin is in debug mode, false otherwise.
		 */
		virtual bool IsDebugging() const =0;

		/**
		 * @brief Returns the plugin status.
		 */
		virtual PluginStatus GetStatus() const =0;
		
		/**
		 * @brief Sets whether the plugin is paused or not.
		 *
		 * @return			True on successful state change, false otherwise.
		 */
		virtual bool SetPauseState(bool paused) =0;

		/**
		 * @brief Returns the unique serial number of a plugin.
		 */
		virtual unsigned int GetSerial() const =0;

		/**
		 * @brief Returns a function by name.
		 *
		 * @param public_name		Name of the function.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionByName(const char *public_name) =0;

		/**
		 * @brief Returns a function by its id.
		 *
		 * @param func_id			Function ID.
		 * @return					A new IPluginFunction pointer, NULL if not found.
		 */
		virtual IPluginFunction *GetFunctionById(funcid_t func_id) =0;

		/**
		 * @brief Returns a plugin's identity token.
		 */
		virtual IdentityToken_t GetIdentity() =0;
	};


	/**
	 * @brief Iterates over a list of plugins.
	 */
	class IPluginIterator
	{
	public:
		virtual ~IPluginIterator()
		{
		};
	public:
		/**
		 * @brief Returns true if there are more plugins in the iterator.
		 */
		virtual bool MorePlugins() =0;

		/**
		 * @brief Returns the plugin at the current iterator position.
		 */
		virtual IPlugin *GetPlugin() =0;

		/**
		 * @brief Advances to the next plugin in the iterator.
		 */
		virtual void NextPlugin() =0;

		/**
		 * @brief Destroys the iterator object.
		 * Note: You may use 'delete' in lieu of this function.
		 */
		virtual void Release() =0;
	};

	/**
	 * @brief Listens for plugin-oriented events.
	 */
	class IPluginsListener
	{
	public:
		/**
		 * @brief Called when a plugin is created/mapped into memory.
		 */
		virtual void OnPluginCreated(IPlugin *plugin)
		{
		}

		/**
		 * @brief Called when a plugin is fully loaded successfully.
		 */
		virtual void OnPluginLoaded(IPlugin *plugin)
		{
		}

		/**
		 * @brief Called when a plugin is unloaded (only if fully loaded).
		 */
		virtual void OnPluginUnloaded(IPlugin *plugin)
		{
		}

		/**
		 * @brief Called when a plugin is destroyed.
		 * NOTE: Always called if Created, even if load failed.
		 */
		virtual void OnPluginDestroyed(IPlugin *plugin)
		{
		}
	};


	/**
	 * @brief Manages the runtime loading and unloading of plugins.
	 */
	class IPluginManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_PLUGINSYSTEM_NAME;
		}

		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_PLUGINSYSTEM_VERSION;
		}
	public:
		/**
		 * @brief Attempts to load a plugin.
		 *
		 * @param path		Path and filename of plugin, relative to plugins folder.
		 * @param debug		Whether or not to default the plugin into debug mode.
		 * @param type		Lifetime of the plugin.
		 * @param error		Buffer to hold any error message.
		 * @param err_max	Maximum length of error message buffer.
		 * @return			A new plugin pointer on success, false otherwise.
		 */
		virtual IPlugin *LoadPlugin(const char *path, 
									bool debug,
									PluginType type,
									char error[],
									size_t err_max) =0;

		/**
		 * @brief Attempts to unload a plugin.
		 *
		 * @param plugin	Pointer to the plugin handle.
		 * @return			True on success, false otherwise.
		 */
		virtual bool UnloadPlugin(IPlugin *plugin) =0;

		/**
		 * @brief Finds a plugin by its context.
		 * Note: This function should be considered O(1).
		 *
		 * @param ctx		Pointer to an sp_context_t.
		 * @return			Pointer to a matching IPlugin, or NULL if none found.
		 */
		virtual IPlugin *FindPluginByContext(const sp_context_t *ctx) =0;

		/**
		 * @brief Returns the number of plugins (both failed and loaded).
		 *
		 * @return			The number of internally cached plugins.
		 */
		virtual unsigned int GetPluginCount() =0;

		/**
		 * @brief Returns a pointer that can be used to iterate through plugins.
		 * Note: This pointer must be freed using EITHER delete OR IPluginIterator::Release().
		 */
		virtual IPluginIterator *GetPluginIterator() =0;

		/** 
		 * @brief Adds a plugin manager listener.
		 *
		 * @param listener	Pointer to a listener.
		 */
		virtual void AddPluginsListener(IPluginsListener *listener) =0;

		/**
		 * @brief Removes a plugin listener.
		 * 
		 * @param listener	Pointer to a listener.
		 */
		virtual void RemovePluginsListener(IPluginsListener *listener) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
