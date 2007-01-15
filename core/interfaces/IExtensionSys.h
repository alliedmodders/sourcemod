#ifndef _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_

#include <IShareSys.h>
#include <ILibrarySys.h>

namespace SourceMod
{
	class IExtensionInterface;

	/** 
	 * @brief Encapsulates an IExtension.
	 */
	class IExtension
	{
	public:
		/**
		 * @brief Returns the extension's API interface
		 *
		 * @return			An IExtensionInterface pointer.
		 */
		virtual IExtensionInterface *GetAPI() =0;

		/**
		 * @brief Returns the filename of the extension, relative to the
		 * extension folder.
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
	};

	#define SMINTERFACE_EXTENSIONAPI_VERSION	1

	/**
	 * @brief The interface an extension must expose.
	 */
	class IExtensionInterface
	{
	public:
		/**
		 * @brief Called when the extension is loaded.
		 * 
		 * @param me		Pointer back to extension.
		 * @param sys		Pointer to interface sharing system of SourceMod.
		 * @param error		Error buffer to print back to, if any.
		 * @param err_max	Maximum size of error buffer.
		 * @param late		If this extension was loaded "late" (i.e. manually).
		 * @return			True if load should continue, false otherwise.
		 */
		virtual bool OnExtensionLoad(IExtension *me,
								  IShareSys *sys, 
								  char *error, 
								  size_t err_max, 
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

		virtual unsigned int GetExtensionVersion()
		{
			return SMINTERFACE_EXTENSIONAPI_VERSION;
		}

		/**
		 * @brief Returns true if the extension is Metamod-dependent.
		 */
		virtual bool IsMetamodExtension() =0;
	public:
		virtual const char *GetExtensionName() =0;
		virtual const char *GetExtensionURL() =0;
		virtual const char *GetExtensionTag() =0;
		virtual const char *GetExtensionAuthor() =0;
		virtual const char *GetExtensionVerString() =0;
		virtual const char *GetExtensionDescription() =0;
		virtual const char *GetExtensionDateString() =0;
	};

	#define SMINTERFACE_EXTENSIONMANAGER_NAME			"IExtensionManager"
	#define SMINTERFACE_EXTENSIONMANAGER_VERSION		1

	enum ExtensionLifetime
	{
		ExtLifetime_Forever,			//Extension will never be unloaded automatically
		ExtLifetime_Map,				//Extension will be unloaded at the end of the map
	};

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
	public:
		/**
		 * @brief Loads a extension into the extension system.
		 *
		 * @param path		Path to extension file, relative to the extensions folder.
		 * @param lifetime	Lifetime of the extension.
		 * @param error		Error buffer.
		 * @param err_max	Maximum error buffer length.
		 * @return			New IExtension on success, NULL on failure.
		 */
		virtual IExtension *LoadModule(const char *path, 
									ExtensionLifetime lifetime, 
									char *error,
									size_t err_max) =0;

		/**
		 * @brief Returns the number of plugins that will be unloaded when this
		 * module is unloaded.
		 *
		 * @param pExt		IExtension pointer.
		 * @param optional	Optional pointer to be filled with # of plugins that 
		 *					are dependent, but will continue safely.  NOT YET USED.
		 * @return			Total number of dependent plugins.
		 */
		virtual unsigned int NumberOfPluginDependents(IExtension *pExt, unsigned int *optional) =0;

		/**
		 * @brief Returns whether or not the extension can be unloaded.
		 *
		 * @param pExt		IExtension pointer.
		 * @return			True if unloading is possible, false otherwise.
		 */
		virtual bool IsExtensionUnloadable(IExtension *pExtension) =0;

		/**
		 * @brief Attempts to unload a module.
		 *
		 * @param pExt		IExtension pointer.
		 * @return			True if successful, false otherwise.
		 */
		virtual bool UnloadModule(IExtension *pExt) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_
