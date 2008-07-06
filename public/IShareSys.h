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

#ifndef _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
#define _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_

/**
 * @file IShareSys.h
 * @brief Defines the Share System, responsible for shared resources and dependencies.
 *
 *  The Share System also manages the Identity_t data type, although this is internally
 * implemented with the Handle System.
 */

#include <sp_vm_types.h>


namespace SourceMod
{
	class IExtension;
	struct IdentityToken_t;

	/** Forward declaration from IHandleSys.h */
	typedef unsigned int		HandleType_t;

	/** Forward declaration from IHandleSys.h */
	typedef HandleType_t		IdentityType_t;

	/**
	 * @brief Defines the base functionality required by a shared interface.
	 */
	class SMInterface
	{
	public:
		/**
		 * @brief Must return an integer defining the interface's version.
		 */
		virtual unsigned int GetInterfaceVersion() =0;

		/**
		 * @brief Must return a string defining the interface's unique name.
		 */
		virtual const char *GetInterfaceName() =0;

		/**
		 * @brief Must return whether the requested version number is backwards compatible.
		 * Note: This can be overridden for breaking changes or custom versioning.
		 * 
		 * @param version		Version number to compare against.
		 * @return				True if compatible, false otherwise.
		 */
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version > GetInterfaceVersion())
			{
				return false;
			}

			return true;
		}
	};

	/**
	 * @brief Tracks dependencies and fires dependency listeners.
	 */
	class IShareSys
	{
	public:
		/**
		 * @brief Adds an interface to the global interface system.
		 *
		 * @param myself		Object adding this interface, in order to track dependencies.
		 * @param iface			Interface pointer (must be unique).
		 * @return				True on success, false otherwise.
		 */
		virtual bool AddInterface(IExtension *myself, SMInterface *iface) =0;

		/**
		 * @brief Requests an interface from the global interface system.
		 * If found, the interface's internal reference count will be increased.
		 *
		 * @param iface_name	Interface name.
		 * @param iface_vers	Interface version to attempt to match.
		 * @param myself		Object requesting this interface, in order to track dependencies.
		 * @param pIface		Pointer to store the return value in.
		 */
		virtual bool RequestInterface(const char *iface_name, 
										unsigned int iface_vers,
										IExtension *myself,
										SMInterface **pIface) =0;

		/**
		 * @brief Adds a list of natives to the global native pool, to be 
		 * bound on plugin load.
		 *
		 * Adding natives does not bind them to any loaded plugins; the 
		 * plugins must be reloaded for new natives to take effect.  
		 * 
		 * @param myself		Identity token of parent object.
		 * @param natives		Array of natives to add.  The last entry in 
		 *						the array must be filled with NULLs to 
		 *						terminate the array.  The array must be static 
		 *						as Core will cache the pointer for the 
		 *						lifetime of the extension.
		 */
		virtual void AddNatives(IExtension *myself, const sp_nativeinfo_t *natives) =0;

		/**
		 * @brief Creates a new identity type.
		 * NOTE: Module authors should never need to use this.  Due to the current implementation,
		 * there is a hardcoded limit of 15 types.  Core uses up a few, so think carefully!
		 *
		 * @param name			String containing type name.  Must not be empty or NULL.
		 * @return				A new HandleType_t identifier, or 0 on failure.
		 */
		virtual IdentityType_t CreateIdentType(const char *name) =0;

		/**
		 * @brief Finds an identity type by name.
		 * DEFAULT IDENTITY TYPES:
		 *  "PLUGIN"	- An IPlugin object.
		 *  "MODULE"	- An IModule object.
		 *  "CORE"		- An SMGlobalClass or other singleton.
		 * 
		 * @param name			String containing type name to search for.
		 * @return				A HandleType_t identifier if found, 0 otherwise.
		 */
		virtual IdentityType_t FindIdentType(const char *name) =0;

		/**
		 * @brief Creates a new identity token.  This token is guaranteed to be 
		 * unique amongst all other open identities.
		 *
		 * @param type			Identity type.
		 * @param ptr			Private data pointer (cannot be NULL).
		 * @return				A new IdentityToken_t pointer, or NULL on failure.
		 */
		virtual IdentityToken_t *CreateIdentity(IdentityType_t type, void *ptr) =0;

		/** 
		 * @brief Destroys an identity type.  Note that this will delete any identities
		 * that are under this type.  
		 *
		 * @param type			Identity type.
		 */
		virtual void DestroyIdentType(IdentityType_t type) =0;

		/**
		 * @brief Destroys an identity token.  Any handles being owned by this token, or
		 * any handles being 
		 *
		 * @param identity		Identity to remove.
		 */
		virtual void DestroyIdentity(IdentityToken_t *identity) =0;

		/**
		 * @brief Requires an extension.  This tells SourceMod that without this extension,
		 * your extension should not be loaded.  The name should not include the ".dll" or
		 * the ".so" part of the file name.
		 *
		 * @param myself		IExtension pointer to yourself.
		 * @param filename		File of extension to require.
		 * @param require		Whether or not this extension is a required dependency.
		 * @param autoload		Whether or not to autoload this extension.
		 */
		virtual void AddDependency(IExtension *myself, const char *filename, bool require, bool autoload) =0;

		/**
		 * @brief Registers a library name to an extension.
		 *
		 * @param myself		Extension to register library to.
		 * @param name			Library name.
		 */
		virtual void RegisterLibrary(IExtension *myself, const char *name) =0;

		/**
		 * @brief Adds natives that will override Core natives when called.
		 *
		 * A Core version of each native must exist.  If one does not, then 
		 * Core will simply ignore that entry.  No more than one override 
		 * can exist on a given native.
		 *
		 * Override natives represent a weak coupling.  If the extension is 
		 * unloaded, the native will be re-bound to the Core version.  If 
		 * the extension is loaded after plugins are loaded, the override 
		 * will not take effect until those plugins are reloaded.
		 * 
		 * @param myself		Identity token of parent object.
		 * @param natives		Array of natives to add.  The last entry in 
		 *						the array must be filled with NULLs to 
		 *						terminate the array.  The array must be static 
		 *						as Core will cache the pointer for the 
		 *						lifetime of the extension.
		 */
		virtual void OverrideNatives(IExtension *myself, const sp_nativeinfo_t *natives) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
