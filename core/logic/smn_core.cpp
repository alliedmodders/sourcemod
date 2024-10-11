/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2017 AlliedModders LLC.  All rights reserved.
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

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <iomanip>
#include <sstream>
#include <list>
#include "common_logic.h"
#include "Logger.h"

#include <ISourceMod.h>
#include <ITranslator.h>
#include <DebugReporter.h>
#include <FrameIterator.h>
#include <MemoryPointer.h>

#include <sourcehook.h>
#include <sh_memory.h>

#if defined PLATFORM_WINDOWS
#include <windows.h>
#include "sm_invalidparamhandler.h"
#elif defined PLATFORM_POSIX
#include <limits.h>
#include <unistd.h>
#include <sys/times.h>
#endif
#include <IForwardSys.h>
#include <ILibrarySys.h>
#include <bridge/include/CoreProvider.h>
#include <bridge/include/IScriptManager.h>
#include <bridge/include/IExtensionBridge.h>
#include <sh_vector.h>

using namespace SourceMod;
using namespace SourcePawn;


HandleType_t g_PlIter;
HandleType_t g_FrameIter;
HandleType_t g_MemoryPtr;

IForward *g_OnLogAction = NULL;

ConVar *g_datetime_format = NULL;

class CoreNativeHelpers : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		HandleAccess hacc;
		handlesys->InitAccessDefaults(NULL,  &hacc);
		hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY|HANDLE_RESTRICT_OWNER;

		g_PlIter = handlesys->CreateType("PluginIterator", this, 0, NULL, NULL, g_pCoreIdent, NULL);
		g_FrameIter = handlesys->CreateType("FrameIterator", this, 0, NULL, NULL, g_pCoreIdent, NULL);

		HandleAccess mp_hacc;
		TypeAccess mp_tacc;
		mp_hacc.access[HandleAccess_Read] = 0;
		mp_hacc.access[HandleAccess_Delete] = HANDLE_RESTRICT_OWNER;
		mp_hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY | HANDLE_RESTRICT_OWNER;
		mp_tacc.access[HTypeAccess_Create] = true;
		g_MemoryPtr = handlesys->CreateType("MemoryPointer", this, 0, &mp_tacc, &mp_hacc, g_pCoreIdent, NULL);

		g_OnLogAction = forwardsys->CreateForward("OnLogAction", 
			ET_Hook, 
			5, 
			NULL,
			Param_Cell,
			Param_Cell,
			Param_Cell,
			Param_Cell,
			Param_String);
		
		g_datetime_format = bridge->FindConVar("sm_datetime_format");
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		if (type == g_MemoryPtr)
		{
			((IMemoryPointer *)object)->Delete();
		}
		else if (type == g_FrameIter)
		{
			delete (SafeFrameIterator *) object;
		}
		else if (type == g_PlIter)
		{
			IPluginIterator *iter = (IPluginIterator *)object;
			iter->Release();
		}
	}
	void OnSourceModShutdown()
	{
		forwardsys->ReleaseForward(g_OnLogAction);
		handlesys->RemoveType(g_PlIter, g_pCoreIdent);
		handlesys->RemoveType(g_FrameIter, g_pCoreIdent);
		handlesys->RemoveType(g_MemoryPtr, g_pCoreIdent);
	}
} g_CoreNativeHelpers;

/**
 * @brief Nearly identical to the standard core plugin iterator
 * with one key difference. Next doesn't increment the counter
 * the first time it is ran. This is a hack for the methodmap..
 */
