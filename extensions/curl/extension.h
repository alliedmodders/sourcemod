/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Sample extension code header.
 */

#include "smsdk_ext.h"
#include "IWebternet.h"
#include "IBaseDownloader.h"
#include <amtl/am-linkedlist.h>
#include <amtl/am-vector.h>
#include <amtl/am-thread-utils.h>
#include <string.h>


/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class CurlExt : public SDKExtension, IPluginsListener
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlength, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	//virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	//virtual bool QueryRunning(char *error, size_t maxlength);
	const char *GetExtensionVerString();
	const char *GetExtensionDateString();

	virtual void OnPluginUnloaded(IPlugin *plugin);

public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlength, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength);
#endif
};

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
size_t UTIL_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap);

// Handle helper class
class HTTPHandleDispatcher : public IHandleTypeDispatch
{
public:
	virtual void OnHandleDestroy(HandleType_t type, void *object);
};

extern HTTPHandleDispatcher g_HTTPHandler;

struct HTTPRequestCompletedContextFunction {
	funcid_t uPluginFunction;
	bool bHasContext;
};

union HTTPRequestCompletedContextPack {
	uint64_t ulContextValue;
	struct {
		HTTPRequestCompletedContextFunction *pCallbackFunction;
		cell_t iPluginContextValue;
	};
};

struct HTTPRequestHandleSet
{
	Handle_t hndlSession;
	Handle_t hndlDownloader;
	Handle_t hndlForm;
};

class HTTPSessionManager
{
public:
	static HTTPSessionManager& instance()
	{
		static HTTPSessionManager _instance;
		return _instance;
	}
	~HTTPSessionManager() {}

	void Initialize();
	void Shutdown();
	void PluginUnloaded(IPlugin *plugin);
	void RunFrame();
	void BurnSessionHandle(IPluginContext * pCtx, HTTPRequestHandleSet &handles);
	void PostAndDownload(IPluginContext *pCtx, 
		HTTPRequestHandleSet handles,
		const char *url,
		HTTPRequestCompletedContextPack contextPack);
	void Download(IPluginContext *pCtx, 
		HTTPRequestHandleSet handles,
		const char *url,
		HTTPRequestCompletedContextPack contextPack);
protected:
	
private:
	HTTPSessionManager() {}
	HTTPSessionManager(const HTTPSessionManager&);
	HTTPSessionManager & operator = (const HTTPSessionManager &);

	enum HTTPRequestMethod
	{
		HTTP_GET,
		HTTP_POST
	};

	struct HTTPRequest 
	{
		Handle_t plugin;
		HTTPRequestMethod method;
		HTTPRequestHandleSet handles;
		const char *url;
		HTTPRequestCompletedContextPack contextPack;
		cell_t result;

		bool operator==(const HTTPRequest& lhs) const
		{
			return !memcmp(&lhs, this, sizeof(HTTPRequest));
		}
	};

	void RemoveFinishedThreads();
	void AddCallback(HTTPRequest request);

	static const unsigned int iMaxRequestsPerFrame = 20;
	IThreadWorker *m_pWorker;
	ke::ConditionVariable threads_;
	ke::LinkedList<IThreadHandle*> threads;
	ke::ConditionVariable callbacks_;
	ke::Vector<HTTPRequest> callbacks;

	class HTTPAsyncRequestHandler : public IThread
	{
	public:
		HTTPAsyncRequestHandler(HTTPRequest request)
		{
			this->request = request;
		}
		~HTTPAsyncRequestHandler() {}
	protected:
	private:
		HTTPRequest request;
		virtual void RunThread(IThreadHandle *pHandle);
		virtual void OnTerminate(IThreadHandle *pHandle, bool cancel)
		{
			delete this;
		}
	};
};

void OnGameFrame(bool simulating);
IPlugin *FindPluginByContext(IPluginContext *pContext);

// Natives
extern const sp_nativeinfo_t curlext_natives[];

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
