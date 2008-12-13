#include <errno.h>
#include "smud.h"
#include "smud_threads.h"

#define LISTEN_PORT 6500
#define LISTEN_QUEUE_LENGTH 6

char fileNames[NUM_FILES][30] = {
	"core.games.txt",
	"sdktools.games.txt",
	"sdktools.games.ep2.txt",
	"sdktools.games.l4d.txt",
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

#if defined DEBUG
	fprintf(stdout, "Loading Gamedata files into memory\n");
#endif

	for (int i=0; i<NUM_FILES; i++)
	{
		snprintf(filename, sizeof(filename), "./md5/%s", fileNames[i]);
		file = open(filename, O_RDWR);

		if (!file)
		{
			fprintf(stderr, "Could not find file: %s", filename);
			return 1;
		}

		if (stat(filename, &sbuf) == -1)
		{
			fprintf(stderr, "Could not stat file: %s (error: %s)", filename, strerror(errno));
			return 1;
		}

		if ((fileLocations[i] = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, file, 0)) == MAP_FAILED)
		{
			fprintf(stderr, "Could not mmap file: %s (error: %s)", filename, strerror(errno));
			return 1;
		}

		fileLength[i] = sbuf.st_size;

#if defined DEBUG
		fprintf(stdout, "Initialised file of %s of length %i\n", fileNames[i], fileLength[i]);
#endif
	}

#if defined DEBUG
	fprintf(stdout, "Initializing Thread Pool\n");
#endif

	pool = new ThreadPool();
	
	if (!pool->Start())
	{
		fprintf(stderr, "Could not initialize thread pool!\n");
		return 1;
	}

#if defined DEBUG
	printf("Create Server Socket\n");
#endif

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(LISTEN_PORT);

	pProtocol = getprotobyname("tcp");

	if (pProtocol == NULL)
	{
		fprintf(stderr, "Could not get tcp proto: %s", strerror(errno));
		return 1;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, pProtocol->p_proto);

	if (serverSocket < 0) 
	{
		fprintf(stderr, "Could not open socket: %s", strerror(errno));
		return 1;
	}

	opts = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

	if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
	{
		fprintf(stderr, "Could not bind socket: %s", strerror(errno));
		return 1;
	}

	if (listen(serverSocket, LISTEN_QUEUE_LENGTH) < 0) 
	{
		fprintf(stderr, "Could not listen on socket: %s", strerror(errno));
		return 1;
	}

	fprintf(stdout, "Server has started.\n");

	while (1)
	{
		addressLen = sizeof(clientAddress);

		clientSocket = accept(serverSocket,
							  (struct sockaddr *)&clientAddress,
							  (socklen_t *)&addressLen);
		if (clientSocket < 0)
		{
			fprintf(stderr, "Could not accept client: %s", strerror(errno));
			continue;
		}

		opts = fcntl(clientSocket, F_GETFL, 0);
		if (fcntl(clientSocket, F_SETFL, opts|O_NONBLOCK) < 0)
		{
			fprintf(stderr, "Could not non-block client: %s", strerror(errno));
			closesocket(clientSocket);
			continue;
		}

#if defined DEBUG
		fprintf(stdout,
				"Accepting connection from client (sock %d, ip %s)",
				clientSocket,
				inet_ntoa(clientAddress.sin_addr));
#endif

		pool->AddConnection(clientSocket);
	}

	delete pool;
}