class CMMPluginIterator
	: public IPluginIterator,
	  public IPluginsListener
{
public:
	CMMPluginIterator(const CVector<SMPlugin *> *list)
		: m_hasStarted(false)
	{
		for(auto iter = list->begin(); iter != list->end(); ++iter) {
			m_list.push_back(*iter);
		}
		scripts->FreePluginList(list);

		m_current = m_list.begin();

		scripts->AddPluginsListener(this);
	}

	virtual ~CMMPluginIterator()
	{
		scripts->RemovePluginsListener(this);
	}
	virtual bool MorePlugins() override
	{
		return (m_current != m_list.end());
	}
	virtual IPlugin *GetPlugin() override
	{
		return *m_current;
	}
	virtual void NextPlugin() override
	{
		if(!m_hasStarted)
		{
			m_hasStarted = true;
			return;
		}

		m_current++;
	}
	virtual void Release() override
	{
		delete this;
	}

public:
	virtual void OnPluginDestroyed(IPlugin *plugin) override
	{
		if (*m_current == plugin)
			m_current = m_list.erase(m_current);
		else
			m_list.remove(static_cast<SMPlugin *>(plugin));
	}

private:
	std::list<SMPlugin *> m_list;
	std::list<SMPlugin *>::iterator m_current;
	bool m_hasStarted;
};

void LogAction(Handle_t hndl, int type, int client, int target, const char *message)
{
	if (g_OnLogAction->GetFunctionCount())
	{
		cell_t result = 0;
		g_OnLogAction->PushCell(hndl);
		g_OnLogAction->PushCell(type);
		g_OnLogAction->PushCell(client);
		g_OnLogAction->PushCell(target);
		g_OnLogAction->PushString(message);
		g_OnLogAction->Execute(&result);

		if (result >= (ResultType)Pl_Handled)
		{
			return;
		}
	}

	const char *logtag = "SM";
	if (type == 2)
	{
		HandleError err;
		IPlugin *pPlugin = scripts->FindPluginByHandle(hndl, &err);
		if (pPlugin)
		{
			logtag = pPlugin->GetFilename();
		}
	}

	g_Logger.LogMessage("[%s] %s", logtag, message);
}
 
static cell_t ThrowError(IPluginContext *pContext, const cell_t *params)
{
	char buffer[512];

	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);

	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);
		if (eh.HasException())
			return 0;
	}

	pContext->ReportError("%s", buffer);
	return 0;
}

static cell_t GetTime(IPluginContext *pContext, const cell_t *params)
{
	time_t t = g_pSM->GetAdjustedTime();
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	*(time_t *)addr = t;

	return static_cast<cell_t>(t);
}

static cell_t FormatTime(IPluginContext *pContext, const cell_t *params)
{
	char *format, *buffer;
	pContext->LocalToString(params[1], &buffer);
	pContext->LocalToStringNULL(params[3], &format);

	if (format == NULL)
	{
		format = const_cast<char *>(bridge->GetCvarString(g_datetime_format));
	}

	time_t t;
	size_t written = 0;
	
	// scope for InvalidParameterHandler
	{
#ifdef PLATFORM_WINDOWS
		InvalidParameterHandler p;
#endif
		t = (params[4] == -1) ? g_pSM->GetAdjustedTime() : (time_t)params[4];
		written = strftime(buffer, params[2], format, localtime(&t));
	}

	if (params[2] && format[0] != '\0' && !written)
	{
		pContext->ThrowNativeError("Invalid time format or buffer too small");
		return 0;
	}

	return 1;
}

static int ParseTime(IPluginContext *pContext, const cell_t *params)
{
	char *datetime;
	char *format;
	pContext->LocalToStringNULL(params[1], &datetime);
	pContext->LocalToStringNULL(params[2], &format);

	if (format == NULL)
	{
		format = const_cast<char *>(bridge->GetCvarString(g_datetime_format));
	}
	else if (!format[0])
	{
		return pContext->ThrowNativeError("Time format string cannot be empty.");
	}
	if (!datetime || !datetime[0])
	{
		return pContext->ThrowNativeError("Date/time string cannot be empty.");
	}

	// https://stackoverflow.com/a/33542189
	std::tm t{};
	std::istringstream input(datetime);

	auto previousLocale = input.imbue(std::locale::classic());
	input >> std::get_time(&t, format);
	bool failed = input.fail();
	input.imbue(previousLocale);

	if (failed)
	{
		return pContext->ThrowNativeError("Invalid date/time string or time format.");
	}

#if defined PLATFORM_WINDOWS
	return _mkgmtime(&t);
#elif defined PLATFORM_LINUX || defined PLATFORM_APPLE
	return timegm(&t);
#else
	return pContext->ThrowNativeError("Platform has no implemented UTC conversion for std::tm to std::time_t");
#endif
}

