#ifndef _INCLUDE_WEB_CALLBACK_H_
#define _INCLUDE_WEB_CALLBACK_H_

#include "curlapi.h"

class WebCallback : public ITransferHandler
{
public:
	DownloadWriteStatus OnDownloadWrite(IWebTransfer *session,
		void *userdata,
		void *ptr,
		size_t size,
		size_t nmemb);
};

extern WebCallback* gCallback;
#endif