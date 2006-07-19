#ifndef _INCLUDE_SPFILE_H
#define _INCLUDE_SPFILE_H

#include "sp_file_headers.h"

/** 
 * Used for overwriting writing routines.
 */
typedef struct sp_writefuncs_s
{
	void *(*fnOpen)(const char *);	/* filename, returns handle */
	void (*fnClose)(void *);			/* handle */
	/* buffer, size, count, handle, returns count written */
	size_t (*fnWrite)(const void *, size_t, size_t, void *);	
	/* buffer, size, count, handle, returns count read */
	size_t (*fnRead)(void *, size_t, size_t, void *);
	/* returns current position from start */
	size_t (*fnGetPos)(void *);
	/* sets current position from start, return 0 for success, nonzero for error */
	int (*fnSetPos)(void *, size_t);
} sp_writefuncs_t;

typedef struct sp_file_s
{
	sp_file_hdr_t header;
	sp_file_section_t *sections;
	size_t *offsets;
	sp_writefuncs_t funcs;
	size_t lastsection;
	size_t curoffs;
	void *handle;
	int state;
	char *nametab;
	size_t nametab_idx;
} sp_file_t;

/**
 * Creates a new SourcePawn binary file.  
 * You may optionally specify alternative writing functions.
 */
sp_file_t *spfw_create(const char *name, sp_writefuncs_t *optfuncs);

/**
 * Closes file handle and frees memory.
 */
void spfw_destroy(sp_file_t *spf);

/**
 * Adds a section name to the header. 
 * Only valid BEFORE finalization.
 * Returns the number of sections, or 0 on failure.
 */
uint8_t spfw_add_section(sp_file_t *spf, const char *name);

/**
 * Finalizes the section header.
 * This means no more sections can be added after this call.
 * Also, aligns the writer to the first section.
 * Returns 0 on success, nonzero on error.
 */
int spfw_finalize_header(sp_file_t *spf);

/**
 * Finalizes the current section and advances to the next.
 * In order for this to be accurate, the file pointer must
 *  reside at the end before calling this, because the size
 *  is calculated by differencing with the last known offset.
 * Returns 1 if there are more sections left, 0 otherwise.
 * Returns -1 if the file state is wrong.
 */
int spfw_next_section(sp_file_t *spf);

/**
 * Finalizes all sections.
 * Cannot be called until all sections are used.
 * Must be called with the file pointer at the end.
 * Also does compression!
 */
int spfw_finalize_all(sp_file_t *spf);

#endif //_INCLUDE_SPFILE_H
