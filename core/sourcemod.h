/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_GLOBALHEADER_H_
#define _INCLUDE_SOURCEMOD_GLOBALHEADER_H_

#include "sm_globals.h"
#include <ISourceMod.h>
#include <sh_stack.h>
#include <sh_vector.h>

using namespace SourceHook;

#if defined _DEBUG
# define IF_DEBUG_SPEW
# define ENDIF_DEBUG_SPEW
#else
# define IF_DEBUG_SPEW			\
	if (sm_show_debug_spew)		\
	{
# define ENDIF_DEBUG_SPEW		\
	}
#endif

/**
 * @brief Implements SourceMod's global overall management, API, and logic
 */

class SourceModBase : 
	public ISourceMod,
	public SMGlobalClass
{
public:
	SourceModBase();
public:
	/**
	 * @brief Initializes SourceMod, or returns an error on failure.
	 */
	bool InitializeSourceMod(char *error, size_t maxlength, bool late);

	/** 
	 * @brief Starts everything SourceMod needs to run
	 */
	void StartSourceMod(bool late);

	/** 
	 * @brief Shuts down all SourceMod components
	 */
	void CloseSourceMod();

	/**
	 * @brief Map change hook
	 */
	bool LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);

	/**
	 * @brief Level shutdown hook
	 */
	void LevelShutdown();

	/** 
	 * @brief Returns whether or not a map load is in progress
	 */
	bool IsMapLoading() const;

	/** 
	 * @brief Stores the global target index.
	 */
	unsigned int SetGlobalTarget(unsigned int index);

	/** 
	 * @brief Returns the global target index.
	 */
	unsigned int GetGlobalTarget() const;
public: // SMGlobalClass
	ConfigResult OnSourceModConfigChanged(const char *key, 
										  const char *value, 
										  ConfigSource source, 
										  char *error, 
										  size_t maxlength);
public: // ISourceMod
	const char *GetGamePath() const;
	const char *GetSourceModPath() const;
	size_t BuildPath(PathType type, char *buffer, size_t maxlength, const char *format, ...);
	void LogMessage(IExtension *pExt, const char *format, ...);
	void LogError(IExtension *pExt, const char *format, ...);
	size_t FormatString(char *buffer, size_t maxlength, IPluginContext *pContext, const cell_t *params, unsigned int param);
	void *CreateDataPack();
	void FreeDataPack(void *pack);
	HandleType_t GetDataPackHandleType(bool readonly=false);
	KeyValues *ReadKeyValuesHandle(Handle_t hndl, HandleError *err=NULL, bool root=false);
	const char *GetGameFolderName() const;
	ISourcePawnEngine *GetScriptingEngine();
	IVirtualMachine *GetScriptingVM();
	void AllPluginsLoaded();
	time_t GetAdjustedTime();
	void GlobalPause();
	void GlobalUnpause();
	void DoGlobalPluginLoads();
	void AddGameFrameHook(GAME_FRAME_HOOK hook);
	void RemoveGameFrameHook(GAME_FRAME_HOOK hook);
	void ProcessGameFrameHooks(bool simulating);
	size_t Format(char *buffer, size_t maxlength, const char *fmt, ...);
	size_t FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap);
	void AddFrameAction(FRAMEACTION fn, void *data);
	const char *GetCoreConfigValue(const char *key);
	int GetPluginId();
	int GetShApiVersion();
	bool IsMapRunning();
	void *FromPseudoAddress(uint32_t pseudoAddr);
	uint32_t ToPseudoAddress(void *addr);
private:
	void ShutdownServices();
private:
	char m_SMBaseDir[PLATFORM_MAX_PATH];
	char m_SMRelDir[PLATFORM_MAX_PATH];
	char m_ModDir[32];
	bool m_IsMapLoading;
	bool m_ExecPluginReload;
	bool m_ExecOnMapEnd;
	unsigned int m_target;
	bool m_GotBasePath;
	CVector<GAME_FRAME_HOOK> m_frame_hooks;
};

void UTIL_ConsolePrintVa(const char *fmt, va_list ap);
void UTIL_ConsolePrint(const char *fmt, ...);

extern bool g_Loaded;
extern bool sm_show_debug_spew;
extern SourceModBase g_SourceMod;

#endif //_INCLUDE_SOURCEMOD_GLOBALHEADER_H_