static cell_t GetPluginIterator(IPluginContext *pContext, const cell_t *params)
{
	IPluginIterator *iter = scripts->GetPluginIterator();

	Handle_t hndl = handlesys->CreateHandle(g_PlIter, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);

	if (hndl == BAD_HANDLE)
	{
		iter->Release();
	}

	return hndl;
}

static cell_t MorePlugins(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IPluginIterator *pIter;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	return pIter->MorePlugins() ? 1 : 0;
}

static cell_t ReadPlugin(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IPluginIterator *pIter;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	IPlugin *pPlugin = pIter->GetPlugin();
	if (!pPlugin)
	{
		return BAD_HANDLE;
	}

	pIter->NextPlugin();

	return pPlugin->GetMyHandle();
}

static cell_t PluginIterator_Create(IPluginContext *pContext, const cell_t *params)
{
	IPluginIterator *iter = new CMMPluginIterator(scripts->ListPlugins());

	Handle_t hndl = handlesys->CreateHandle(g_PlIter, iter, pContext->GetIdentity(), g_pCoreIdent, NULL);

	if (hndl == BAD_HANDLE)
	{
		iter->Release();
	}

	return hndl;
}

static cell_t PluginIterator_Next(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleSecurity sec{pContext->GetIdentity(), g_pCoreIdent};
	HandleError err{};
	IPluginIterator *pIter = nullptr;

	if ((err=handlesys->ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	if(!pIter->MorePlugins())
		return 0;

	pIter->NextPlugin();
	return 1;
}

static cell_t PluginIterator_Plugin_get(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleSecurity sec{pContext->GetIdentity(), g_pCoreIdent};
	HandleError err{};
	IPluginIterator *pIter = nullptr;

	if ((err=handlesys->ReadHandle(hndl, g_PlIter, &sec, (void **)&pIter)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	IPlugin *pPlugin = pIter->GetPlugin();
	if (!pPlugin)
	{
		return BAD_HANDLE;
	}

	return pPlugin->GetMyHandle();
}

IPlugin *GetPluginFromHandle(IPluginContext *pContext, Handle_t hndl)
{
	if (hndl == BAD_HANDLE)
	{
		return scripts->FindPluginByContext(pContext->GetContext());
	} else {
		HandleError err;
		IPlugin *pPlugin = scripts->FindPluginByHandle(hndl, &err);
		if (!pPlugin)
		{
			pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
		}
		return pPlugin;
	}
}

static cell_t GetPluginStatus(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	return pPlugin->GetStatus();
}

static cell_t GetPluginFilename(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[2], params[3], pPlugin->GetFilename(), NULL);

	return 1;
}

static cell_t IsPluginDebugging(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	return pPlugin->IsDebugging() ? 1 : 0;
}

/* Local to plugins only */
enum PluginInfo
{
	PlInfo_Name,			/**< Plugin name */
	PlInfo_Author,			/**< Plugin author */
	PlInfo_Description,		/**< Plugin description */
	PlInfo_Version,			/**< Plugin verison */
	PlInfo_URL,				/**< Plugin URL */
};

static cell_t GetPluginInfo(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = GetPluginFromHandle(pContext, params[1]);
	if (!pPlugin)
	{
		return 0;
	}

	const sm_plugininfo_t *info = pPlugin->GetPublicInfo();

	if (!info)
	{
		return 0;
	}

	const char *str = NULL;

	switch ((PluginInfo)params[2])
	{
	case PlInfo_Name:
		{
			str = info->name;
			break;
		}
	case PlInfo_Author:
		{
			str = info->author;
			break;
		}
	case PlInfo_Description:
		{
			str = info->description;
			break;
		}
	case PlInfo_Version:
		{
			str = info->version;
			break;
		}
	case PlInfo_URL:
		{
			str = info->url;
			break;
		}
	}

	if (!str || str[0] == '\0')
	{
		return 0;
	}

	pContext->StringToLocalUTF8(params[3], params[4], str, NULL);

	return 1;
}

static cell_t SetFailState(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	SMPlugin *pPlugin;

	pContext->LocalToString(params[1], &str);
	pPlugin = scripts->FindPluginByContext(pContext->GetContext());

	if (params[0] == 1)
	{
		pPlugin->EvictWithError(Plugin_Failed, "%s", str);

		return pContext->ThrowNativeErrorEx(SP_ERROR_ABORTED, "%s", str);
	}
	else
	{
		char buffer[2048];

		{
			DetectExceptions eh(pContext);
			g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);
			if (eh.HasException()) {
				pPlugin->EvictWithError(Plugin_Failed, "%s", str);
				return 0;
			}
			pPlugin->EvictWithError(Plugin_Failed, "%s", buffer);
			pContext->ReportFatalError("%s", buffer);
			return 0;
		}
	}

	return 0;
}

static cell_t GetSysTickCount(IPluginContext *pContext, const cell_t *params)
{
#if defined PLATFORM_WINDOWS
	return (cell_t)GetTickCount();
#elif defined PLATFORM_POSIX
	tms tm;
	clock_t ticks = times(&tm);
	long ticks_per_sec = sysconf(_SC_CLK_TCK);
	double fticks = (double)ticks / (double)ticks_per_sec;
	fticks *= 1000.0f;
	if (fticks > INT_MAX)
	{
		double r = (int)(fticks / INT_MAX) * (double)INT_MAX;
		fticks -= r;
	}
	return (cell_t)fticks;
#endif
}

static cell_t AutoExecConfig(IPluginContext *pContext, const cell_t *params)
{
	SMPlugin *plugin = scripts->FindPluginByContext(pContext->GetContext());

	char *cfg, *folder;
	pContext->LocalToString(params[2], &cfg);
	pContext->LocalToString(params[3], &folder);

	if (cfg[0] == '\0')
	{
		static char temp_str[255];
		static char temp_file[PLATFORM_MAX_PATH];
		char *ptr;

		libsys->GetFileFromPath(temp_str, sizeof(temp_str), plugin->GetFilename());
		if ((ptr = strstr(temp_str, ".smx")) != NULL)
		{
			*ptr = '\0';
		}

		/* We have the raw filename! */
		g_pSM->Format(temp_file, sizeof(temp_file), "plugin.%s", temp_str);
		cfg = temp_file;
	}

	plugin->AddConfig(params[1] ? true : false, cfg, folder);

	return 1;
}

static cell_t MarkNativeAsOptional(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	uint32_t idx;

	pContext->LocalToString(params[1], &name);
	if (pContext->FindNativeByName(name, &idx) != SP_ERROR_NONE)
	{
		/* Oops! This HAS to silently fail! */
		return 0;
	}

	pContext->GetRuntime()->UpdateNativeBinding(idx, nullptr, SP_NTVFLAG_OPTIONAL, nullptr);
	return 1;
}

static cell_t RegPluginLibrary(IPluginContext *pContext, const cell_t *params)
{
	char *name;
	SMPlugin *pl = scripts->FindPluginByContext(pContext->GetContext());

	pContext->LocalToString(params[1], &name);

	pl->AddLibrary(name);
	return 1;
}

static cell_t LibraryExists(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	if (strcmp(str, "__CanTestFeatures__") == 0)
	{
		return 1;
	}

	if (scripts->LibraryExists(str))
	{
		return 1;
	}

	if (extsys->LibraryExists(str))
	{
		return 1;
	}

	return 0;
}

static cell_t sm_LogAction(IPluginContext *pContext, const cell_t *params)
{
	char buffer[2048];
	g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
	{
		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 3);
		if (eh.HasException())
			return 0;
	}

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

	LogAction(pPlugin->GetMyHandle(), 2, params[1], params[2], buffer);

	return 1;
}

static cell_t LogToFile(IPluginContext *pContext, const cell_t *params)
{
	char *file;
	pContext->LocalToString(params[1], &file);

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	FILE *fp = fopen(path, "at");
	if (!fp)
	{
		return pContext->ThrowNativeError("Could not open file \"%s\"", path);
	}

	char buffer[2048];
	{
		DetectExceptions eh(pContext);
		g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException()) {
			fclose(fp);
			return 0;
		}
	}

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

	g_Logger.LogToOpenFile(fp, "[%s] %s", pPlugin->GetFilename(), buffer);

	fclose(fp);

	return 1;
}

