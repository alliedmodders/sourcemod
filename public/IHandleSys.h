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

#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_

/**
 * @file IHandleSys.h
 * @brief Defines the interface for creating, reading, and removing Handles.
 * 
 *  The Handle system abstracts generic pointers into typed objects represented by 
 * 32bit codes.  This is extremely useful for verifying data integrity and cross-platform
 * support in SourcePawn scripts.  When a Plugin unloads, all its Handles are freed, ensuring 
 * that no memory leaks are present,  They have reference counts and thus can be duplicated, 
 * or cloned, and are safe to pass between Plugins even if one is unloaded.
 *
 *  Handles are created with a given type (custom types may be created).  They can have
 * per-Identity permissions for deletion, reading, and cloning.  They also support generic
 * operations.  For example, deleting a Handle will call that type's destructor on the generic
 * pointer, making cleanup easier for users and eliminating memory leaks.
 */

#include <IShareSys.h>
#include <sp_vm_types.h>

#define SMINTERFACE_HANDLESYSTEM_NAME			"IHandleSys"
#define SMINTERFACE_HANDLESYSTEM_VERSION		5

/** Specifies no Identity */
#define DEFAULT_IDENTITY			NULL
/** Specifies no Type.  This is invalid for everything but reading a Handle. */
#define NO_HANDLE_TYPE				0
/** Specifies an invalid/NULL Handle */
#define BAD_HANDLE					0

namespace SourceMod
{
	/**
	 * @brief Represents a Handle Type ID.
	 */
	typedef unsigned int HandleType_t;

	/**
	 * @brief Represents a Handle ID.
	 */
	typedef unsigned int Handle_t;


	/*
	 * About type checking:
	 * Types can be inherited - a Parent type ("Supertype") can have child types.
	 * When accessing handles, type checking is done.  This table shows how this is resolved:
	 *
	 * HANDLE		CHECK		->  RESULT
	 * ------		-----			------
	 * Parent		Parent			Success
	 * Parent		Child			Fail
	 * Child		Parent			Success
	 * Child		Child			Success
	 */

	/**
	 * @brief Lists the possible handle error codes.
	 */
	enum HandleError
	{
		HandleError_None = 0,		/**< No error */
		HandleError_Changed,		/**< The handle has been freed and reassigned */
		HandleError_Type,			/**< The handle has a different type registered */
		HandleError_Freed,			/**< The handle has been freed */
		HandleError_Index,			/**< generic internal indexing error */
		HandleError_Access,			/**< No access permitted to free this handle */
		HandleError_Limit,			/**< The limited number of handles has been reached */
		HandleError_Identity,		/**< The identity token was not usable */
		HandleError_Owner,			/**< Owners do not match for this operation */
		HandleError_Version,		/**< Unrecognized security structure version */
		HandleError_Parameter,		/**< An invalid parameter was passed */
		HandleError_NoInherit,		/**< This type cannot be inherited */
	};

	/**
	 * @brief Lists access rights specific to a type.
	 */
	enum HTypeAccessRight
	{
		HTypeAccess_Create = 0,		/**< Handles of this type can be created (DEFAULT=false) */
		HTypeAccess_Inherit,		/**< Sub-types can inherit this type (DEFAULT=false) */
		/* -------------- */
		HTypeAccess_TOTAL,			/**< Total number of type access rights */
	};

	/**
	 * @brief Lists access rights specific to a Handle.  
	 * 
	 * These rights are exclusive. For example, you do not need "read" access to delete or clone.
	 */
	enum HandleAccessRight
	{
		HandleAccess_Read,			/**< Can be read (DEFAULT=ident only) */
		HandleAccess_Delete,		/**< Can be deleted (DEFAULT=owner only) */
		HandleAccess_Clone,			/**< Can be cloned (DEFAULT=any) */
		/* ------------- */
		HandleAccess_TOTAL,			/**< Total number of access rights */
	};

	/** Access is restricted to the identity */
	#define HANDLE_RESTRICT_IDENTITY	(1<<0)	
	/** Access is restricted to the owner */
	#define HANDLE_RESTRICT_OWNER		(1<<1)

