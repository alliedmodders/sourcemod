#ifndef _INCLUDE_SMUD_CONNECTION_H_
#define _INCLUDE_SMUD_CONNECTION_H_

#include "smud.h"
#include <list>
#include "poll.h"

#define QUERY_HEADER_SIZE 11
#define QUERY_CONTENT_SIZE 16

enum ConnectionState
{
	ConnectionState_ReadQueryHeader,
	ConnectionState_ReadQueryData,
	ConnectionState_ReplyQuery,
	ConnectionState_SendingFiles,
	ConnectionState_SendUnknownList,
	ConnectionState_Complete,
};

enum QueryResult
{
	QueryResult_Continue,
	QueryResult_Complete,
};

enum MD5Status
{
	MD5Status_Unknown,
	MD5Status_Current,
	MD5Status_NeedsUpdate,
};

enum UpdateStatus
{ 
	Update_Unknown = 0,				/* Version wasn't recognised or version querying is unsupported */
	Update_Current = 1,				/* Server is running latest version */
	Update_NewBuild = 2,			/* Server is on a svn release and a newer version is available */
	Update_MinorAvailable = 3,		/* Server is on a release and a minor release has superceeded it */
	Update_MajorAvailable = 4,		/* Server is on a release and a major release has superceeded it */
	Update_CriticalAvailable = 5,	/* A critical update has been released (security fixes etc) */
};

struct smud_connection
{
	smud_connection(int fd)
	{
		shouldSend = NULL;
		fileLocation = NULL;
		headerSent = NULL;
		sentSums = 0;
		sendCount = 0;
		currentFile = 0;
		unknownCount = 0;
		pollData.events = POLLIN;
		start = time(NULL);
		state = ConnectionState_ReadQueryHeader;
		this->fd = fd;
		pollData.fd = fd;
		buffer = NULL;
		writtenCount = 0;
	}

	~smud_connection()
	{
		if (shouldSend != NULL)
			delete [] shouldSend;
		if (fileLocation != NULL)
			delete [] fileLocation;
		if (headerSent != NULL)
			delete [] headerSent;
	}

	int fd;							/** Socket file descriptor */
	time_t start;					/** The time this connection was received (for timeouts) */
	ConnectionState state;			/** How far through processing the connection we are */
	uint8_t sentSums;				/** Number of MD5 Sums sent from the client */
	MD5Status *shouldSend;			/** Arrays of statuses for each sum */
	int *fileLocation;				/** Array of indexes into the global file list for each sum (only valid if shouldSend[i] == MD5Status_NeedsUpdate) */
	bool *headerSent;				/** Has the header been sent yet for each sum? Header == file index and size */
	int sendCount;					/** Number of files that need to be sent */
	int unknownCount;				/** Number of files that were unknown */
	int currentFile;				/** Current file being sent (index into the above 3 arrays) */
	pollfd pollData;				/** Data to be passed into poll() */
	char *buffer;					/** Temporary storage buffer to hold data until all of it is available */
	int writtenCount;				/** Number of bytes written into the storage buffer */
};



class ConnectionPool
{
public:
	ConnectionPool();
	~ConnectionPool();
public:
	void AddConnection(int fd);
	void Process(bool *terminate);
private:
	QueryResult ProcessConnection(smud_connection *con);
	void ReadQueryHeader(smud_connection *con);
	void ReadQueryContent(smud_connection *con);
	void ReplyQuery(smud_connection *con);
	void SendFile(smud_connection *con);
	void SendUnknownList(smud_connection *con);

	MD5Status GetMD5UpdateStatus(const char *md5, smud_connection *con, int fileNum);
private:
	std::list<smud_connection *> m_Links;
	std::list<smud_connection *> m_AddQueue;
	pthread_mutex_t m_AddLock;
	time_t m_timeOut;
};

#endif //_INCLUDE_SMUD_CONNECTION_H_

