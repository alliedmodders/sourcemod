#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_

#include <IShareSys.h>
#include <sp_vm_types.h>

#define SMINTERFACE_HANDLESYSTEM_NAME			"IHandleSys"
#define SMINTERFACE_HANDLESYSTEM_VERSION		1

#define DEFAULT_IDENTITY			NULL

namespace SourceMod
{
	/**
	 * Both of these types have invalid values of '0' for error checking.
	 */
	typedef unsigned int HandleType_t;
	typedef unsigned int Handle_t;


	/**
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

	enum HandleError
	{
		HandleError_None = 0,		/* No error */
		HandleError_Changed,		/* The handle has been freed and reassigned */
		HandleError_Type,			/* The handle has a different type registered */
		HandleError_Freed,			/* The handle has been freed */
		HandleError_Index,			/* generic internal indexing error */
		HandleError_Access,			/* No access permitted to free this handle */
		HandleError_Limit,			/* The limited number of handles has been reached */
		HandleError_Identity,		/* The identity token was not usable */
		HandleError_Owner,			/* Owners do not match for this operation */
		HandleError_Version,		/* Unrecognized security structure version */
		HandleError_Parameter,		/* An invalid parameter was passed */
		HandleError_NoInherit,		/* This type cannot be inherited */
	};

	/**
	 * Access rights specific to a type
	 */
	enum HTypeAccessRight
	{
		HTypeAccess_Create = 0,		/* Handles of this type can be created (DEFAULT=false) */
		HTypeAccess_Inherit,		/* Sub-types can inherit this type (DEFAULT=false) */
		/* -------------- */
		HTypeAccess_TOTAL,			/* Total number of type access rights */
	};

	/**
	 * Access rights specific to a Handle.  These rights are exclusive.
	 * For example, you do not need "read" access to delete or clone.
	 */
	enum HandleAccessRight
	{
		HandleAccess_Read,			/* Can be read (DEFAULT=ident only) */
		HandleAccess_Delete,		/* Can be deleted (DEFAULT=owner only) */
		HandleAccess_Clone,			/* Can be cloned (DEFAULT=any) */
		/* ------------- */
		HandleAccess_TOTAL,			/* Total number of access rights */
	};

	#define HANDLE_RESTRICT_IDENTITY	(1<<0)	/* Access is restricted to the identity */
	#define HANDLE_RESTRICT_OWNER		(1<<1)	/* Access is restricted to the owner */

	/**
	 * This is used to define per-type access rights.
	 */
	struct TypeAccess
	{
		TypeAccess()
		{
			hsVersion = SMINTERFACE_HANDLESYSTEM_VERSION;
		}
		unsigned int hsVersion;
		IdentityToken_t *ident;
		bool access[HTypeAccess_TOTAL];
	};

	/**
	 * This is used to define per-Handle access rights.
	 */
	struct HandleAccess
	{
		HandleAccess()
		{
			hsVersion = SMINTERFACE_HANDLESYSTEM_VERSION;
		}
		unsigned int hsVersion;
		unsigned int access[HandleAccess_TOTAL];
	};

	/**
	 * This pair of tokens is used for identification.
	 */
	struct HandleSecurity
	{
		IdentityToken_t *pOwner;			/* Owner of the Handle */
		IdentityToken_t *pIdentity;			/* Owner of the Type */
	};

	class IHandleTypeDispatch
	{
	public:
		virtual unsigned int GetDispatchVersion()
		{
			return SMINTERFACE_HANDLESYSTEM_VERSION;
		}
	public:
		/**
		 * @brief Called when destroying a handle.  Must be implemented.
		 */
		virtual void OnHandleDestroy(HandleType_t type, void *object) =0;
	};

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
		 * @param err		Optional pointer to store an error code.
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
		 * @param err		Optional pointer to store an error code.
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
		 * @param type		Handle_t identifier to destroy.
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
		 * @return			True on success, false if version is unsupported.
		 */
		virtual bool InitAccessDefaults(TypeAccess *pTypeAccess, HandleAccess *pHandleAccess) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_
