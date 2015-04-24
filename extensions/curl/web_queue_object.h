#ifndef _INCLUDE_WEB_QUEUE_OBJECT_H_
#define _INCLUDE_WEB_QUEUE_OBJECT_H_

#include "sp_caller.h"
#include "curlapi.h"

class WebHandle
{
private:
	IWebTransfer* session;
public:
	SourceMod::Handle_t webForm = BAD_HANDLE;
public:
	WebHandle(){ session = NULL; }
	WebHandle(IWebTransfer* webSession){ session = webSession; }
	~WebHandle() { delete session; }
	IWebTransfer* GetSession(){ return session; }
};

class WebQueueObject
{
private:
	const char* url;
	SPCaller spBridge;
	cell_t userData;
public:
	WebQueueObject();
	WebQueueObject(const char* url, SPCaller spBridge, cell_t userData);
	void RunDownload();
	DownloadWriteStatus OnDownloadWrite(const char* data);
	void SkipDownload();
};

extern HandleType_t WebSessionType;
extern HandleType_t WebFormType;
#endif