#include <assert.h>
#include "smud_connections.h"
#include "smud.h"

ConnectionPool::ConnectionPool()
{
	pthread_mutex_init(&m_AddLock, NULL);
	m_timeOut = 1000;
}

ConnectionPool::~ConnectionPool()
{
	pthread_mutex_destroy(&m_AddLock);
}

void ConnectionPool::AddConnection( int fd )
{
	smud_connection *connection = new smud_connection(fd);

	pthread_mutex_lock(&m_AddLock);
	m_AddQueue.push_back(connection);
	pthread_mutex_unlock(&m_AddLock);
}

void ConnectionPool::Process( bool *terminate )
{
	struct timespec ts_wait;

	ts_wait.tv_sec = 0;
	ts_wait.tv_nsec = 50000000; /* 50ms */

	std::list<smud_connection *>::iterator iter;
	smud_connection *con = NULL;

	while (1)
	{
		if (*terminate)
		{
			return;
		}

		iter = m_Links.begin();

		QueryResult result = QueryResult_Continue;
		int pollReturn = 0;

		/* Add all connections that want processing to the sets */
		while (iter != m_Links.end())
		{
			con = *iter;

			pollReturn = poll(&(con->pollData), 1, 0);
			assert(pollReturn <= 1);

			if (pollReturn == -1)
			{
				//Something went badly wrong or the connection closed.
				result = QueryResult_Complete;
			}
			else if (pollReturn == 1)
			{
				//Poll returns the number of sockets available (which can only ever be 1)
				result = ProcessConnection(con);
			}

			if (result == QueryResult_Complete)
			{
				iter = m_Links.erase(iter);
#if defined DEBUG
				fprintf(stdout, "Closing socket %d\n", con->fd);
#endif
				closesocket(con->fd);
				delete con;
				continue;
			}

			iter++;
		}

		/* Add new items to process */

		iter = m_Links.end();
		pthread_mutex_lock(&m_AddLock);
		m_Links.splice(iter, m_AddQueue);
		pthread_mutex_unlock(&m_AddLock);

		nanosleep(&ts_wait, NULL);
	}
}

QueryResult ConnectionPool::ProcessConnection( smud_connection *con )
{
	switch (con->state)
	{
		case ConnectionState_ReadQueryHeader:
		{
			ReadQueryHeader(con);
			break;
		}

		case ConnectionState_ReadQueryData:
		{
			ReadQueryContent(con);
			break;
		}

		case ConnectionState_ReplyQuery:
		{
			ReplyQuery(con);
			break;
		}

		case ConnectionState_SendingFiles:
		{
			SendFile(con);
			break;
		}

		case ConnectionState_SendUnknownList:
		{
			SendUnknownList(con);
			break;
		}

		case ConnectionState_Complete:
		{
			break;
		}
	}

	if (con->state == ConnectionState_Complete)
	{
		printf("Ending connection because it marked itself as finished\n");
		return QueryResult_Complete;
	}

	if (con->start + m_timeOut < time(NULL))
	{
		printf("Ending connection because it has passed maximum allowed time\n");
		return QueryResult_Complete;
	}

	return QueryResult_Continue;
}

void ConnectionPool::ReadQueryHeader( smud_connection *con )
{
	if (con->buffer == NULL)
	{
		con->buffer = new char[QUERY_HEADER_SIZE];
		con->writtenCount = 0;
	}
	
	int bytesReceived = 0;

	bytesReceived = recv(con->fd, con->buffer+con->writtenCount, QUERY_HEADER_SIZE-con->writtenCount, 0);

	if (bytesReceived == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
			delete [] con->buffer;
			con->buffer = NULL;
			con->writtenCount = 0;
		}

		return;
	}

	con->writtenCount += bytesReceived;

	assert(con->writtenCount <= QUERY_HEADER_SIZE);

	if (con->writtenCount < QUERY_HEADER_SIZE)
	{
		/* Don't change the connection status, so next cycle we will come back to here and continue receiving data */
		return;
	}
	
	if (con->buffer[0] != 'A' || con->buffer[1] != 'G')
	{
		con->state = ConnectionState_Complete;
		delete [] con->buffer;
		con->buffer = NULL;
		con->writtenCount = 0;
		return;
	}

	//Ignore the next 8 bytes for the moment. Versioning data is currently unused
	// uint16[4] - source version major/minor/something/rev

	con->sentSums = con->buffer[10];

	con->state = ConnectionState_ReadQueryData;
