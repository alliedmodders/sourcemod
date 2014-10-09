/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_INTERCOM_H_
#define _INCLUDE_SOURCEMOD_INTERCOM_H_

#include <sp_vm_api.h>
#include <IHandleSys.h>
#include <IShareSys.h>
#include <IPluginSys.h>
#include <IDBDriver.h>
#include <sh_string.h>
#include <sp_vm_api.h>
#include <sh_vector.h>
#include <IExtensionSys.h>
#include <IForwardSys.h>
#include <IAdminSystem.h>

using namespace SourceMod;
using namespace SourcePawn;
using namespace SourceHook;

/**
 * Add 1 to the RHS of this expression to bump the intercom file
 * This is to prevent mismatching core/logic binaries
 */
#define SM_LOGIC_MAGIC		(0x0F47C0DE - 29)

#if defined SM_LOGIC
class IVEngineServer
#else
class IVEngineServer_Logic
#endif
{
public:
	virtual bool IsMapValid(const char *map) = 0;
	virtual bool IsDedicatedServer() = 0;
	virtual void InsertServerCommand(const char *cmd) = 0;
	virtual void ServerCommand(const char *cmd) = 0;
	virtual void ServerExecute() = 0;
	virtual const char *GetClientConVarValue(int clientIndex, const char *name) = 0;
	virtual void ClientCommand(edict_t *pEdict, const char *szCommand) = 0;
	virtual void FakeClientCommand(edict_t *pEdict, const char *szCommand) = 0;
};

typedef void * FileHandle_t;
typedef int FileFindHandle_t; 

#if defined SM_LOGIC
class IFileSystem
#else
class IFileSystem_Logic
#endif
{
public:
	virtual const char *FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle) = 0;
	virtual const char *FindNext(FileFindHandle_t handle) = 0;
	virtual bool FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void FindClose(FileFindHandle_t handle) = 0;
	virtual FileHandle_t Open(const char *pFileName, const char *pOptions, const char *pathID = 0) = 0;
	virtual void Close(FileHandle_t file) = 0;
	virtual char *ReadLine(char *pOutput, int maxChars, FileHandle_t file) = 0;
	virtual bool EndOfFile(FileHandle_t file) = 0;
	virtual bool FileExists(const char *pFileName, const char *pPathID = 0) = 0;
	virtual unsigned int Size(const char *pFileName, const char *pPathID = 0) = 0;
	virtual int Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual void Seek(FileHandle_t file, int post, int seekType) = 0;
	virtual unsigned int Tell(FileHandle_t file) = 0;
	virtual int FPrint(FileHandle_t file, const char *pData) = 0;
	virtual void Flush(FileHandle_t file) = 0;
	virtual bool IsOk(FileHandle_t file) = 0;
	virtual void RemoveFile(const char *pRelativePath, const char *pathID = 0) = 0;
	virtual void RenameFile(char const *pOldPath, char const *pNewPath, const char *pathID = 0) = 0;
	virtual bool IsDirectory(const char *pFileName, const char *pathID = 0) = 0;
	virtual void CreateDirHierarchy(const char *path, const char *pathID = 0) = 0;
};

namespace SourceMod
{
	class ISourceMod;
	class ILibrarySys;
	class ITextParsers;
	class IThreader;
	class IRootConsole;
	class IPluginManager;
	class IForwardManager;
	class ITimerSystem;
	class IPlayerManager;
	class IAdminSystem;
	class IGameHelpers;
	class IPhraseCollection;
	class ITranslator;
	class IGameConfig;
	class IMenuManager;
}

class IVEngineServer;
class IFileSystem;
class ConVar;
class KeyValues;
class SMGlobalClass;
class IPlayerInfo;

