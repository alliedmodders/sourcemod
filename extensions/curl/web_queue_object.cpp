#include <assert.h>
#include <smsdk_ext.h>
#include "web_queue_object.h"
#include "web_callback.h"

using namespace SourceMod;
using namespace SourcePawn;

WebQueueObject::WebQueueObject()
{
	this->url = nullptr;
}

WebQueueObject::WebQueueObject(const char* url, SPCaller spBridge, cell_t userData)
{
	this->url = url;
	this->spBridge = spBridge;
	this->userData = userData;
}

void WebQueueObject::RunDownload()
{
	HandleSecurity sec(spBridge.GetPlugin()->GetIdentity(), myself->GetIdentity());
	WebHandle webHandle;
	if (handlesys->ReadHandle(spBridge.GetSession(), WebSessionType, &sec, (void**)&webHandle) != HandleError_None)
		return spBridge.GetPlugin()->ReportError("Unable to read web session handle %x", spBridge.GetSession());
	if (webHandle.webForm != BAD_HANDLE)
	{
		IWebForm* paramList;
		if (handlesys->ReadHandle(webHandle.webForm, WebFormType, &sec, (void**)&paramList) != HandleError_None)
			goto NoForm;
		if (!webHandle.GetSession()->PostAndDownload(url, paramList, gCallback, this))
			spBridge.Call_OnError();
		assert(handlesys->FreeHandle(webHandle.webForm, &sec) == HandleError_None);
		webHandle.webForm = BAD_HANDLE;
	}
	else
	{
	NoForm:
		if (!webHandle.GetSession()->Download(url, gCallback, this))
			spBridge.Call_OnError();
	}
}

DownloadWriteStatus WebQueueObject::OnDownloadWrite(const char* data)
{
	return spBridge.Call_OnDownloadWrite(data, userData);
}

void WebQueueObject::SkipDownload()
{
	spBridge.Call_OnDownloadWrite("", userData);
}