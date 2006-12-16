#ifndef _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_

#include <IShareSys.h>
#include <sp_vm_types.h>

#define SMINTERFACE_HANDLESYSTEM_NAME			"IHandleSys"
#define SMINTERFACE_HANDLESYSTEM_VERSION		1

namespace SourceMod
{
	/**
	 * Both of these types have invalid values of '0' for error checking.
	 */
	typedef unsigned int HandleType_t;
	typedef unsigned int Handle_t;

	enum HandleError
	{
		HandleError_None = 0,		/* No error */
		HandleError_Changed,		/* The handle has been freed and reassigned */
		HandleError_Type,			/* The handle has a different type registered */
		HandleError_Freed,			/* The handle has been freed */
		HandleError_Index,			/* generic internal indexing error */
		HandleError_Access,			/* No access permitted to free this handle */
	};

	struct HandleAccess
	{
		HandleAccess() : canRead(true), canDelete(true), canInherit(true)
		{
		}
		bool canCreate;		/* Instances can be created by other objects */
		bool canRead;		/* Handle and type can be read by other objects */
		bool canDelete;		/* Handle can be deleted by other objects */
		bool canInherit;	/* Handle type can be inherited */
	};

	struct HandleSecurity
	{
		IdentityToken_t owner;		/* Owner of the handle */
		HandleAccess all;			/* Access permissions of everyone */
	};

	class IHandleTypeDispatch
	{
	public:
		virtual unsigned int GetInterfaceVersion()
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
		 * NOTE: Handle names must be unique if not private.
		 *
		 * @param name		Name of handle type (NULL or "" to be anonymous)
		 * @param dispatch	Pointer to a valid IHandleTypeDispatch object.
		 * @return			A new HandleType_t unique ID.
		 */
		virtual HandleType_t CreateType(const char *name, 
										IHandleTypeDispatch *dispatch) =0;

		/**
		 * @brief Creates a new Handle type.
		 * NOTE: Currently, a child type may not have its own children.
		 * NOTE: Handle names must be unique if not private.
		 *
		 * @param name		Name of handle type (NULL or "" to be anonymous)
		 * @param dispatch	Pointer to a valid IHandleTypeDispatch object.
		 * @param parent	Parent handle to inherit from, 0 for none.
		 * @param security	Pointer to a temporary HandleSecurity object, NULL to use defaults.
		 * @return			A new HandleType_t unique ID.
		 */
		virtual HandleType_t CreateTypeEx(const char *name,
										  IHandleTypeDispatch *dispatch,
										  HandleType_t parent,
										  const HandleSecurity *security) =0;


		/**
		 * @brief Creates a sub-type for a Handle.
		 * NOTE: Currently, a child type may not have its own children.
		 * NOTE: Handle names must be unique if not private.
		 * NOTE: This is a wrapper around the above.
		 *
		 * @param name		Name of a handle.
		 * @param parent	Parent handle type.
		 * @param dispatch	Pointer to a valid IHandleTypeDispatch object.
		 * @return			A new HandleType_t unique ID.
		 */
		virtual HandleType_t CreateChildType(const char *name, 
										     HandleType_t parent, 
										     IHandleTypeDispatch *dispatch) =0;

		/**
		 * @brief Removes a handle type.
		 * NOTE: This removes all child types.
		 *
		 * @param token		Identity token.  Removal fails if the token does not match.
		 * @param type		Type chain to remove.
		 * @return			True on success, false on failure.
		 */
		virtual bool RemoveType(HandleType_t type, IdentityToken_t ident) =0;

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
		 * @param owner		Identity token for object using this handle.
		 * @param ident		Identity token if any security rights are needed.
		 * @return			A new Handle_t, or 0 on failure.
		 */
		virtual Handle_t CreateHandleEx(HandleType_t type, 
										void *object, 
										IdentityToken_t owner, 
										IdentityToken_t ident) =0;

		/**
		 * @brief Creates a new handle.
		 * NOTE: This is a wrapper around the above function.
		 * 
		 * @param type		Type to use on the handle.
		 * @param object	Object to bind to the handle.
		 * @param ctx		Plugin context that will own this handle.  NULL for none.
		 * @return			A new Handle_t.
		 */
		virtual Handle_t CreateScriptHandle(HandleType_t type, void *object, sp_context_t *ctx) =0;

		/**
		 * @brief Destroys a handle.
		 *
		 * @param type		Handle_t identifier to destroy.
		 * @return			A HandleError error code.
		 */
		virtual HandleError DestroyHandle(Handle_t handle) =0;

		/**
		 * @brief Retrieves the contents of a handle.
		 *
		 * @param handle	Handle_t from which to retrieve contents.
		 * @param type		Expected type to read as.
		 * @param object	Address to store object in.
		 * @return			HandleError error code.
		 */
		virtual HandleError ReadHandle(Handle_t handle, HandleType_t type, void **object) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_HANDLESYSTEM_INTERFACE_H_
