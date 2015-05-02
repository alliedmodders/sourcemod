#ifndef _INCLUDED_WEB_EXECUTER_H_
#define _INCLUDED_WEB_EXECUTER_H_
#include "web_callback.h"
#include <IThreader.h>

class WebExecuter :
	public SourceMod::IThread,
	public SourceMod::ITransferHandler
{
private:
	const char* url;
	WebCallback callbackManager;
	IWebTransfer* session;
	IWebForm* form;
public:
	WebExecuter(const char* webUrl, SourceMod::Handle_t handle, SourcePawn::IPluginContext* plugin, funcid_t callback, funcid_t error, cell_t data, SourceMod::Handle_t formHandle = BAD_HANDLE);
	~WebExecuter();
public: //IThread
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);
public: //ITransferHandler
	DownloadWriteStatus OnDownloadWrite(IWebTransfer *session, void *userdata, void *ptr, size_t size, size_t nmemb);
};
#endif//_INCLUDED_WEB_EXECUTER_H_