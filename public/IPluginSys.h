/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_

/**
 * @file IPluginSys.h
 * @brief Defines the interface for the Plugin System, which manages loaded plugins.
 */

#include <IShareSys.h>
#include <sp_vm_api.h>

#define SMINTERFACE_PLUGINSYSTEM_NAME		"IPluginManager"
#define SMINTERFACE_PLUGINSYSTEM_VERSION	1

/** Context user slot 3 is used Core for holding an IPluginContext pointer. */
#define SM_CONTEXTVAR_USER		3

namespace SourceMod
{
	class IPlugin;

	/** 
	 * @brief Encapsulates plugin public information exposed through "myinfo."
	 */
	typedef struct sm_plugininfo_s
	{
		const char *name;			/**< Plugin name */
		const char *author;			/**< Plugin author */
		const char *description;	/**< Plugin description */
		const char *version;		/**< Plugin version string */
		const char *url;			/**< Plugin URL */
	} sm_plugininfo_t;


	/**
	 * @brief Describes the usability status of a plugin.
	 */
	enum PluginStatus
	{
		Plugin_Running=0,		/**< Plugin is running */
		/* All states below are unexecutable */
		Plugin_Paused,			/**< Plugin is loaded but paused */
		Plugin_Error,			/**< Plugin is loaded but errored/locked */
		/* All states below do not have all natives */
		Plugin_Loaded,			/**< Plugin has passed loading and can be finalized */
		Plugin_Failed,			/**< Plugin has a fatal failure */
		Plugin_Created,			/**< Plugin is created but not initialized */
		Plugin_Uncompiled,		/**< Plugin is not yet compiled by the JIT */
		Plugin_BadLoad,			/**< Plugin failed to load */
	};


	/**
	 * @brief Describes the object lifetime of a plugin.
	 */
	enum PluginType
	{
		PluginType_Private,			/**< Plugin is privately managed and receives no forwards */
		PluginType_MapUpdated,		/**< Plugin will never be unloaded unless for updates on mapchange */
		PluginType_MapOnly,			/**< Plugin will be removed at mapchange */
		PluginType_Global,			/**< Plugin will never be unloaded or updated */
	};

	/**
	 * @brief Encapsulates a run-time plugin as maintained by SourceMod.
	 */
	class IPlugin
	{
	public:
		/** Virtual destructor */
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
		 * @brief Returns a plugin's identity token.
		 */
		virtual IdentityToken_t *GetIdentity() const =0;
	};


	/**
	 * @brief Iterates over a list of plugins.
	 */
	class IPluginIterator
	{
	public:
		/** Virtual destructor */
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
