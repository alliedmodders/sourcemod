/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Webternet Extension
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

#include <sourcemod_version.h>
#include "extension.h"
#include <stdarg.h>
#include <sm_platform.h>
#include <curl/curl.h>
#include "curlapi.h"
#include "FileDownloader.h"
#include "MemoryDownloader.h"


/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CurlExt curl_ext;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&curl_ext);

// Natives
extern sp_nativeinfo_t curlext_natives[];

HandleType_t g_SessionHandle = 0;
HandleType_t g_FormHandle = 0;
HandleType_t g_DownloadHandle = 0;
HTTPHandleDispatcher g_HTTPHandler;
HTTPSessionManager& g_SessionManager = HTTPSessionManager::instance();

bool CurlExt::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	long flags;
	CURLcode code;

	flags = CURL_GLOBAL_NOTHING;
#if defined PLATFORM_WINDOWS
	flags = CURL_GLOBAL_WIN32;
#endif

	code = curl_global_init(flags);
	if (code)
	{
		smutils->Format(error, maxlength, "%s", curl_easy_strerror(code));
		return false;
	}

	if (!sharesys->AddInterface(myself, &g_webternet))
	{
		smutils->Format(error, maxlength, "Could not add IWebternet interface");
		return false;
	}

	// Register natives
	g_pShareSys->AddNatives(myself, curlext_natives);
	// Register session handle handler
	HandleAccess hacc;
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	g_SessionHandle = g_pHandleSys->CreateType("HTTPSession", 
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register web form handle handler
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;
	g_FormHandle = g_pHandleSys->CreateType("HTTPWebForm",
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register download handle handler
	g_pHandleSys->InitAccessDefaults(NULL, &hacc);
	hacc.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;
	g_DownloadHandle = g_pHandleSys->CreateType("HTTPDownloader",
		&g_HTTPHandler, 0, 0, &hacc, myself->GetIdentity(), NULL);

	// Register game listeners
	plsys->AddPluginsListener(this);
	g_SessionManager.Initialize();
	smutils->AddGameFrameHook(&OnGameFrame);
	
	return true;
}

void CurlExt::SDK_OnUnload()
{
	g_pHandleSys->RemoveType(g_SessionHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_FormHandle, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_DownloadHandle, myself->GetIdentity());
	plsys->RemovePluginsListener(this);
	smutils->RemoveGameFrameHook(&OnGameFrame);
	g_SessionManager.Shutdown();
	curl_global_cleanup();
}

const char *CurlExt::GetExtensionVerString()
{
	return SOURCEMOD_VERSION;
}

const char *CurlExt::GetExtensionDateString()
{
	return SOURCEMOD_BUILD_TIME;
}

void OnGameFrame(bool simulating)
{
	g_SessionManager.RunFrame();
}

void CurlExt::OnPluginUnloaded(IPlugin *plugin)
{
	g_SessionManager.PluginUnloaded(plugin);
}

void HTTPSessionManager::PluginUnloaded(IPlugin *plugin)
{
	// Wait for running requests to finish
	/* NOTE: when a plugin gets unloaded all associated handles get
	 * automatically destroyed, so running threads may access invalid
	 * memory and crash the host process. This blocking "solution" may
	 * have to be re factored to a better solution.
	 */
	{
		ke::AutoLock lock(&threads_);

		if (!threads.empty())
		{
			for (ke::LinkedList<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
			{
				if ((*i) != NULL)
				{
					(*i)->WaitForThread();
					(*i)->DestroyThis();
					i = this->threads.erase(i);
				}
			}
		}
	}
}

void HTTPSessionManager::PostAndDownload(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContext &reqCtx)
{
	HTTPRequest request = {};
	BurnSessionHandle(pCtx, handles);

	request.plugin = FindPluginByContext(pCtx)->GetMyHandle();
	request.handles = handles;
	request.method = HTTP_POST;
	request.url = url;
	request.context = reqCtx;

	m_pWorker->MakeThread(new HTTPAsyncRequestHandler(request), Thread_AutoRelease);
}

void HTTPSessionManager::Download(IPluginContext *pCtx, 
	HTTPRequestHandleSet handles, const char *url, 
	HTTPRequestCompletedContext &reqCtx)
{
	HTTPRequest request = {};
	BurnSessionHandle(pCtx, handles);

	request.plugin = FindPluginByContext(pCtx)->GetMyHandle();
	request.handles = handles;
	request.method = HTTP_GET;
	request.url = url;
	request.context = reqCtx;

	m_pWorker->MakeThread(new HTTPAsyncRequestHandler(request), Thread_AutoRelease);
}

void HTTPSessionManager::BurnSessionHandle(IPluginContext *pCtx, 
	HTTPRequestHandleSet &handles)
{
	HandleSecurity sec;
	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	// TODO: maybe better way to do this?
	// Make session handle inaccessible to the user
	Handle_t hndlNew = g_pHandleSys->FastCloneHandle(handles.hndlSession);
	if (g_pHandleSys->FreeHandle(handles.hndlSession, &sec) != 
		HandleError_None)
	{
		pCtx->ThrowNativeError("Couldn't free HTTP session handle");
		return;
	}

	handles.hndlSession = hndlNew;
}

void HTTPSessionManager::RunFrame()
{
	// Try to execute pending callbacks
	if (callbacks_.DoTryLock())
	{
		if (!callbacks.empty())
		{
			HTTPRequest request = this->callbacks[0];
			HandleError herr;
			IPlugin *parent = plsys->PluginFromHandle(request.plugin, &herr);

			// Is the requesting plugin still alive?
			if (parent != NULL && herr != HandleError_Freed)
			{
				IPluginContext *pCtx = parent->GetBaseContext();
				funcid_t id = request.context.GetPluginFunction();
				IPluginFunction *pFunction = pCtx->GetFunctionById(id);

				if (pFunction != NULL)
				{
					// Push data and execute callback
					pFunction->PushCell(request.handles.hndlSession);
					pFunction->PushCell(request.result);
					pFunction->PushCell(request.handles.hndlDownloader);
					if (request.context.HasContextValue())
					{
						pFunction->PushCell(request.context.GetContextValue());
					}
					pFunction->Execute(NULL);
				}
			}

			callbacks.remove(0);
		}

		callbacks_.DoUnlock();
	}

	// Do some quick "garbage collection" on finished threads
	this->RemoveFinishedThreads();
}

void HTTPSessionManager::Shutdown()
{
	// Block until all running threads have finished
	this->RemoveFinishedThreads();

	// Destroy all remaining callback calls
	{
		ke::AutoLock lock(&callbacks_);
		this->callbacks.clear();
	}

	threader->DestroyWorker(m_pWorker);
}

void HTTPSessionManager::AddCallback(HTTPRequest request)
{
	ke::AutoLock lock(&callbacks_);
	this->callbacks.append(request);
}

void HTTPSessionManager::RemoveFinishedThreads()
{
	if (threads_.DoTryLock())
	{
		if (!this->threads.empty())
		{
			for (ke::LinkedList<IThreadHandle*>::iterator i(threads.begin()), end(threads.end()); i != end; ++i)
			{
				if ((*i) != NULL)
				{
					if ((*i)->GetState() == Thread_Done)
					{
						(*i)->DestroyThis();
						i = this->threads.erase(i);
					}
				}
			}
		}

		threads_.DoUnlock();
	}
}

void HTTPSessionManager::Initialize()
{
	m_pWorker = threader->MakeWorker(NULL, true);
	m_pWorker->Start();
}

void HTTPSessionManager::HTTPAsyncRequestHandler::RunThread(IThreadHandle *pHandle)
{
	HandleError herr;
	IPlugin *parent = plsys->PluginFromHandle(request.plugin, &herr);

	if (parent == NULL || herr == HandleError_Freed)
	{
		// The parent plugin got unloaded, no more work to do
		return;
	}

	HandleSecurity sec;
	sec.pOwner = parent->GetIdentity();
	sec.pIdentity = myself->GetIdentity();

	IWebTransfer *xfer = NULL;
	g_pHandleSys->ReadHandle(this->request.handles.hndlSession,
		g_SessionHandle, &sec, (void **)&xfer);

	IBaseDownloader *downldr = NULL;
	g_pHandleSys->ReadHandle(this->request.handles.hndlDownloader,
		g_DownloadHandle, &sec, (void **)&downldr);

	switch (this->request.method)
	{
	case HTTP_GET:
		this->request.result = 
			xfer->Download(this->request.url, downldr, NULL);
		break;
	case HTTP_POST:
		IWebForm *form = NULL;
		g_pHandleSys->ReadHandle(this->request.handles.hndlForm,
			g_FormHandle, &sec, (void **)&form);

		this->request.result =
			xfer->PostAndDownload(this->request.url, form, downldr, NULL);
		break;
	}

	g_SessionManager.AddCallback(request);
}

void HTTPHandleDispatcher::OnHandleDestroy(HandleType_t type, void *object)
{
	if (type == g_SessionHandle)
	{
		delete ((IWebTransfer *)object);
	}
	else if (type == g_DownloadHandle)
	{
		delete ((IBaseDownloader *)object);
	}
	else if (type == g_FormHandle)
	{
		delete ((IWebForm *)object);
	}
}

IPlugin *FindPluginByContext(IPluginContext *pContext)
{
	IPlugin *pFoundPlugin;

	IPluginIterator *pPluginIterator = plsys->GetPluginIterator();
	while (pPluginIterator->MorePlugins())
	{
		IPlugin *pPlugin = pPluginIterator->GetPlugin();

		if (pPlugin->GetBaseContext() == pContext)
		{
			pFoundPlugin = pPlugin;
			break;
		}

		pPluginIterator->NextPlugin();
	}
	pPluginIterator->Release();

	return pFoundPlugin;
}