static cell_t LogToFileEx(IPluginContext *pContext, const cell_t *params)
{
	char *file;
	pContext->LocalToString(params[1], &file);

	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "%s", file);

	FILE *fp = fopen(path, "at");
	if (!fp)
	{
		return pContext->ThrowNativeError("Could not open file \"%s\"", path);
	}

	char buffer[2048];
	{
		DetectExceptions eh(pContext);
		g_pSM->SetGlobalTarget(SOURCEMOD_SERVER_LANGUAGE);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 2);
		if (eh.HasException()) {
			fclose(fp);
			return 0;
		}
	}

	g_Logger.LogToOpenFile(fp, "%s", buffer);

	fclose(fp);

	return 1;
}

static cell_t GetExtensionFileStatus(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	pContext->LocalToString(params[1], &str);

	IExtension *pExtension = extsys->FindExtensionByFile(str);
	
	if (!pExtension)
	{
		return -2;
	}
	
	if (!pExtension->IsLoaded())
	{
		return -1;
	}

	char *error;
	pContext->LocalToString(params[2], &error);
	if (!pExtension->IsRunning(error, params[3]))
	{
		return 0;
	}

	return 1;
}

static cell_t FindPluginByNumber(IPluginContext *pContext, const cell_t *params)
{
	IPlugin *pPlugin = scripts->FindPluginByOrder(params[1]);
	
	if (pPlugin == NULL)
	{
		return BAD_HANDLE;
	}

	return pPlugin->GetMyHandle();
}