	/**
	 * @brief This is used to define per-type access rights.
	 */
	struct TypeAccess
	{
		/** Constructor */
		TypeAccess()
		{
			hsVersion = SMINTERFACE_HANDLESYSTEM_VERSION;
		}
		unsigned int hsVersion;			/**< Handle API version */
		IdentityToken_t *ident;			/**< Identity owning this type */
		bool access[HTypeAccess_TOTAL];	/**< Access array */
	};

	/**
	 * @brief This is used to define per-Handle access rights.
	 */
	struct HandleAccess
	{
		/** Constructor */
		HandleAccess()
		{
			hsVersion = SMINTERFACE_HANDLESYSTEM_VERSION;
		}
		unsigned int hsVersion;						/**< Handle API version */
		unsigned int access[HandleAccess_TOTAL];	/**< Access array */
	};

	/**
	 * @brief This pair of tokens is used for identification.
	 */
	struct HandleSecurity
	{
		HandleSecurity()
		{
		}
		HandleSecurity(IdentityToken_t *owner, IdentityToken_t *identity)
			: pOwner(owner), pIdentity(identity)
		{
		}
		IdentityToken_t *pOwner;			/**< Owner of the Handle */
		IdentityToken_t *pIdentity;			/**< Owner of the Type */
	};

	/**
	 * @brief Hooks type-specific Handle operations.
	 */
	class IHandleTypeDispatch
	{
	public:
		/** Returns the Handle API version */
		virtual unsigned int GetDispatchVersion()
		{
			return SMINTERFACE_HANDLESYSTEM_VERSION;
		}
	public:
		/**
		 * @brief Called when destroying a handle.  Must be implemented.
		 *
		 * @param type		Handle type.
		 * @param object	Handle internal object.
		 */
		virtual void OnHandleDestroy(HandleType_t type, void *object) =0;

		/**
		 * @brief Called to get the size of a handle's memory usage in bytes.
		 * Implementation is optional.
		 *
		 * @param type		Handle type.
		 * @param object	Handle internal object.
		 * @param pSize		Pointer to store the approximate memory usage in bytes.
		 * @return			True on success, false if not implemented.
		 */
		virtual bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
		{
			return false;
		}
	};

