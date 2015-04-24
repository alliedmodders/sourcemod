#include "sp_caller.h"

using namespace SourceMod;
using namespace SourcePawn;

SPCaller::SPCaller()
{
	this->onError = 0;
	this->onDownload = 0;
	this->pCtx = nullptr;
	this->webSession = BAD_HANDLE;
}

SPCaller::SPCaller(funcid_t onError, funcid_t onDownload, SourcePawn::IPluginContext* pCtx, SourceMod::Handle_t webSession)
{
	this->onError = onError;
	this->onDownload = onDownload;
	this->pCtx = pCtx;
	this->webSession = webSession;
}

void SPCaller::Call_OnError()
{
	IPluginFunction* func = pCtx->GetFunctionById(onError);
	if (!func)
		return pCtx->ReportError("Invalid onError function address %x", onError);
	func->PushCell(webSession);
	cell_t disposable;
	if (func->Execute(&disposable) != SP_ERROR_NONE)
		pCtx->ReportError("Calling onError function at %x failed.", onError);
}

DownloadWriteStatus SPCaller::Call_OnDownloadWrite(const char* data, cell_t userData)
{
	IPluginFunction* func = pCtx->GetFunctionById(onDownload);
	if (!func)
	{
		pCtx->ReportError("Invalid onDownload function address %x", onDownload);
		return DownloadWrite_Okay;
	}
	func->PushCell(webSession);
	func->PushString(data);
	func->PushCell(userData);
	cell_t result;
	if (func->Execute(&result) != SP_ERROR_NONE)
	{
		pCtx->ReportError("Calling onDownload function at %x failed.", onDownload);
		return DownloadWrite_Okay;
	}
	return (DownloadWriteStatus)result;
}