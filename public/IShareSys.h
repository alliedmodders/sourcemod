/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
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
		 * @brief Must return whether the requested version number is backwards comaptible.
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
		 * @brief Adds a list of natives to the global native pool, to be bound on plugin load.
		 * NOTE: Adding natives currently does not bind them to any loaded plugins.
		 * You must manually bind late natives.
		 * 
		 * @param myself		Identity token of parent object.
		 * @param natives		Array of natives to add.  The last entry must have NULL members.
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
		 * @brief Creates a new identity token.  This token is guaranteed to be unique
		 * amongst all other open identities.
		 *
		 * @param type			Identity type.
		 * @return				A new IdentityToken_t identifier.
		 */
		virtual IdentityToken_t *CreateIdentity(IdentityType_t type) =0;

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
	};
};

#endif //_INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
