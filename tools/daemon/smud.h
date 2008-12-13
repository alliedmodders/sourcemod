#ifndef _INCLUDE_SMUD_MAIN_H_
#define _INCLUDE_SMUD_MAIN_H_

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define closesocket close

#define NUM_FILES 6

extern char fileNames[NUM_FILES][30];
extern void *fileLocations[NUM_FILES];
extern int fileLength[NUM_FILES];

#endif //_INCLUDE_SMUD_MAIN_H_