#if defined DEBUG
	fprintf(stdout, "Query Header Read Complete, %i md5's expected\n", con->sentSums);
#endif

	delete [] con->buffer;
	con->buffer = NULL;
	con->writtenCount = 0;
}

void ConnectionPool::ReplyQuery(smud_connection *con)
{
	char data[12];
	data[0] = 'A';
	data[1] = 'G';

	data[2] = (char)Update_Unknown; //unused versioning crap
	*(short *)&data[3] = 1;
	*(short *)&data[5] = 0;
	*(short *)&data[7] = 0;
	*(short *)&data[9] = 3;

	data[11] = (char)con->sendCount;

	int bytesSent = send(con->fd, data+con->writtenCount, sizeof(data)-con->writtenCount, 0);
	
	if (bytesSent == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
			con->writtenCount = 0;
		}

		return;
	}

	con->writtenCount += bytesSent;

	assert(con->writtenCount <= 12);

	if (con->writtenCount < 12)
	{
		/** Still more data needs to be sent - Return so we come back here next cycle */
		return;
	}
	
	con->state = ConnectionState_SendingFiles;
	con->writtenCount = 0;
#if defined DEBUG
	printf("Query Reply Header Complete\n");
#endif
}

void ConnectionPool::ReadQueryContent( smud_connection *con )
{
	if (con->buffer == NULL)
	{
		con->buffer = new char[QUERY_CONTENT_SIZE*con->sentSums];
		con->writtenCount = 0;
	}

	int bytesReceived = 0;

	bytesReceived = recv(con->fd, con->buffer+con->writtenCount, (QUERY_CONTENT_SIZE*con->sentSums)-con->writtenCount, 0);

	if (bytesReceived == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
			delete [] con->buffer;
			con->buffer = NULL;
			con->writtenCount = 0;
		}

		return;
	}

	con->writtenCount += bytesReceived;

	assert(con->writtenCount <= (QUERY_CONTENT_SIZE*con->sentSums));

	if (con->writtenCount < (QUERY_CONTENT_SIZE*con->sentSums))
	{
		/* Don't change the connection status, so next cycle we will come back to here and continue receiving data */
		return;
	}

	con->shouldSend = new MD5Status[con->sentSums]();
	con->fileLocation = new int[con->sentSums]();
	con->headerSent = new bool[con->sentSums]();

	for (int i=0; i<con->sentSums; i++)
	{
		con->fileLocation[i] = -1;
		con->shouldSend[i] = GetMD5UpdateStatus(con->buffer + (QUERY_CONTENT_SIZE*i), con, i);
		
		if (con->shouldSend[i] == MD5Status_NeedsUpdate)
		{
#if defined DEBUG
			fprintf(stdout, "File %i needs updating\n", i);
#endif
			con->sendCount++;
			con->headerSent[i] = false;
			continue;
		}

		if (con->shouldSend[i] == MD5Status_Unknown)
		{
#if defined DEBUG
			fprintf(stdout, "File %i is unknown\n", i);
#endif
			con->unknownCount++;
		}
	}

	con->state = ConnectionState_ReplyQuery;
	con->pollData.events = POLLOUT;
	delete [] con->buffer;
	con->buffer = NULL;
	con->writtenCount = 0;
#if defined DEBUG
	fprintf(stdout, "Query Data Read Complete\n");
#endif
}