class IPlayerInfo_Logic
{
public:
	virtual bool IsObserver(IPlayerInfo *pInfo) = 0;
	virtual int GetTeamIndex(IPlayerInfo *pInfo) = 0;
	virtual int GetFragCount(IPlayerInfo *pInfo) = 0;
	virtual int GetDeathCount(IPlayerInfo *pInfo) = 0;
	virtual int GetArmorValue(IPlayerInfo *pInfo) = 0;
	virtual void GetAbsOrigin(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetAbsAngles(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetPlayerMins(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual void GetPlayerMaxs(IPlayerInfo *pInfo, float *x, float *y, float *z) = 0;
	virtual const char *GetWeaponName(IPlayerInfo *pInfo) = 0;
	virtual const char *GetModelName(IPlayerInfo *pInfo) = 0;
	virtual int GetHealth(IPlayerInfo *pInfo) = 0;
	virtual void ChangeTeam(IPlayerInfo *pInfo, int iTeamNum) = 0;
};

namespace SourceMod
{
	class IChangeableForward;
}

struct ServerGlobals
{
	const double *universalTime;
	float *interval_per_tick;
	float *frametime;
};

struct AutoConfig
{
	SourceHook::String autocfg;
	SourceHook::String folder;
	bool create;
};

enum LibraryAction
{
	LibraryAction_Removed,
	LibraryAction_Added
};

class CNativeOwner;

class SMPlugin : public IPlugin
{
public:
	virtual size_t GetConfigCount() = 0;
	virtual AutoConfig *GetConfig(size_t i) = 0;
	virtual void AddLibrary(const char *name) = 0;
	virtual void AddConfig(bool create, const char *cfg, const char *folder) = 0;
	virtual void SetErrorState(PluginStatus status, const char *fmt, ...) = 0;
};

class IScriptManager
{
public:
	virtual void LoadAll(const char *config_path, const char *plugins_path) = 0;
	virtual void RefreshAll() = 0;
	virtual void Shutdown() = 0;
	virtual IdentityToken_t *GetIdentity() = 0;
	virtual void SyncMaxClients(int maxClients) = 0;
	virtual void AddPluginsListener(IPluginsListener *listener) = 0;
	virtual void RemovePluginsListener(IPluginsListener *listener) = 0;
	virtual IPluginIterator *GetPluginIterator() = 0;
	virtual void OnLibraryAction(const char *name, LibraryAction action) = 0;
	virtual bool LibraryExists(const char *name) = 0;
	virtual SMPlugin *FindPluginByOrder(unsigned num) = 0;
	virtual SMPlugin *FindPluginByIdentity(IdentityToken_t *ident) = 0;
	virtual SMPlugin *FindPluginByContext(IPluginContext *ctx) = 0;
	virtual SMPlugin *FindPluginByContext(sp_context_t *ctx) = 0;
	virtual SMPlugin *FindPluginByConsoleArg(const char *text) = 0;
	virtual SMPlugin *FindPluginByHandle(Handle_t hndl, HandleError *errp) = 0;
	virtual bool UnloadPlugin(IPlugin *plugin) = 0;
	virtual const CVector<SMPlugin *> *ListPlugins() = 0;
	virtual void FreePluginList(const CVector<SMPlugin *> *list) = 0;
	virtual void AddFunctionsToForward(const char *name, IChangeableForward *fwd) = 0;
};

class IExtensionSys : public IExtensionManager
{
public:
	virtual IExtension *LoadAutoExtension(const char *name, bool bErrorOnMissing=true) = 0;
	virtual void TryAutoload() = 0;
	virtual void Shutdown() = 0;
	virtual IExtension *FindExtensionByFile(const char *name) = 0;
	virtual bool LibraryExists(const char *name) = 0;
	virtual void CallOnCoreMapStart(edict_t *edictList, int edictCount, int maxClients) = 0;
	virtual IExtension *GetExtensionFromIdent(IdentityToken_t *token) = 0;
	virtual void BindChildPlugin(IExtension *ext, SMPlugin *plugin) = 0;
	virtual void AddRawDependency(IExtension *myself, IdentityToken_t *token, void *iface) = 0;
	virtual const CVector<IExtension *> *ListExtensions() = 0;
	virtual void FreeExtensionList(const CVector<IExtension *> *list) = 0;
	virtual void CallOnCoreMapEnd() = 0;
};

class ILogger
{
public:
	virtual void LogMessage(const char *msg, ...) = 0;
	virtual void LogError(const char *msg, ...) = 0;
	virtual void LogFatal(const char *msg, ...) = 0;
};

class AutoPluginList
{
public:
	AutoPluginList(IScriptManager *scripts)
		: scripts_(scripts), list_(scripts->ListPlugins())
	{
	}
	~AutoPluginList()
	{
		scripts_->FreePluginList(list_);
	}
	const CVector<SMPlugin *> *operator ->()
	{
		return list_;
	}
private:
	IScriptManager *scripts_;
	const CVector<SMPlugin *> *list_;
};

class AutoExtensionList
{
public:
	AutoExtensionList(IExtensionSys *extensions)
		: extensions_(extensions), list_(extensions_->ListExtensions())
	{
	}
	~AutoExtensionList()
	{
		extensions_->FreeExtensionList(list_);
	}
	const CVector<IExtension *> *operator ->()
	{
		return list_;
	}
private:
	IExtensionSys *extensions_;
	const CVector<IExtension *> *list_;
};

class CCommand;

struct sm_core_t
{
	/* Objects */
	ISourceMod		*sm;
	ILibrarySys		*libsys;
	IVEngineServer	*engine;
	IFileSystem		*filesystem;
	IPlayerInfo_Logic *playerInfo;
	IRootConsole	*rootmenu;
	ITimerSystem    *timersys;
	IPlayerManager  *playerhelpers;
	IGameHelpers    *gamehelpers;
	IMenuManager	*menus;
	ISourcePawnEngine **spe1;
	ISourcePawnEngine2 **spe2;
	/* Functions */
	ConVar *		(*FindConVar)(const char*);
	unsigned int	(*strncopy)(char*, const char*, size_t);
	char *			(*TrimWhitespace)(char *, size_t &);
	void			(*LogToGame)(const char *message);
	void			(*ConPrint)(const char *message);
	const char *	(*GetCvarString)(ConVar*);
	bool			(*GetCvarBool)(ConVar*);
	size_t			(*Format)(char*, size_t, const char*, ...);
	size_t			(*FormatArgs)(char*, size_t, const char*,va_list ap);
	bool			(*gnprintf)(char *, size_t, const char *, IPhraseCollection *, void **,
	                            unsigned int, unsigned int &, size_t *, const char **);
	size_t			(*atcprintf)(char *, size_t, const char *, IPluginContext *, const cell_t *, int *);
	bool            (*GetGameName)(char *buffer, size_t maxlength);
	const char *    (*GetGameDescription)();
	const char *    (*GetSourceEngineName)();
	bool            (*SymbolsAreHidden)();
	const char *	(*GetCoreConfigValue)(const char*);
	bool			(*IsMapLoading)();
	bool			(*IsMapRunning)();
	int				(*Argc)(const CCommand &args);
	const char *	(*Arg)(const CCommand &args, int arg);
	int				(*LoadMMSPlugin)(const char *file, bool *ok, char *error, size_t maxlength);
	void			(*UnloadMMSPlugin)(int id);
	void			(*DoGlobalPluginLoads)();
	bool			(*AreConfigsExecuted)();
	void			(*ExecuteConfigs)(IPluginContext *ctx);
	DatabaseInfo	(*GetDBInfoFromKeyValues)(KeyValues *);
	int				(*GetActivityFlags)();
	int				(*GetImmunityMode)();
	void			(*UpdateAdminCmdFlags)(const char *cmd, OverrideType type, FlagBits bits, bool remove);
	bool			(*LookForCommandAdminFlags)(const char *cmd, FlagBits *pFlags);
	const char		*gamesuffix;
	/* Data */
	ServerGlobals   *serverGlobals;
	void *          serverFactory;
	void *          engineFactory;
	void *          matchmakingDSFactory;
	SMGlobalClass *	listeners;
};

struct sm_logic_t
{
	SMGlobalClass	*head;
	IThreader		*threader;
	ITranslator		*translator;
	const char      *(*stristr)(const char *, const char *);
	bool			(*CoreTranslate)(char *,  size_t, const char *, unsigned int, size_t *, ...);
	void            (*AddCorePhraseFile)(const char *filename);
	unsigned int	(*ReplaceAll)(char*, size_t, const char *, const char *, bool);
	char            *(*ReplaceEx)(char *, size_t, const char *, size_t, const char *, size_t, bool);
	size_t          (*DecodeHexString)(unsigned char *, size_t, const char *);
	IGameConfig *   (*GetCoreGameConfig)();
	bool			(*OnLogPrint)(const char *msg);	// true to supersede
	IDebugListener   *debugger;
	void			(*GenerateError)(IPluginContext *, cell_t, int, const char *, ...);
	void			(*AddNatives)(sp_nativeinfo_t *natives);
	void			(*DumpHandles)(void (*dumpfn)(const char *fmt, ...));
	bool			(*DumpAdminCache)(const char *filename);
	void            (*RegisterProfiler)(IProfilingTool *tool);
	IScriptManager	*scripts;
	IShareSys		*sharesys;
	IExtensionSys	*extsys;
	IHandleSys		*handlesys;
	IForwardManager	*forwardsys;
	IAdminSystem	*adminsys;
	IdentityToken_t *core_ident;
	ILogger			*logger;
	float			sentinel;
};

typedef void (*LogicInitFunction)(const sm_core_t *core, sm_logic_t *logic);
typedef LogicInitFunction (*LogicLoadFunction)(uint32_t magic);
typedef ITextParsers *(*GetITextParsers)();

#endif /* _INCLUDE_SOURCEMOD_INTERCOM_H_ */
