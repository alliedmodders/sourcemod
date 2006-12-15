#ifndef _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_

#include <IShareSys.h>
#include <ILibrarySys.h>

namespace SourceMod
{
	class IModuleInterface;

	class IModule
	{
	public:
		virtual IModuleInterface *GetModuleInfo() =0;
		virtual const char *GetFilename() =0;
		virtual IdentityToken_t GetIdentityToken() =0;
	};

	class IModuleInterface
	{
	public:
		/**
		 * @brief Called when the module is loaded.
		 * 
		 * @param me		Pointer back to module.
		 * @param token		Identity token handle.
		 * @param sys		Pointer to interface sharing system of SourceMod.
		 * @param error		Error buffer to print back to, if any.
		 * @param err_max	Maximum size of error buffer.
		 * @param late		If this module was loaded "late" (i.e. manually).
		 * @return			True if load should continue, false otherwise.
		 */
		virtual bool OnModuleLoad(IModule *me,
								  IdentityToken_t token,
								  IShareSys *sys, 
								  char *error, 
								  size_t err_max, 
								  bool late) =0;

		/**
		 * @brief Called when the module is unloaded.
		 *
		 * @param force		True if this unload will be forced.
		 * @param error		Error message buffer.
		 * @param err_max	Maximum siez of error buffer.
		 * @return			True on success, false to request no unload.
		 */
		virtual bool OnModuleUnload(bool force, char *error, size_t err_max) =0;

		/**
		 * @brief Called when your pause state is about to change.
		 * 
		 * @param pause		True if pausing, false if unpausing.
		 */
		virtual void OnPauseChange(bool pause) =0;
	public:
		virtual const char *GetModuleName() =0;
		virtual const char *GetModuleVersion() =0;
		virtual const char *GetModuleURL() =0;
		virtual const char *GetModuleTags() =0;
		virtual const char *GetModuleAuthor() =0;
	};

	#define SMINTERFACE_MODULEMANAGER_NAME		"IModuleManager"
	#define SMINTERFACE_MODULEMANAGER_VERSION		1

	enum ModuleLifetime
	{
		ModuleLifetime_Forever,				//Module will never be unloaded automatically
		ModuleLifetime_Map,					//Module will be unloaded at the end of the map
		ModuleLifetime_Dependent,			//Module will be unloaded once its dependencies are gone
	};

	class IModuleManager : public SMInterface
	{
	public:
		/**
		 * @brief Loads a module into the module system.
		 *
		 * @param path		Path to module file, relative to the modules folder.
		 * @param lifetime	Lifetime of the module.
		 * @param error		Error buffer.
		 * @param err_max	Maximum error buffer length.
		 * @return			New IModule on success, NULL on failure.
		 */
		virtual IModule *LoadModule(const char *path, 
									ModuleLifetime lifetime, 
									char *error,
									size_t err_max);
	};
};

#endif //_INCLUDE_SOURCEMOD_MODULE_INTERFACE_H_