MD5Status ConnectionPool::GetMD5UpdateStatus( const char *md5 , smud_connection *con, int fileNum)
{
	//Try find a file with this name in some directory.
	char path[100] = "./md5/";
	char temp[4];
	char md5String[33] = "";

	for (int i=0; i<16; i++)
	{
		snprintf(temp, sizeof(temp), "%02x", (unsigned char)md5[i]);
		strcat(md5String, temp);
	}

	strcat(path, md5String);

#if defined DEBUG
	fprintf(stdout, "checking for file \"%s\"\n", path);
#endif
	
	FILE *file = fopen(path, "r");

	if (file == NULL)
	{
		printf("Couldn't find file!\n");
		return MD5Status_Unknown;
	}

	char latestMD5[33];
	fgets(latestMD5, 33, file);
#if defined DEBUG
	fprintf(stdout, "Latest md5 is: %s\n", latestMD5);
#endif

	if (strcmp(latestMD5, md5String) == 0)
	{
		fclose(file);
		return MD5Status_Current;
	}

	char filename[100];
	filename[0] = '\n';

	while (filename[0] == '\n')
	{
		fgets(filename, sizeof(filename), file);
	}

	if (filename[strlen(filename)-1] == '\n')
	{	
		filename[strlen(filename)-1] = '\0';
	}

	fclose(file);

#if defined DEBUG
	fprintf(stdout, "Filename is %s\n", filename);
#endif

	//We now need to match this filename with one of our mmap'd files in memory and store it until send gets called.
	for (int i=0; i<NUM_FILES; i++)
	{
		if (strcmp(fileNames[i], filename) == 0)
		{
			con->fileLocation[fileNum] = i;
#if defined DEBUG
			fprintf(stdout, "File %i mapped to local file %i\n", fileNum, i);
#endif
			return MD5Status_NeedsUpdate;
		}
	}

	return MD5Status_Unknown;
}

void ConnectionPool::SendFile( smud_connection *con )
{
	//Find the next file to send.
	while (con->writtenCount == 0 &&
			con->currentFile < con->sentSums &&
			con->shouldSend[con->currentFile] != MD5Status_NeedsUpdate)
	{
		con->currentFile++;
	}

	//All files have been sent.
	if (con->currentFile >= con->sentSums)
	{
#if defined DEBUG
		fprintf(stdout, "All files sent!\n");
#endif
		con->state = ConnectionState_SendUnknownList;
		con->writtenCount = 0;
		return;
	}

	void *file = fileLocations[con->fileLocation[con->currentFile]];
	int filelength = fileLength[con->fileLocation[con->currentFile]];

#if defined DEBUG
	fprintf(stdout, "Sending file of length %i\n", filelength);
	fprintf(stdout,
			"Current file index is: %i, maps to file index: %i\n",
			con->currentFile,
			con->fileLocation[con->currentFile]);
#endif

	int sentBytes = 0;

	if (!con->headerSent[con->currentFile])
	{
		char buffer[5];
		buffer[0] = con->currentFile;
		*((int *)&buffer[1]) = filelength;

		sentBytes = send(con->fd, buffer+con->writtenCount, 5-con->writtenCount, 0);

		if (sentBytes == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				con->state = ConnectionState_Complete;
				con->writtenCount = 0;
			}

			return;
		}

		con->writtenCount += sentBytes;
	
		assert(con->writtenCount <= 5);

		if (con->writtenCount < 5)
		{
			return;
		}

		con->headerSent[con->currentFile] = true;
		con->writtenCount = 0;
	}

	sentBytes = send(con->fd, (unsigned char *)file+con->writtenCount, filelength-con->writtenCount, 0);

	if (sentBytes == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
			con->writtenCount = 0;
		}

		return;
	}

	con->writtenCount += sentBytes;

	assert(con->writtenCount <= filelength);

	if (con->writtenCount < filelength)
	{
		return;
	}

	con->currentFile++;
	con->writtenCount = 0;
#if defined DEBUG
	fprintf(stdout, "Sent a file!: %s\n", fileNames[con->fileLocation[con->currentFile-1]]);
#endif
}

void ConnectionPool::SendUnknownList( smud_connection *con )
{
	int size = con->unknownCount+1;
	char *packet = new char[size]();

	packet[0] = con->unknownCount;

#if defined DEBUG
	fprintf(stdout, "%i Files are unknown\n", con->unknownCount);
#endif

	int i=1;

	for (int j=0; j<con->sentSums; j++)
	{
		if (con->shouldSend[j] == MD5Status_Unknown)
		{
			packet[i] = j;
			i++;
		}
	}

	int sentBytes = send(con->fd, packet+con->writtenCount, size-con->writtenCount, 0);
	
	if (sentBytes == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
			con->writtenCount = 0;
		}

		return;
	}

	con->writtenCount += sentBytes;

	assert(con->writtenCount <= size);

	if (con->writtenCount < size)
	{
		return;
	}

	con->state = ConnectionState_Complete;
	con->writtenCount = 0;
#if defined DEBUG
	fprintf(stdout, "Unknowns Sent\n");
#endif
}

