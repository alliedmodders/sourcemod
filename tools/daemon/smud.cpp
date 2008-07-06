#include "smud.h"
#include "smud_threads.h"

#define LISTEN_PORT 6500
#define LISTEN_QUEUE_LENGTH 6

char fileNames[NUM_FILES][30] = {
	"core.games.txt",
	"sdktools.games.txt",
	"sdktools.games.ep2.txt",
	"sm-cstrike.games.txt",
	"sm-tf2.games.txt",
};

void *fileLocations[NUM_FILES];
int fileLength[NUM_FILES];

int main(int argc, char **argv)
{
	ThreadPool *pool;
	struct protoent *pProtocol;
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	int serverSocket;
	int clientSocket;
	int addressLen;
	int opts;
	int file;
	char filename[100];
	struct stat sbuf;

	printf("Loading Gamedata files into memory\n");

	for (int i=0; i<NUM_FILES; i++)
	{
		snprintf(filename, sizeof(filename), "./md5/%s", fileNames[i]);
		file = open(filename, O_RDWR);

		if (!file)
		{
			return 1;
		}

		if (stat(filename, &sbuf) == -1)
		{
			return 1;
		}

		if ((fileLocations[i] = mmap(NULL, sbuf.st_size, PROT_READ, MAP_SHARED, file, 0)) == (caddr_t)(-1))
		{
			return 1;
		}

		fileLength[i] = sbuf.st_size;

		printf("Initialised file of %s of length %i\n", fileNames[i], fileLength[i]);
	}

	printf("Initializing Thread Pool\n");

	pool = new ThreadPool();
	
	if (!pool->Start())
	{
		return 1;
	}

	printf("Create Server Socket\n");

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(LISTEN_PORT);

	pProtocol = getprotobyname("tcp");

	if (pProtocol == NULL)
	{
		return 1;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, pProtocol->p_proto);

	if (serverSocket < 0) 
	{
		return 1;
	}

	opts = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

	if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
	{
		return 1;
	}

	if (listen(serverSocket, LISTEN_QUEUE_LENGTH) < 0) 
	{
		return 1;
	}

	printf("Entering Main Loop\n");

	while (1)
	{
		addressLen = sizeof(clientAddress);

		if ( (clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, (socklen_t *)&addressLen)) < 0) 
		{
			continue;
		}

		opts = fcntl(clientSocket, F_GETFL, 0);
		if (fcntl(clientSocket, F_SETFL, opts|O_NONBLOCK) < 0)
		{
			closesocket(clientSocket);
			continue;
		}


		printf("Connection Received!\n");

		pool->AddConnection(clientSocket);
	}

	delete pool;
}



