/**
 * vim: set ts=4 sw=4 tw=99 noet:
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

#ifndef _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_

#include <IShareSys.h>
#include <ILibrarySys.h>
#include "am-string.h"

/**
 * @file IExtensionSys.h
 * @brief Defines the interface for loading/unloading/managing extensions.
 */

struct edict_t;

namespace SourceMod
{
	class IExtensionInterface;
	typedef void *		ITERATOR;		/**< Generic pointer for dependency iterators */

	/** 
	 * @brief Encapsulates an IExtensionInterface and its dependencies.
	 */
	class IExtension
	{
	public:
		/**
		 * @brief Returns whether or not the extension is properly loaded.
		 */
		virtual bool IsLoaded() =0;

		/**
		 * @brief Returns the extension's API interface
		 *
		 * @return			An IExtensionInterface pointer.
		 */
		virtual IExtensionInterface *GetAPI() =0;

		/**
		 * @brief Returns the filename of the extension, relative to the
		 * extension folder.   If the extension is an "external" extension, 
		 * the file path is specified by the extension itself, and may be 
		 * arbitrary (not real).
		 *
		 * @return			A string containing the extension file name.
		 */
		virtual const char *GetFilename() =0;

		/**
		 * @brief Returns the extension's identity token.
		 *
		 * @return			An IdentityToken_t pointer.
		 */
		virtual IdentityToken_t *GetIdentity() =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * @param pOwner        Unused
		 * @param pInterface    Unused
		 * @return              nullptr
		 */
		virtual ITERATOR *FindFirstDependency(IExtension **pOwner, SMInterface **pInterface) =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * @param iter          Unused
		 * @param pOwner        Unused
		 * @param pInterface    Unused
		 * @return              false
		 */
		virtual bool FindNextDependency(ITERATOR *iter, IExtension **pOwner, SMInterface **pInterface) =0;

		/**
		 * @brief Deprecated, do not use.
		 *
		 * @param iter          Unused
		 */
		virtual void FreeDependencyIterator(ITERATOR *iter) =0;

		/**
		 * @brief Queries the extension to see its run state.
		 *
		 * @param error			Error buffer (may be NULL).
		 * @param maxlength		Maximum length of buffer.
		 * @return				True if extension is okay, false if not okay.
		 */
		virtual bool IsRunning(char *error, size_t maxlength) =0;

		/**
		 * @brief Returns whether the extension is local (from the extensions 
		 * folder), or is from an external source (such as Metamod:Source).
		 *
		 * @return				True if from an external source, 
		 *						false if local to SourceMod.
		 */
		virtual bool IsExternal() =0;
	};

	/**
	 * @brief Version code of the IExtensionInterface API itself.
	 *
	 * Note: This is bumped when IShareSys is changed, because IShareSys 
	 * itself is not versioned.
	 *
	 * V6 - added TestFeature() to IShareSys.
	 * V7 - added OnDependenciesDropped() to IExtensionInterface.
	 * V8 - added OnCoreMapEnd() to IExtensionInterface.
	 */
	#define SMINTERFACE_EXTENSIONAPI_VERSION	8

	/**
	 * @brief The interface an extension must expose.
	 */
	class IExtensionInterface
	{
	public:
		/** Returns the interface API version */
		virtual unsigned int GetExtensionVersion()
		{
			return SMINTERFACE_EXTENSIONAPI_VERSION;
		}
	public:
		/**
		 * @brief Called when the extension is loaded.
		 * 
		 * @param me		Pointer back to extension.
		 * @param sys		Pointer to interface sharing system of SourceMod.
		 * @param error		Error buffer to print back to, if any.
		 * @param maxlength	Maximum size of error buffer.
		 * @param late		If this extension was loaded "late" (i.e. manually).
		 * @return			True if load should continue, false otherwise.
		 */
		virtual bool OnExtensionLoad(IExtension *me,
			IShareSys *sys, 
			char *error, 
			size_t maxlength, 
			bool late) =0;

		/**
		 * @brief Called when the extension is about to be unloaded.
		 */
		virtual void OnExtensionUnload() =0;

		/**
		 * @brief Called when all extensions are loaded (loading cycle is done).
		 * If loaded late, this will be called right after OnExtensionLoad().
		 */
		virtual void OnExtensionsAllLoaded() =0;

