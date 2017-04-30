#ifndef _INCLUDE_WEB_THREAD_H_
#define _INCLUDE_WEB_THREAD_H_

#include "web_queue_object.h"
#include <IThreader.h>
#include <am-vector.h>

class WebThread : public SourceMod::IThread
{
private:
	ke::Vector<WebQueueObject> queue;
public:
	bool InsertIntoQueue(WebQueueObject queueObject);
public:
	void RunThread(IThreadHandle *pHandle);
	void OnTerminate(IThreadHandle *pHandle, bool cancel);
};

extern WebThread* gWebThread;

void StartWebThread();
void StopWebThread();
#endif