#include "web_callback.h"
#include <smsdk_ext.h>

using namespace SourceMod;
using namespace SourcePawn;

IWebTransfer* WebCallback::GetSession()
{
	HandleSecurity security(pCtx->GetIdentity(), myself->GetIdentity());
	IWebTransfer* ret;
	if (handlesys->ReadHandle(webHandle, WebSessionType, &security, (void**)&ret) != HandleError_None)
		return NULL;
	return ret;
}

DownloadWriteStatus WebCallback::ExecuteCallback(const char* data)
{
	if (!callback || !callback->IsRunnable())
		return DownloadWrite_Okay;
	callback->PushCell(webHandle);
	callback->PushString(data);
	callback->PushCell(userData);
	cell_t result;
	if (callback->Execute(&result) != SP_ERROR_NONE)
		return DownloadWrite_Okay;
	return (DownloadWriteStatus)result;
}

void WebCallback::ExecuteError()
{
	if (!error || !error->IsRunnable())
		return;
	error->PushCell(webHandle);
	error->Execute(NULL);
}