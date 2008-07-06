#include "smud_threads.h"

ThreadPool::ThreadPool() 
{
}

ThreadPool::~ThreadPool()
{
}

bool ThreadPool::Start()
{
	m_pWorker = new ThreadWorker();
	
	if (!m_pWorker->Start())
	{
		delete m_pWorker;
		return false;
	}

	return true;
}

void ThreadPool::Stop()
{
	m_pWorker->CancelAndWait();
	delete m_pWorker;
}

void ThreadPool::AddConnection(int fd)
{
	m_pWorker->AddConnection(fd);
}

ThreadWorker::ThreadWorker() : m_bShouldCancel(false)
{
}

ThreadWorker::~ThreadWorker()
{
}

bool ThreadWorker::Start()
{
	m_pPool = new ConnectionPool();
	pthread_mutex_init(&m_NotifyLock, NULL);
	pthread_cond_init(&m_Notify, NULL);

	if (pthread_create(&m_Thread, NULL, ThreadCallback, this) != 0)
	{
		return false;
	}

	return true;
}

void ThreadWorker::CancelAndWait()
{
	m_bShouldCancel = true;

	pthread_join(m_Thread, NULL);

	pthread_cond_destroy(&m_Notify);
	pthread_mutex_destroy(&m_NotifyLock);

	delete m_pPool;
}

void ThreadWorker::AddConnection( int fd )
{
	m_pPool->AddConnection(fd);
}

void ThreadWorker::Process()
{
	m_pPool->Process(&m_bShouldCancel);
}

void *ThreadCallback(void *data)
{
	((ThreadWorker *)data)->Process();

	pthread_exit(NULL);
}


