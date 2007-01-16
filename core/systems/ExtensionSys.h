#ifndef _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_

#include <IExtensionSys.h>
#include <ILibrarySys.h>
#include <sh_list.h>
#include <sh_string.h>
#include "sm_globals.h"

using namespace SourceMod;
using namespace SourceHook;

class CExtension : public IExtension
{
public:
	CExtension(const char *filename, char *error, size_t maxlen);
	~CExtension();
public: //IExtension
	IExtensionInterface *GetAPI();
	const char *GetFilename();
	IdentityToken_t *GetIdentity();
	bool IsLoaded();
public:
	void SetError(const char *error);
private:
	IdentityToken_t *m_pIdentToken;
	IExtensionInterface *m_pAPI;
	String m_File;
	ILibrary *m_pLib;
	String m_Error;
};

class CExtensionManager : 
	public IExtensionManager,
	public SMGlobalClass
{
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IExtensionManager
	IExtension *LoadExtension(const char *path, 
		ExtensionLifetime lifetime, 
		char *error,
		size_t err_max);
	unsigned int NumberOfPluginDependents(IExtension *pExt, unsigned int *optional);
	bool IsExtensionUnloadable(IExtension *pExtension);
	bool UnloadExtension(IExtension *pExt);
	IExtension *FindExtensionByFile(const char *file);
	IExtension *FindExtensionByName(const char *ext);
public:
	IExtension *LoadAutoExtension(const char *path);
private:
	List<CExtension *> m_Libs;
};

extern CExtensionManager g_Extensions;

#endif //_INCLUDE_SOURCEMOD_EXTENSION_SYSTEM_H_
