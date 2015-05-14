#include "web_executer.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <smsdk_ext.h>

using namespace SourceMod;
using namespace SourcePawn;

WebExecuter::WebExecuter(const char* webUrl, SourceMod::Handle_t handle, SourcePawn::IPluginContext* plugin, funcid_t callback, funcid_t error, cell_t data, SourceMod::Handle_t formHandle) : url(webUrl), callbackManager(handle, plugin, callback, error, data), session(callbackManager.GetSession())
{
	if (formHandle != BAD_HANDLE)
	{
		HandleSecurity security(callbackManager.pCtx->GetIdentity(), myself->GetIdentity());
		if (handlesys->ReadHandle(formHandle, WebFormType, &security, (void**)&form) == HandleError_None)
			handlesys->FreeHandle(formHandle, &security);
		else
			form = NULL;
	}
	else
		form = NULL;
}

void WebExecuter::RunThread(IThreadHandle *pHandle)
{
	if (!form)
	{
		if (!session->Download(url, this, NULL))
			callbackManager.ExecuteError();
	}
	else
	{
		if (!session->PostAndDownload(url, form, this, NULL))
			callbackManager.ExecuteError();
		delete form;
		form = NULL;
	}
}

WebExecuter::~WebExecuter()
{
	OnTerminate(NULL, false);
}

void WebExecuter::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	if (form)
	{
		delete form;
		form = NULL;
	}
}

DownloadWriteStatus WebExecuter::OnDownloadWrite(IWebTransfer *session, void *userdata, void *ptr, size_t size, size_t nmemb)
{
	smutils->LogMessage(myself, "Download start!");
	//The following block of code excluding the call to ExecuteCallback was copied from SM's GameData Updater Extension's MemoryDownloader.cpp
	size_t bufpos = 0, bufsize = 0, total = size * nmemb;
	char* buffer = NULL;
	if (bufpos + total > bufsize)
	{
		size_t rem = (bufpos + total) - bufsize;
		bufsize += rem + (rem / 2);
		buffer = (char *)realloc(buffer, bufsize);
	}
	assert(bufpos + total <= bufsize);
	memcpy(&buffer[bufpos], ptr, total);
	bufpos += total;
	DownloadWriteStatus result = callbackManager.ExecuteCallback(buffer);
	free(buffer);
	smutils->LogMessage(myself, "Download end!");
	return result;
}