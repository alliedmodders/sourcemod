// vim: set ts=4 sw=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#ifndef _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_PLUGINMNGR_INTERFACE_H_

/**
 * @file IPluginSys.h
 * @brief Defines the interface for the Plugin System, which manages loaded plugins.
 */

#include <IShareSys.h>
#include <IHandleSys.h>
#include <sp_vm_api.h>

#define SMINTERFACE_PLUGINSYSTEM_NAME		"IPluginManager"
#define SMINTERFACE_PLUGINSYSTEM_VERSION	8

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
		// @brief The plugin is running normally.
		Plugin_Running=0,

		// @brief The plugin is paused and no code may be executed until it
		// is resumed.
		Plugin_Paused,

		// @brief The plugin encountered a fatal error, but it may be loaded
		// again later.
		Plugin_Error,

		// @brief The plugin loaded, but has not yet been linked into the
		// dependency system. It may not have all its natives bound yet.
		Plugin_Loaded,

		// @brief The plugin encountered a fatal, unrecoverable error.
		Plugin_Failed,

		// @brief The plugin successfully compiled, but we have not yet begun
		// the loading and initialization process. This state should not be
		// observable.
		Plugin_Created,

		// @brief The plugin has not yet been compiled. This state should not
		// be observable. Plugins in this state do not have a context or
		// runtime.
		Plugin_Uncompiled,

		// @brief The plugin could not be loaded. Either its file was missing
		// or could not be recognized as a valid SourcePawn binary. Plugins
		// in this state do not have a context or runtime.
		Plugin_BadLoad,

		// @brief The plugin was once in a state <= Created, but has since
		// been destroyed. We have left its IPlugin instance around for
		// informational purposes. Plugins in this state do not have a
		// context or runtime.
		Plugin_Evicted
	};


	/**
	 * @brief Describes the object lifetime of a plugin.
	 */
	enum PluginType
	{
		PluginType_Private_Unused,	/**< Unused. */
		PluginType_MapUpdated,		/**< Plugin will never be unloaded unless for updates on mapchange */
		PluginType_MapOnly_Unused,	/**< Unused. */
		PluginType_Global_Unused,	/**< Unused. */
	};

	class IPhraseCollection;

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
		 * @brief Always returns PluginType_MapUpdated.
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
		 *
		 * On SourceMod 1.1 or higher, this always returns true for loaded plugins.
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

		/**
		 * @brief Returns the plugin's phrase collection.
		 *
		 * @return			Plugin's phrase collection.
		 */
		virtual IPhraseCollection *GetPhrases() =0;

		/**
		 * @brief Returns a plugin's handle.
		 *
		 * @return			Plugin's handle.
		 */
		virtual Handle_t GetMyHandle() =0;
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
	class IPluginsListener_V1
	{
	public:
		// @brief This callback should not be used since plugins may be in
		// an unusable state.
		virtual void OnPluginCreated(IPlugin *plugin) final
		{
		}

		// @brief Called when a plugin's required dependencies and natives have
		// been bound. Plugins at this phase may be in any state Failed or
		// lower. This is invoked immediately before OnPluginStart, and sometime
		// after OnPluginCreated.
		virtual void OnPluginLoaded(IPlugin *plugin)
		{
		}

		// @brief Called when a plugin is paused or unpaused.
		virtual void OnPluginPauseChange(IPlugin *plugin, bool paused)
		{
		}

		// @brief Called when a plugin is about to be unloaded. This is called for
		// any plugin for which OnPluginLoaded was called, and is invoked
		// immediately after OnPluginEnd(). The plugin may be in any state Failed
		// or lower.
		//
		// This function must not cause the plugin to re-enter script code. If
		// you wish to be notified of when a plugin is unloading, and to forbid
		// future calls on that plugin, use OnPluginWillUnload and use a
		// plugin property to block future calls.
		virtual void OnPluginUnloaded(IPlugin *plugin)
		{
		}

		// @brief Called when a plugin is destroyed. This is called on all plugins for
		// which OnPluginCreated was called. The plugin may be in any state.
		virtual void OnPluginDestroyed(IPlugin *plugin)
		{
		}
	};

	// @brief Listens for plugin-oriented events. Extends the V1 listener class.
	class IPluginsListener : public IPluginsListener_V1
	{
	public:
		virtual unsigned int GetApiVersion() const {
			return SMINTERFACE_PLUGINSYSTEM_VERSION;
		}

		// @brief Called when a plugin is about to be unloaded, before its
		// OnPluginEnd callback is fired. This can be used to ensure that any
		// asynchronous operations are flushed and no further operations can
		// be started (via SetProperty).
		//
		// Like OnPluginUnloaded, this is only called for plugins which
		// OnPluginLoaded was called.
		virtual void OnPluginWillUnload(IPlugin *plugin)
		{
		}
	};

	static const unsigned int kMinPluginSysApiWithWillUnloadCallback = 8;

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
		 * @param debug		Deprecated, must be false.
		 * @param type		Plugin type. Values other than PluginType_MapUpdated are ignored.
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
		 * @brief Adds a V1 plugin manager listener.
		 *
		 * @param listener	Pointer to a listener.
		 */
		virtual void AddPluginsListener_V1(IPluginsListener_V1 *listener) =0;

		/**
		 * @brief Removes a V1 plugin listener.
		 * 
		 * @param listener	Pointer to a listener.
		 */
		virtual void RemovePluginsListener_V1(IPluginsListener_V1 *listener) =0;

		/**
		 * @brief Converts a Handle to an IPlugin if possible.
		 *
		 * @param handle	Handle.
		 * @param err		Error, set on failure (otherwise undefined).
		 * @return			IPlugin pointer, or NULL on failure.
		 */
		virtual IPlugin *PluginFromHandle(Handle_t handle, HandleError *err) =0;

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