		/**
		 * @brief Called when your pause state is about to change.
		 * 
		 * @param pause		True if pausing, false if unpausing.
		 */
		virtual void OnExtensionPauseChange(bool pause) =0;

		/**
		 * @brief Asks the extension whether it's safe to remove an external 
		 * interface it's using.  If it's not safe, return false, and the 
		 * extension will be unloaded afterwards.
		 *
		 * NOTE: It is important to also hook NotifyInterfaceDrop() in order to clean 
		 * up resources.
		 *
		 * @param pInterface		Pointer to interface being dropped.  This 
		 * 							pointer may be opaque, and it should not 
		 *							be queried using SMInterface functions unless 
		 *							it can be verified to match an existing 
		 *							pointer of known type.
		 * @return					True to continue, false to unload this 
		 * 							extension afterwards.
		 */
		virtual bool QueryInterfaceDrop(SMInterface *pInterface)
		{
			return false;
		}

		/**
		 * @brief Notifies the extension that an external interface it uses is being removed.
		 *
		 * @param pInterface		Pointer to interface being dropped.  This
		 * 							pointer may be opaque, and it should not 
		 *							be queried using SMInterface functions unless 
		 *							it can be verified to match an existing 
		 */
		virtual void NotifyInterfaceDrop(SMInterface *pInterface)
		{
		}

		/**
		 * @brief Return false to tell Core that your extension should be considered unusable.
		 *
		 * @param error				Error buffer.
		 * @param maxlength			Size of error buffer.
		 * @return					True on success, false otherwise.
		 */
		virtual bool QueryRunning(char *error, size_t maxlength)
		{
			return true;
		}
	public:
		/**
		 * @brief For extensions loaded through SourceMod, this should return true 
		 * if the extension needs to attach to Metamod:Source.  If the extension 
		 * is loaded through Metamod:Source, and uses SourceMod optionally, it must 
		 * return false.
		 *
		 * @return					True if Metamod:Source is needed.
		 */
		virtual bool IsMetamodExtension() =0;

		/**
		 * @brief Must return a string containing the extension's short name.
		 *
		 * @return					String containing extension name.
		 */
		virtual const char *GetExtensionName() =0;

		/**
		 * @brief Must return a string containing the extension's URL.
		 *
		 * @return					String containing extension URL.
		 */
		virtual const char *GetExtensionURL() =0;

		/**
		 * @brief Must return a string containing a short identifier tag.
		 *
		 * @return					String containing extension tag.
		 */
		virtual const char *GetExtensionTag() =0;

		/**
		 * @brief Must return a string containing a short author identifier.
		 *
		 * @return					String containing extension author.
		 */
		virtual const char *GetExtensionAuthor() =0;

		/**
		 * @brief Must return a string containing version information.
		 *
		 * Any version string format can be used, however, SourceMod 
		 * makes a special guarantee version numbers in the form of 
		 * A.B.C.D will always be fully displayed, where:
		 * 
		 * A is a major version number of at most one digit.
		 * B is a minor version number of at most two digits.
		 * C is a minor version number of at most two digits.
		 * D is a build number of at most 5 digits.
		 *
		 * Thus, thirteen characters of display is guaranteed.
		 *
		 * @return					String containing extension version.
		 */
		virtual const char *GetExtensionVerString() =0;

		/**
		 * @brief Must return a string containing description text.
		 *
		 * The description text may be longer than the other identifiers, 
		 * as it is only displayed when viewing one extension at a time.
		 * However, it should not have newlines, or any other characters 
		 * which would otherwise disrupt the display pattern.
		 * 
		 * @return					String containing extension description.
		 */
		virtual const char *GetExtensionDescription() =0;

		/**
		 * @brief Must return a string containing the compilation date.
		 *
		 * @return					String containing the compilation date.
		 */
		virtual const char *GetExtensionDateString() =0;

		/**
		 * @brief Called on server activation before plugins receive the OnServerLoad forward.
		 * 
		 * @param pEdictList		Edicts list.
		 * @param edictCount		Number of edicts in the list.
		 * @param clientMax			Maximum number of clients allowed in the server.
		 */
		virtual void OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
		{
		}

		/**
		 * @brief Called once all dependencies have been unloaded. This is
		 * called AFTER OnExtensionUnload(), but before the extension library
		 * has been unloaded. It can be used as an alternate unload hook for
		 * cases where having no dependent plugins would make shutdown much
		 * simplier.
		 */
		virtual void OnDependenciesDropped()
		{
		}

