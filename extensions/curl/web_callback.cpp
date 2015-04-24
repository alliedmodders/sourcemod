#include <string.h>
#include <stdlib.h>
#include "web_callback.h"
#include "web_queue_object.h"

DownloadWriteStatus WebCallback::OnDownloadWrite(IWebTransfer *session,
	void *userdata,
	void *ptr,
	size_t size,
	size_t nmemb)
{
	//All but the WebQueueObject call was taken from SM Gamedata Updater MemoryDownloader.cpp
	char* buffer = nullptr;
	size_t bufsize = 0, bufpos = 0, total = size * nmemb;
	if (bufpos + total > bufsize)
	{
		size_t rem = (bufpos + total) - bufsize;
		bufsize += rem + (rem / 2);
		buffer = (char *)realloc(buffer, bufsize);
	}
	assert(bufpos + total <= bufsize);
	memcpy(&buffer[bufpos], ptr, total);
	bufpos += total;
	DownloadWriteStatus result = (*static_cast<WebQueueObject*>(userdata)).OnDownloadWrite(buffer);
	free(buffer);
	return result;
}