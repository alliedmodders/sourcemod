#ifndef _INCLUDE_SMUD_H_
#define _INCLUDE_SMUD_H_

#include "smud.h"
#include "smud_connections.h"

void *ThreadCallback(void *data);

class ThreadWorker
{
public:
	ThreadWorker();
	~ThreadWorker();
public:
	bool Start();
	void CancelAndWait();
	void AddConnection(int fd);
	void Process();
private:
	ConnectionPool *m_pPool;
	pthread_t m_Thread;
	pthread_mutex_t m_NotifyLock;
	pthread_cond_t m_Notify;
	bool m_bShouldCancel;
};

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
public:
	void AddConnection(int fd);
	bool Start();
	void Stop();
private:
	ThreadWorker *m_pWorker;
};

#endif //_INCLUDE_SMUD_H_