		/**
		 * @brief Called on level shutdown
		 *
		 */

		virtual void OnCoreMapEnd()
		{
		}
	};

	/**
	 * @brief Returned via OnMetamodQuery() to get an IExtensionManager pointer.
	 */
	#define SOURCEMOD_INTERFACE_EXTENSIONS				"SM_ExtensionManager"

	/**
	 * @brief Fired through OnMetamodQuery() to notify plugins that SourceMod is 
	 * loaded.  
	 *
	 * Plugins should not return an interface pointer or IFACE_OK, instead, 
	 * they should attach as needed by searching for SOURCEMOD_INTERFACE_EXTENSIONS.
	 *
	 * This may be fired more than once; if already attached, an extension should 
	 * not attempt to re-attach.  The purpose of this is to notify Metamod:Source 
	 * plugins which load after SourceMod loads.
	 */
	#define SOURCEMOD_NOTICE_EXTENSIONS					"SM_ExtensionsAttachable"

	#define SMINTERFACE_EXTENSIONMANAGER_NAME			"IExtensionManager"
	#define SMINTERFACE_EXTENSIONMANAGER_VERSION		2

	/**
	 * @brief Manages the loading/unloading of extensions.
	 */
	class IExtensionManager : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_EXTENSIONMANAGER_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_EXTENSIONMANAGER_VERSION;
		}
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version < 2)
			{
				return false;
			}
			return SMInterface::IsVersionCompatible(version);
		}
	public:
		/**
		 * @brief Loads a extension into the extension system.
		 *
		 * @param path			Path to extension file, relative to the 
		 *						extensions folder.
		 * @param error			Error buffer.
		 * @param maxlength		Maximum error buffer length.
		 * @return				New IExtension on success, NULL on failure.
		 *						If	NULL is returned, the error buffer will be 
		 *						filled with a null-terminated string.
		 */
		virtual IExtension *LoadExtension(const char *path, 
			char *error,
			size_t maxlength) =0;

		/**
		 * @brief Loads an extension into the extension system, directly, 
		 * as an external extension.  
		 *
		 * The extension receives all normal callbacks.  However, it is 
		 * never opened via LoadLibrary/dlopen or closed via FreeLibrary
		 * or dlclose.
		 *
		 * @param pInterface	Pointer to an IExtensionInterface instance.
		 * @param filepath		Relative path to the extension's file, from 
		 * 						mod folder.
		 * @param filename		Name to use to uniquely identify the extension. 
		 *						The name should be generic, without any 
		 *						platform-specific suffices.  For example, 
		 *						sdktools.ext instead of sdktools.ext.so.
		 *						This filename is used to detect if the 
		 *						extension is already loaded, and to verify 
		 *						plugins that require the same extension.
		 * @param error			Buffer to store error message.
		 * @param maxlength		Maximum size of the error buffer.
		 * @return				IExtension pointer on success, NULL on failure.
		 *						If NULL is returned, the error buffer will be 
		 *						filled with a null-terminated string.
		 */
		virtual IExtension *LoadExternal(IExtensionInterface *pInterface,
			const char *filepath,
			const char *filename,
			char *error,
			size_t maxlength) =0;

		/**
		 * @brief Attempts to unload an extension.  External extensions must 
		 * call this before unloading.
		 *
		 * @param pExt			IExtension pointer.
		 * @return				True if successful, false otherwise.
		 */
		virtual bool UnloadExtension(IExtension *pExt) =0;
	};

	#define SM_IFACEPAIR(name) SMINTERFACE_##name##_NAME, SMINTERFACE_##name##_VERSION

	#define SM_FIND_IFACE_OR_FAIL(prefix, variable, errbuf, errsize) \
		if (!sharesys->RequestInterface(SM_IFACEPAIR(prefix), myself, (SMInterface **)&variable)) \
		{ \
			if (errbuf) \
			{ \
				size_t len = ke::SafeSprintf(errbuf, \
					errsize, \
					"Could not find interface: %s (version: %d)", \
					SM_IFACEPAIR(prefix)); \
				if (len >= errsize) \
				{ \
					errbuf[errsize - 1] = '\0'; \
				} \
			} \
			return false; \
		}

	#define SM_FIND_IFACE(prefix, variable) \
		sharesys->RequestInterface(SM_IFACEPAIR(prefix), myself, (SMInterface **)&variable);
}

#endif //_INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_

