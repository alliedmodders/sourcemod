#ifndef _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
#define _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_

#include <sp_vm_types.h>

#define	NO_IDENTITY		0

namespace SourceMod
{
	class IExtension;
	struct IdentityToken_t;
	typedef unsigned int		HandleType_t;
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
		 * @param token			Identity token of parent object.
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
	};
};

#endif //_INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