static cell_t VerifyCoreVersion(IPluginContext *pContext, const cell_t *params)
{
	return 4;
}

static cell_t GetFeatureStatus(IPluginContext *pContext, const cell_t *params)
{
	FeatureType type = (FeatureType)params[1];
	char *name;

	pContext->LocalToString(params[2], &name);

	return sharesys->TestFeature(pContext->GetRuntime(), type, name);
}

static cell_t RequireFeature(IPluginContext *pContext, const cell_t *params)
{
	FeatureType type = (FeatureType)params[1];
	char *name;

	pContext->LocalToString(params[2], &name);

	if (sharesys->TestFeature(pContext->GetRuntime(), type, name) != FeatureStatus_Available)
	{
		char buffer[255];
		char *msg = buffer;
		char default_message[255];
		SMPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

		DetectExceptions eh(pContext);
		g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 3);
		if (eh.HasException())
			buffer[0] = '\0';

		if (buffer[0] == '\0') {
			g_pSM->Format(default_message, sizeof(default_message), "Feature \"%s\" not available", name);
			msg = default_message;
		}
		pPlugin->EvictWithError(Plugin_Error, "%s", msg);

		if (!eh.HasException())
			pContext->ReportFatalError("%s", msg);
		return 0;
	}

	return 1;
}

enum NumberType
{
	NumberType_Int8,
	NumberType_Int16,
	NumberType_Int32
};

//memory addresses below 0x10000 are automatically considered invalid for dereferencing
#define VALID_MINIMUM_MEMORY_ADDRESS 0x10000

