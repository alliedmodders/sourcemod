/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
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
#define SMINTERFACE_PLUGINSYSTEM_VERSION	2

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
		virtual PluginType GetType() =0;

		/**
		 * @brief Returns the IPluginRuntime::GetDefaultContext() value.
		 *
		 * @return	Pointer to an IPluginContext, or NULL if not loaded.
		 */
		virtual SourcePawn::IPluginContext *GetBaseContext() =0;

		/**
		 * @brief Deprecated, returns NULL.
		 *
		 * @return	NULL.
		 */
		virtual sp_context_t *GetContext() =0;

		/**
		 * @brief Deprecated, returns NULL.
		 *
		 * @return	NULL.
		 */
		virtual void *GetPluginStructure() =0;

		/**
		 * @brief Returns information about the plugin by reference.
		 *
		 * @return			Pointer to a sm_plugininfo_t object, NULL if plugin is not loaded.
		 */
		virtual const sm_plugininfo_t *GetPublicInfo() =0;

		/**
		 * @brief Returns the plugin filename (relative to plugins dir).
		 */
		virtual const char *GetFilename() =0;

		/**
		 * @brief Returns true if a plugin is in debug mode, false otherwise.
		 */
		virtual bool IsDebugging() =0;

		/**
		 * @brief Returns the plugin status.
		 */
		virtual PluginStatus GetStatus() =0;
		
		/**
		 * @brief Sets whether the plugin is paused or not.
		 *
		 * @return			True on successful state change, false otherwise.
		 */
		virtual bool SetPauseState(bool paused) =0;

		/**
		 * @brief Returns the unique serial number of a plugin.
		 */
		virtual unsigned int GetSerial() =0;

		/**
		 * @brief Returns a plugin's identity token.
		 */
		virtual IdentityToken_t *GetIdentity() =0;

		/**
		 * @brief Sets a property on this plugin.  This is used for per-plugin
		 * data from extensions or other parts of core.  The property's value must 
		 * be manually destructed when the plugin is destroyed.
		 *
		 * @param prop		String containing name of the property.
		 * @param ptr		Generic pointer to set.
		 * @return			True on success, false if the property is already set.
		 */
		virtual bool SetProperty(const char *prop, void *ptr) =0;

		/**
		 * @brief Gets a property from a plugin.
		 *
		 * @param prop		String containing the property's name.
		 * @param ptr		Optional pointer to the generic pointer.
		 * @param remove	Optional boolean value; if true, property is removed
		 *					(so it can be set again).
		 * @return			True if the property existed, false otherwise.
		 */
		virtual bool GetProperty(const char *prop, void **ptr, bool remove=false) =0;

		/**
		 * @brief Returns the runtime representing this plugin.
		 *
		 * @return			IPluginRuntime pointer, or NULL if not loaded.
		 */
		virtual SourcePawn::IPluginRuntime *GetRuntime() =0;
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
		* @brief Called when a plugin is paused or unpaused.
		*/
		virtual void OnPluginPauseChange(IPlugin *plugin, bool paused)
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
		 * @param maxlength	Maximum length of error message buffer.
		 * @param wasloaded	Stores if the plugin is already loaded.
		 * @return			A new plugin pointer on success, false otherwise.
		 */
		virtual IPlugin *LoadPlugin(const char *path, 
									bool debug,
									PluginType type,
									char error[],
									size_t maxlength,
									bool *wasloaded) =0;

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
}

#endif //_INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
