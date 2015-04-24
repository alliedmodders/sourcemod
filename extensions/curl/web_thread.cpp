#include <smsdk_ext.h>
#include "web_thread.h"
#include "web_callback.h"

using namespace SourceMod;

WebCallback* gCallback;
WebThread* gWebThread = new WebThread();
IThreadHandle* thread = NULL;

bool WebThread::InsertIntoQueue(WebQueueObject queueObject)
{
	return queue.append(queueObject);
}

void WebThread::RunThread(IThreadHandle *pHandle)
{
	if (!queue.length())
		return;
	queue[0].RunDownload();
	queue.remove(0);
}

void WebThread::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
	if (!queue.length())
		return;
	while (queue.length() > 0)
	{
		queue[0].SkipDownload();
		queue.remove(0);
	}
}

void StartWebThread()
{
	gCallback = new WebCallback();
	ThreadParams params;
	params.flags = Thread_Default;
	params.prio = ThreadPrio_Normal;
	thread = threader->MakeThread(gWebThread, &params);
}

void StopWebThread()
{
	if (thread != NULL)
	{
		thread->WaitForThread();
		thread->DestroyThis();
		thread = NULL;
	}
}