static cell_t LoadFromAddress(IPluginContext *pContext, const cell_t *params)
{
#ifdef PLATFORM_X86
	void *addr = reinterpret_cast<void*>(params[1]);
#else
	void *addr = pseudoAddr.FromPseudoAddress(params[1]);
#endif

	if (addr == NULL)
	{
		return pContext->ThrowNativeError("Address cannot be null");
	}
	else if (reinterpret_cast<uintptr_t>(addr) < VALID_MINIMUM_MEMORY_ADDRESS)
	{
		return pContext->ThrowNativeError("Invalid address 0x%x is pointing to reserved memory.", addr);
	}
	NumberType size = static_cast<NumberType>(params[2]);

	switch(size)
	{
	case NumberType_Int8:
		return *reinterpret_cast<uint8_t*>(addr);
	case NumberType_Int16:
		return *reinterpret_cast<uint16_t*>(addr);
	case NumberType_Int32:
		return *reinterpret_cast<uint32_t*>(addr);
	default:
		return pContext->ThrowNativeError("Invalid number types %d", size);
	}
}


static cell_t StoreToAddress(IPluginContext *pContext, const cell_t *params)
{
#ifdef PLATFORM_X86
	void *addr = reinterpret_cast<void*>(params[1]);
#else
	void *addr = pseudoAddr.FromPseudoAddress(params[1]);
#endif

	if (addr == NULL)
	{
		return pContext->ThrowNativeError("Address cannot be null");
	}
	else if (reinterpret_cast<uintptr_t>(addr) < VALID_MINIMUM_MEMORY_ADDRESS)
	{
		return pContext->ThrowNativeError("Invalid address 0x%x is pointing to reserved memory.", addr);
	}
	cell_t data = params[2];

	NumberType size = static_cast<NumberType>(params[3]);

	// new parameter added after SM 1.10; defaults to true for backwards compatibility
	bool updateMemAccess = true;
	if (params[0] >= 4)
	{
		updateMemAccess = params[4];
	}

	switch(size)
	{
	case NumberType_Int8:
		if (updateMemAccess)
		{
			SourceHook::SetMemAccess(addr, sizeof(uint8_t), SH_MEM_READ|SH_MEM_WRITE|SH_MEM_EXEC);
		}
		*reinterpret_cast<uint8_t*>(addr) = data;
		break;
	case NumberType_Int16:
		if (updateMemAccess)
		{
			SourceHook::SetMemAccess(addr, sizeof(uint16_t), SH_MEM_READ|SH_MEM_WRITE|SH_MEM_EXEC);
		}
		*reinterpret_cast<uint16_t*>(addr) = data;
		break;
	case NumberType_Int32:
		if (updateMemAccess)
		{
			SourceHook::SetMemAccess(addr, sizeof(uint32_t), SH_MEM_READ|SH_MEM_WRITE|SH_MEM_EXEC);
		}
		*reinterpret_cast<uint32_t*>(addr) = data;
		break;
	default:
		return pContext->ThrowNativeError("Invalid number types %d", size);
	}

	return 0;
}

static cell_t IsNullVector(IPluginContext *pContext, const cell_t *params)
{
	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	if (!pNullVec)
		return 0;
	
	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	return addr == pNullVec;
}

static cell_t IsNullString(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	if (pContext->LocalToStringNULL(params[1], &str) != SP_ERROR_NONE)
		return 0;

	return str == nullptr;
}

static cell_t MemoryPointer_Create(IPluginContext *pContext, const cell_t *params)
{
	auto ptr = new MemoryPointer(params[1]);

	Handle_t handle = handlesys->CreateHandle(g_MemoryPtr, ptr, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (handle == BAD_HANDLE)
	{
		delete ptr;
		return BAD_HANDLE;
	}
	
	return handle;
}

static cell_t MemoryPointer_Store(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMemoryPointer *ptr;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&ptr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	unsigned int bytesSize = 0;
	switch (params[3])
	{
	case NumberType_Int8:
		bytesSize = sizeof(std::uint8_t);
		break;
	case NumberType_Int16:
		bytesSize = sizeof(std::uint16_t);
		break;
	case NumberType_Int32:
		bytesSize = sizeof(std::uint32_t);
		break;
	default:
		return pContext->ThrowNativeError("Invalid number types %d", params[3]);
	}
	
	ptr->Store(params[2], bytesSize, params[4], params[5] != 0);

	return 0;
}

static cell_t MemoryPointer_Load(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMemoryPointer *ptr;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&ptr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	unsigned int bytesSize = 0;
	switch (params[2])
	{
	case NumberType_Int8:
		bytesSize = sizeof(std::uint8_t);
		break;
	case NumberType_Int16:
		bytesSize = sizeof(std::uint16_t);
		break;
	case NumberType_Int32:
		bytesSize = sizeof(std::uint32_t);
		break;
	default:
		return pContext->ThrowNativeError("Invalid number types %d", params[2]);
	}
	
	return ptr->Load(bytesSize, params[3]);
}

static cell_t MemoryPointer_StoreMemoryPointer(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMemoryPointer *ptr;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&ptr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	hndl = (Handle_t)params[2];
	IMemoryPointer *store;
	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&store)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	ptr->StorePtr(store->Get(), params[3], params[4] != 0);
	return 0;
}

