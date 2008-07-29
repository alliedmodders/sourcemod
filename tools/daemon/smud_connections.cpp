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

	printf("New Connection Added\n");
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
			con = (smud_connection *)*iter;

			pollReturn = poll(&(con->pollData), 1, 0);

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
				closesocket(con->fd);
				delete con;

				printf("Connection Completed!\n");

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
	char data[11];

	if (recv(con->fd, data, sizeof(data), 0) == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
		}

		return;
	}

	if (data[0] != 'A' || data[1] != 'G')
	{
		con->state = ConnectionState_Complete;
		return;
	}

	//Ignore the next 8 bytes for the moment. Versioning data is currently unused
	// uint16[4] - source version major/minor/something/rev

	con->sentSums = data[10];

	con->state = ConnectionState_ReadQueryData;
	printf("Query Header Read Complete, %i md5's expected\n", con->sentSums);
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

	if (send(con->fd, data, sizeof(data), 0) == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
		}

		return;
	}
	
	//Now we need more send sub functions for all the damn files. Geh.
	//Alternatively we could just send all at once here. Could make for a damn big query. 100k anyone?

	con->state = ConnectionState_SendingFiles;
	printf("Query Reply Header Complete\n");
}

void ConnectionPool::ReadQueryContent( smud_connection *con )
{
	char *data = new char[16*(con->sentSums)]();

	if (recv(con->fd, data, 16*(con->sentSums), 0) == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
		}

		delete [] data;
		return;
	}

	con->shouldSend = new MD5Status[con->sentSums]();
	con->fileLocation = new int[con->sentSums]();
	con->headerSent = new bool[con->sentSums]();

	for (int i=0; i<con->sentSums; i++)
	{
		con->fileLocation[i] = -1;
		con->shouldSend[i] = GetMD5UpdateStatus(data + (16*i), con, i);
		
		if (con->shouldSend[i] == MD5Status_NeedsUpdate)
		{
			printf("File %i needs updating\n", i);
			con->sendCount++;
			con->headerSent[i] = false;
			continue;
		}

		if (con->shouldSend[i] == MD5Status_Unknown)
		{
			printf("File %i is unknown\n", i);
			con->unknownCount++;
		}
	}

	con->state = ConnectionState_ReplyQuery;
	con->pollData.events = POLLOUT;
	delete [] data;
	printf("Query Data Read Complete\n");
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

	printf("checking for file \"%s\"\n", path);
	
	FILE *file = fopen(path, "r");

	if (file == NULL)
	{
		printf("Couldn't find file!\n");
		return MD5Status_Unknown;
	}

	char latestMD5[33];
	fgets(latestMD5, 33, file);
	printf("Latest md5 is: %s\n", latestMD5);

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

	printf("Filename is %s\n", filename);

	//We now need to match this filename with one of our mmap'd files in memory and store it until send gets called.
	for (int i=0; i<NUM_FILES; i++)
	{
		if (strcmp(fileNames[i], filename) == 0)
		{
			con->fileLocation[fileNum] = i;
			printf("File %i mapped to local file %i\n", fileNum, i);
			return MD5Status_NeedsUpdate;
		}
	}

	return MD5Status_Unknown;
}

void ConnectionPool::SendFile( smud_connection *con )
{
	//Find the next file to send.
	while (con->currentFile < con->sentSums &&
			con->shouldSend[con->currentFile] != MD5Status_NeedsUpdate)
	{
		con->currentFile++;
	}

	//All files have been sent.
	if (con->currentFile >= con->sentSums)
	{
		printf("All files sent!\n");
		con->state = ConnectionState_SendUnknownList;
		return;
	}

	void *file = fileLocations[con->fileLocation[con->currentFile]];
	int filelength = fileLength[con->fileLocation[con->currentFile]];

	printf("Sending file of length %i\n", filelength);
	printf("Current file index is: %i, maps to file index: %i\n", con->currentFile, con->fileLocation[con->currentFile]);

	if (!con->headerSent[con->currentFile])
	{
		char buffer[5];
		buffer[0] = con->currentFile;
		*((int *)&buffer[1]) = filelength;

		if (send(con->fd, buffer, 5, 0) == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				con->state = ConnectionState_Complete;
			}

			return;
		}

		con->headerSent[con->currentFile] = true;
	}

	if (send(con->fd, file, filelength, 0) == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
		}

		return;
	}

	con->currentFile++;
	printf("Sent a file!: %s\n", fileNames[con->fileLocation[con->currentFile-1]]);
}

void ConnectionPool::SendUnknownList( smud_connection *con )
{
	int size = con->unknownCount+1;
	char *packet = new char[size]();

	packet[0] = con->unknownCount;

	printf("%i Files are unknown\n", con->unknownCount);

	int i=1;

	for (int j=0; j<con->sentSums; j++)
	{
		if (con->shouldSend[j] == MD5Status_Unknown)
		{
			packet[i] = j;
			i++;
		}
	}

	if (send(con->fd, packet, size, 0) == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			con->state = ConnectionState_Complete;
		}

		return;
	}

	con->state = ConnectionState_Complete;
	printf("Unknown's Sent\n");
}
