#ifndef _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
#define _INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_

namespace SourceMod
{
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

	enum UnloadableParentType
	{
		ParentType_Module,
		ParentType_Plugin
	};

	/**
	 * @brief Denotes a top-level unloadable object.
	 */
	class IUnloadableParent
	{
	public:
		virtual UnloadableParentType GetParentType() =0;

		virtual void *GetParentToken() =0;
	
		/**
		 * @brief Called when an interface this object has requested is removed.
		 *
		 * @param pIface		Interface being removed.
		 */
		virtual void OnInterfaceUnlink(SMInterface *pIface) =0;
	protected:
		void *m_parent_token;
	};

	/**
	 * @brief Listens for unlinked objects.
	 */
	class IUnlinkListener
	{
	public:
		/**
		 * @brief Called when a parent object is unloaded.
		 *
		 * @param parent		The parent object which is dying.
		 */
		virtual void OnParentUnlink(IUnloadableParent *parent)
		{
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
		 * @param parent		Parent unloadable token given to the module/interface.
		 */
		virtual bool AddInterface(SMInterface *iface, IUnloadableParent *parent) =0;

		/**
		 * @brief Requests an interface from the global interface system.
		 * If found, the interface's internal reference count will be increased.
		 *
		 * @param iface_name	Interface name.
		 * @param iface_vers	Interface version to attempt to match.
		 * @param me			Object requesting this interface, in order to track dependencies.
		 * @param pIface		Pointer to store the return value in.
		 */
		virtual bool RequestInterface(const char *iface_name, 
										unsigned int iface_vers, 
										IUnloadableParent *me,
										void **pIface) =0;

		/**
		 * @brief Unloads an interface.
		 *
		 * @param iface			Interface pointer.
		 * @param parent		Security token, trivial measure to prevent accidental unloads.
		 *                      This token must match the one used with AddInterface().
		 */
		virtual void RemoveInterface(SMInterface *iface, IUnloadableParent *parent) =0;

		/**
		 * @brief Adds an unlink listener.
		 *
		 * @param pListener		Listener pointer.
		 */
		virtual void AddUnlinkListener(IUnlinkListener *pListener) =0;

		/**
		 * @brief Removes an unlink listener.
		 *
		 * @param pListener		Listener pointer.
		 */
		virtual void RemoveUnlinkListener(IUnlinkListener *pListener) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_IFACE_SHARE_SYS_H_