static cell_t MemoryPointer_LoadMemoryPointer(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMemoryPointer *ptr;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&ptr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	auto newPtr = new MemoryPointer(ptr->LoadPtr(params[2]), 0);

	Handle_t newHandle = handlesys->CreateHandle(g_MemoryPtr, newPtr, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (newHandle == BAD_HANDLE)
	{
		delete newPtr;
		return BAD_HANDLE;
	}
	
	return newHandle;
}

static cell_t MemoryPointer_Offset(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	IMemoryPointer *ptr;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_MemoryPtr, &sec, (void **)&ptr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}

	auto newPtr = new MemoryPointer((void*)(((intptr_t)ptr->Get()) + params[2]), 0);
	Handle_t newHandle = handlesys->CreateHandle(g_MemoryPtr, newPtr, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (newHandle == BAD_HANDLE)
	{
		delete newPtr;
		return BAD_HANDLE;
	}
	
	return newHandle;
}

static cell_t FrameIterator_Create(IPluginContext *pContext, const cell_t *params)
{
	IFrameIterator *it = pContext->CreateFrameIterator();
	
	SafeFrameIterator *iterator = new SafeFrameIterator(it);
	
	pContext->DestroyFrameIterator(it);
	
	Handle_t handle = handlesys->CreateHandle(g_FrameIter, iterator, pContext->GetIdentity(), g_pCoreIdent, NULL);
	if (handle == BAD_HANDLE)
	{
		delete iterator;
		return BAD_HANDLE;
	}
	
	return handle;
}

static cell_t FrameIterator_Next(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	SafeFrameIterator *iterator;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_FrameIter, &sec, (void **)&iterator)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	return iterator->Next();
}

static cell_t FrameIterator_Reset(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	SafeFrameIterator *iterator;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_FrameIter, &sec, (void **)&iterator)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	iterator->Reset();
	return 0;
}

static cell_t FrameIterator_LineNumber(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	SafeFrameIterator *iterator;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_FrameIter, &sec, (void **)&iterator)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	int lineNum = iterator->LineNumber();
	if (lineNum < 0)
	{
		return pContext->ThrowNativeError("Iterator out of bounds. Check return value of FrameIterator.Next");
	}
	
	return lineNum;
}

static cell_t FrameIterator_GetFunctionName(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	SafeFrameIterator *iterator;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_FrameIter, &sec, (void **)&iterator)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	const char* functionName = iterator->FunctionName();
	if (!functionName)
	{
		return pContext->ThrowNativeError("Iterator out of bounds. Check return value of FrameIterator.Next");
	}
	
	char* buffer;
	pContext->LocalToString(params[2], &buffer);
	size_t size = static_cast<size_t>(params[3]);
	
	ke::SafeStrcpy(buffer, size, functionName);
	return 0;
}

