#ifndef _INCLUDE_MEMFILE_H
#define _INCLUDE_MEMFILE_H

#include <malloc.h>

typedef struct memfile_s
{
	char *name;
	char *base;
	long offs;
	long usedoffs;
	size_t size;
	int _static;
} memfile_t;

memfile_t *memfile_creat(const char *name, size_t init);
void memfile_destroy(memfile_t *mf);
void memfile_seek(memfile_t *mf, long seek);
int memfile_write(memfile_t *mf, void *buffer, size_t size);
size_t memfile_read(memfile_t *mf, void *buffer, size_t maxsize);
long memfile_tell(memfile_t *mf);

#endif //_INCLUDE_MEMFILE_H
