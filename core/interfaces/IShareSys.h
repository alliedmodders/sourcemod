#ifndef _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
#define _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_

#include <sp_vm_types.h>

#define	DEFAULT_IDENTITY		0

namespace SourceMod
{
	typedef unsigned int		IdentityToken_t;
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
		 * @brief Adds an interface to the global interface system
		 *
		 * @param iface			Interface pointer (must be unique).
		 * @param token			Parent token of the module/interface.
		 */
		virtual bool AddInterface(SMInterface *iface, IdentityToken_t token) =0;

		/**
		 * @brief Requests an interface from the global interface system.
		 * If found, the interface's internal reference count will be increased.
		 *
		 * @param iface_name	Interface name.
		 * @param iface_vers	Interface version to attempt to match.
		 * @param token			Object requesting this interface, in order to track dependencies.
		 * @param pIface		Pointer to store the return value in.
		 */
		virtual bool RequestInterface(const char *iface_name, 
										unsigned int iface_vers,
										IdentityToken_t token,
										void **pIface) =0;

		/**
		 * @brief Adds a list of natives to the global native pool.
		 * 
		 * @param token			Identity token of parent object.
		 * @param natives		Array of natives to add, NULL terminated.
		 */
		virtual void AddNatives(IdentityToken_t token, const sp_nativeinfo_t *natives[]) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