static cell_t FrameIterator_GetFilePath(IPluginContext *pContext, const cell_t *params)
{
	Handle_t hndl = (Handle_t)params[1];
	HandleError err;
	SafeFrameIterator *iterator;

	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;
	sec.pOwner = pContext->GetIdentity();

	if ((err=handlesys->ReadHandle(hndl, g_FrameIter, &sec, (void **)&iterator)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Could not read Handle %x (error %d)", hndl, err);
	}
	
	const char* filePath = iterator->FilePath();
	if (!filePath)
	{
		return pContext->ThrowNativeError("Iterator out of bounds. Check return value of FrameIterator.Next");
	}

	char* buffer;
	pContext->LocalToString(params[2], &buffer);
	size_t size = static_cast<size_t>(params[3]);
	
	ke::SafeStrcpy(buffer, size, filePath);
	return 0;
}

static cell_t LogStackTrace(IPluginContext *pContext, const cell_t *params)
{
	char buffer[512];
	g_pSM->FormatString(buffer, sizeof(buffer), pContext, params, 1);

	IFrameIterator *it = pContext->CreateFrameIterator();
	std::vector<std::string> arr = g_DbgReporter.GetStackTrace(it);
	pContext->DestroyFrameIterator(it);

	IPlugin *pPlugin = scripts->FindPluginByContext(pContext->GetContext());

	g_Logger.LogError("[SM] Stack trace requested: %s", buffer);
	g_Logger.LogError("[SM] Called from: %s", pPlugin->GetFilename());
	for (size_t i = 0; i < arr.size(); ++i)
	{
		g_Logger.LogError("%s", arr[i].c_str());
	}
	return 0;
}

REGISTER_NATIVES(coreNatives)
{
	{"ThrowError",				ThrowError},
	{"GetTime",					GetTime},
	{"FormatTime",				FormatTime},
	{"ParseTime",				ParseTime},
	{"GetPluginIterator",		GetPluginIterator},
	{"MorePlugins",				MorePlugins},
	{"ReadPlugin", 				ReadPlugin},
	{"GetPluginStatus",			GetPluginStatus},
	{"GetPluginFilename",		GetPluginFilename},
	{"IsPluginDebugging",		IsPluginDebugging},
	{"GetPluginInfo",			GetPluginInfo},
	{"SetFailState",			SetFailState},
	{"GetSysTickCount",			GetSysTickCount},
	{"AutoExecConfig",			AutoExecConfig},
	{"MarkNativeAsOptional",	MarkNativeAsOptional},
	{"RegPluginLibrary",		RegPluginLibrary},
	{"LibraryExists",			LibraryExists},
	{"LogAction",				sm_LogAction},
	{"LogToFile",				LogToFile},
	{"LogToFileEx",				LogToFileEx},
	{"GetExtensionFileStatus",	GetExtensionFileStatus},
	{"FindPluginByNumber",		FindPluginByNumber},
	{"VerifyCoreVersion",		VerifyCoreVersion},
	{"GetFeatureStatus",        GetFeatureStatus},
	{"RequireFeature",          RequireFeature},
	{"LoadFromAddress",         LoadFromAddress},
	{"StoreToAddress",          StoreToAddress},
	{"IsNullVector",			IsNullVector},
	{"IsNullString",			IsNullString},
	{"LogStackTrace",           LogStackTrace},

	{"MemoryPointer.MemoryPointer",				MemoryPointer_Create},
	{"MemoryPointer.Store",						MemoryPointer_Store},
	{"MemoryPointer.Load",						MemoryPointer_Load},
	{"MemoryPointer.StoreMemoryPointer",		MemoryPointer_StoreMemoryPointer},
	{"MemoryPointer.LoadMemoryPointer",			MemoryPointer_LoadMemoryPointer},
	{"MemoryPointer.Offset",					MemoryPointer_Offset},
	
	{"FrameIterator.FrameIterator",				FrameIterator_Create},
	{"FrameIterator.Next",						FrameIterator_Next},
	{"FrameIterator.Reset",						FrameIterator_Reset},
	{"FrameIterator.LineNumber.get",			FrameIterator_LineNumber},
	{"FrameIterator.GetFunctionName",			FrameIterator_GetFunctionName},
	{"FrameIterator.GetFilePath",				FrameIterator_GetFilePath},

	{"PluginIterator.PluginIterator", 			PluginIterator_Create},
	{"PluginIterator.Next", 					PluginIterator_Next},
	{"PluginIterator.Plugin.get", 				PluginIterator_Plugin_get},
	{NULL,						NULL},
};