	/**
	 * @brief Provides functions for managing Handles.
	 */
	class IHandleSys : public SMInterface
	{
	public:
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_HANDLESYSTEM_VERSION;
		}
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_HANDLESYSTEM_NAME;
		}
	public:
		/**
		 * @brief Creates a new Handle type.
		 * NOTE: Currently, a child type may not have its own children.
		 * NOTE: Handle names must be unique if not private.
		 *
		 * @param name		Name of handle type (NULL or "" to be anonymous)
		 * @param dispatch	Pointer to a valid IHandleTypeDispatch object.
		 * @param parent	Parent handle to inherit from, 0 for none.
		 * @param typeAccess	Pointer to a TypeAccess object, NULL to use default 
		 *						or inherited permissions.  Pointer can be temporary.
		 * @param hndlAccess	Pointer to a HandleAccess object to define default
		 *						default permissions on each Handle.  NULL to use default
		 *						permissions.
		 * @param ident		Security token for any permissions.  If typeAccess is NULL, this 
		 *					becomes the owning identity.
		 * @param err		Optional pointer to store an error code on failure (undefined on success).
		 * @return			A new HandleType_t unique ID, or 0 on failure.
		 */
		virtual HandleType_t CreateType(const char *name,
										  IHandleTypeDispatch *dispatch,
										  HandleType_t parent,
										  const TypeAccess *typeAccess,
										  const HandleAccess *hndlAccess,
										  IdentityToken_t *ident,
										  HandleError *err) =0;

		/**
		 * @brief Removes a handle type.
		 * NOTE: This removes all child types.
		 *
		 * @param type		Type chain to remove.
		 * @param ident		Identity token.  Removal fails if the token does not match.
		 * @return			True on success, false on failure.
		 */
		virtual bool RemoveType(HandleType_t type, IdentityToken_t *ident) =0;

		/**
		 * @brief Finds a handle type by name.
		 *
		 * @param name		Name of handle type to find (anonymous not allowed).
		 * @param type		Address to store found handle in (if not found, undefined).
		 * @return			True if found, false otherwise.
		 */
		virtual bool FindHandleType(const char *name, HandleType_t *type) =0;

		/**
		 * @brief Creates a new handle.
		 * 
		 * @param type		Type to use on the handle.
		 * @param object	Object to bind to the handle.
		 * @param owner		Owner of the new Handle (may be NULL).
		 * @param ident		Identity for type access if needed (may be NULL).
		 * @param err		Optional pointer to store an error code on failure (undefined on success).
		 * @return			A new Handle_t, or 0 on failure.
		 */
		virtual Handle_t CreateHandle(HandleType_t type, 
									  void *object,
									  IdentityToken_t *owner,
									  IdentityToken_t *ident,
									  HandleError *err) =0;

		/**
		 * @brief Frees the memory associated with a handle and calls any destructors.
		 * NOTE: This function will decrement the internal reference counter.  It will
		 * only perform any further action if the counter hits 0.
		 *
		 * @param handle	Handle_t identifier to destroy.
		 * @param pSecurity	Security information struct (may be NULL).
		 * @return			A HandleError error code.
		 */
		virtual HandleError FreeHandle(Handle_t handle, const HandleSecurity *pSecurity) =0;

		/**
		 * @brief Clones a handle by adding to its internal reference count.  Its data,
		 * type, and security permissions remain the same.
		 *
		 * @param handle	Handle to duplicate.  Any non-free handle target is valid.
		 * @param newhandle	Stores the duplicated handle in the pointer (must not be NULL).
		 * @param newOwner	New owner of cloned handle.
		 * @param pSecurity	Security information struct (may be NULL).
		 * @return			A HandleError error code.
		 */
		virtual HandleError CloneHandle(Handle_t handle, 
										Handle_t *newhandle, 
										IdentityToken_t *newOwner,
										const HandleSecurity *pSecurity) =0;

		/**
		 * @brief Retrieves the contents of a handle.
		 *
		 * @param handle	Handle_t from which to retrieve contents.
		 * @param type		Expected type to read as.  0 ignores typing rules.
		 * @param pSecurity	Security information struct (may be NULL).
		 * @param object	Optional address to store object in.
		 * @return			HandleError error code.
		 */
		virtual HandleError ReadHandle(Handle_t handle, 
									   HandleType_t type, 
									   const HandleSecurity *pSecurity,
									   void **object) =0;

		/**
		 * @brief Sets access permissions on one or more structures.
		 *
		 * @param pTypeAccess	Optional TypeAccess buffer to initialize with the default values.
		 * @param pHandleAccess	Optional HandleAccess buffer to initialize with the default values.
		 * @return				True on success, false if version is unsupported.
		 */
		virtual bool InitAccessDefaults(TypeAccess *pTypeAccess, HandleAccess *pHandleAccess) =0;

		/**
		 * @brief Creates a new handle.
		 * 
		 * @param type		Type to use on the handle.
		 * @param object	Object to bind to the handle.
		 * @param pSec		Security pointer; pOwner is written as the owner,
		 *					pIdent is used as the parent identity for authorization.
		 * @param pAccess	Access right descriptor for the Handle; NULL for type defaults.
		 * @param err		Optional pointer to store an error code on failure (undefined on success).
		 * @return			A new Handle_t, or 0 on failure.
		 */
		virtual Handle_t CreateHandleEx(HandleType_t type, 
										void *object,
										const HandleSecurity *pSec,
										const HandleAccess *pAccess,
										HandleError *err) =0;

		/**
		 * @brief Clones a handle, bypassing security checks.
		 *
		 * @return          A new Handle_t, or 0 on failure.
		 */
		virtual Handle_t FastCloneHandle(Handle_t hndl) =0;

		/**
		 * @brief Type checks two handles.
		 *
		 * @param given		Type to test.
		 * @param actual	Type to check for.
		 * @return			True if "given" is a subtype of "actual", false otherwise.
		 */
		virtual bool TypeCheck(HandleType_t given, HandleType_t actual) = 0;
	};
}

#endif //_INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_

