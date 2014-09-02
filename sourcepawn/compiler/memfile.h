#ifndef _INCLUDE_MEMFILE_H
#define _INCLUDE_MEMFILE_H

#include <stdlib.h>

typedef struct memfile_s
{
	char *name;
	char *base;
	long offs;
	long usedoffs;
	size_t size;
	int _static;
} memfile_t;

/**
 * Creates a new memory file
 * init is the initial size in bytes
 */
memfile_t *memfile_creat(const char *name, size_t init);

/**
 * Frees the memory associated.
 */
void memfile_destroy(memfile_t *mf);

/**
 * Seeks to a given offset (always from start)
 */
void memfile_seek(memfile_t *mf, long seek);

/**
 * Writes to a memory buffer (expands as necessary).
 * Returns 1 on success, 0 on failure.
 */
int memfile_write(memfile_t *mf, const void *buffer, size_t size);

/**
 * Reads a number of bytes from a memory buffer.
 * Returns the number of bytes read until the end was hit.
 */
size_t memfile_read(memfile_t *mf, void *buffer, size_t maxsize);

/**
 * Returns the current position from the start.
 */
long memfile_tell(memfile_t *mf);

/**
 * Resets all the states of the memory buffer.
 * (does not actually free or zero memory)
 */
void memfile_reset(memfile_t *mf);

typedef memfile_t MEMFILE;
MEMFILE *mfcreate(const char *filename);
void mfclose(MEMFILE *mf);
int mfdump(MEMFILE *mf);
long mflength(const MEMFILE *mf);
long mfseek(MEMFILE *mf,long offset,int whence);
unsigned int mfwrite(MEMFILE *mf,const unsigned char *buffer,unsigned int size);
unsigned int mfread(MEMFILE *mf,unsigned char *buffer,unsigned int size);
char *mfgets(MEMFILE *mf,char *string,unsigned int size);
int mfputs(MEMFILE *mf,const char *string);

#endif //_INCLUDE_MEMFILE_H
