#ifndef _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_

#include <IExtensionSys.h>
#include <ILibrarySys.h>
#include <sh_list.h>
#include <sh_string.h>
#include "sm_globals.h"
#include "ShareSys.h"
#include <ISmmAPI.h>
#include <IPluginSys.h>
#include <IRootConsoleMenu.h>

using namespace SourceMod;
using namespace SourceHook;

class CExtension : public IExtension
{
	friend class CExtensionManager;
public:
	CExtension(const char *filename, char *error, size_t maxlen);
	~CExtension();
public: //IExtension
	IExtensionInterface *GetAPI();
	const char *GetFilename();
	IdentityToken_t *GetIdentity();
	bool IsLoaded();
	ITERATOR *FindFirstDependency(IExtension **pOwner, SMInterface **pInterface);
	bool FindNextDependency(ITERATOR *iter, IExtension **pOwner, SMInterface **pInterface);
	void FreeDependencyIterator(ITERATOR *iter);
	bool IsRunning(char *error, size_t maxlength);
public:
	void SetError(const char *error);
	void AddDependency(IfaceInfo *pInfo);
	void AddInterface(SMInterface *pInterface);
	void AddPlugin(IPlugin *pPlugin);
	void RemovePlugin(IPlugin *pPlugin);
	void MarkAllLoaded();
private:
	IdentityToken_t *m_pIdentToken;
	IExtensionInterface *m_pAPI;
	String m_File;
	ILibrary *m_pLib;
	String m_Error;
	List<IfaceInfo> m_Deps;
	List<SMInterface *> m_Interfaces;
	List<IPlugin *> m_Plugins;
	List<const sp_nativeinfo_t *> m_Natives;
	PluginId m_PlId;
	unsigned int unload_code;
	bool m_FullyLoaded;
};

class CExtensionManager : 
	public IExtensionManager,
	public SMGlobalClass,
	public IPluginsListener,
	public IRootConsoleCommand
{
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IExtensionManager
	IExtension *LoadExtension(const char *path, 
		ExtensionLifetime lifetime, 
		char *error,
		size_t err_max);
	bool UnloadExtension(IExtension *pExt);
	IExtension *FindExtensionByFile(const char *file);
	IExtension *FindExtensionByName(const char *ext);
public: //IPluginsListener
	void OnPluginDestroyed(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmd, unsigned int argcount);
public:
	IExtension *LoadAutoExtension(const char *path);
	void BindDependency(IExtension *pOwner, IfaceInfo *pInfo);
	void AddInterface(IExtension *pOwner, SMInterface *pInterface);
	void BindChildPlugin(IExtension *pParent, IPlugin *pPlugin);
	void AddNatives(IExtension *pOwner, const sp_nativeinfo_t *natives);
	void BindAllNativesToPlugin(IPlugin *pPlugin);
	void MarkAllLoaded();
private:
	CExtension *FindByOrder(unsigned int num);
private:
	List<CExtension *> m_Libs;
};

extern CExtensionManager g_Extensions;

#endif //_INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
