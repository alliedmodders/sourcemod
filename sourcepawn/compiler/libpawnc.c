/*	LIBPAWNC.C
 *
 *	A "glue file" for building the Pawn compiler as a DLL or shared library.
 *
 *	Copyright (c) ITB CompuPhase, 2000-2006
 *
 *	This software is provided "as-is", without any express or implied warranty.
 *	In no event will the authors be held liable for any damages arising from
 *	the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	1.	The origin of this software must not be misrepresented; you must not
 *			claim that you wrote the original software. If you use this software in
 *			a product, an acknowledgment in the product documentation would be
 *			appreciated but is not required.
 *	2.	Altered source versions must be plainly marked as such, and must not be
 *			misrepresented as being the original software.
 *	3.	This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id$
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sc.h"
#include "memfile.h"

#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__ || defined DARWIN
#include <sys/types.h>
#include <sys/stat.h>
#endif

/* pc_printf()
 * Called for general purpose "console" output. This function prints general
 * purpose messages; errors go through pc_error(). The function is modelled
 * after printf().
 */
int pc_printf(const char *message,...)
{
	int ret;
	va_list argptr;

	va_start(argptr,message);
	ret=vprintf(message,argptr);
	va_end(argptr);

	return ret;
}

/* pc_error()
 * Called for producing error output.
 *		number			the error number (as documented in the manual)
 *		message		 a string describing the error with embedded %d and %s tokens
 *		filename		the name of the file currently being parsed
 *		firstline	 the line number at which the expression started on which
 *								the error was found, or -1 if there is no "starting line"
 *		lastline		the line number at which the error was detected
 *		argptr			a pointer to the first of a series of arguments (for macro
 *								"va_arg")
 * Return:
 *		If the function returns 0, the parser attempts to continue compilation.
 *		On a non-zero return value, the parser aborts.
 */
int pc_error(int number,char *message,char *filename,int firstline,int lastline,va_list argptr)
{
static char *prefix[3]={ "error", "fatal error", "warning" };

	if (number!=0) {
		char *pre;
		int idx;

		if (number < 120)
			idx = 0;
		else if (number < 200)
			idx = 1;
		else
			idx = 2;

		pre=prefix[idx];
		if (firstline>=0)
			fprintf(stdout,"%s(%d -- %d) : %s %03d: ",filename,firstline,lastline,pre,number);
		else
			fprintf(stdout,"%s(%d) : %s %03d: ",filename,lastline,pre,number);
	} /* if */
	vfprintf(stdout,message,argptr);
	fflush(stdout);
	return 0;
}

/* pc_opensrc()
 * Opens a source file (or include file) for reading. The "file" does not have
 * to be a physical file, one might compile from memory.
 *		filename		the name of the "file" to read from
 * Return:
 *		The function must return a pointer, which is used as a "magic cookie" to
 *		all I/O functions. When failing to open the file for reading, the
 *		function must return NULL.
 * Note:
 *		Several "source files" may be open at the same time. Specifically, one
 *		file can be open for reading and another for writing.
 */
void *pc_opensrc(char *filename)
{
	#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__ || defined DARWIN
	struct stat fileInfo;
	if (stat(filename, &fileInfo) != 0) {
		return NULL;
	}

	if (S_ISDIR(fileInfo.st_mode)) {
		return NULL;
	}
	#endif

	return fopen(filename,"rt");
}

/* pc_createsrc()
 * Creates/overwrites a source file for writing. The "file" does not have
 * to be a physical file, one might compile from memory.
 *		filename		the name of the "file" to create
 * Return:
 *		The function must return a pointer which is used as a "magic cookie" to
 *		all I/O functions. When failing to open the file for reading, the
 *		function must return NULL.
 * Note:
 *		Several "source files" may be open at the same time. Specifically, one
 *		file can be open for reading and another for writing.
 */
void *pc_createsrc(char *filename)
{
	return fopen(filename,"wt");
}

/* pc_closesrc()
 * Closes a source file (or include file). The "handle" parameter has the
 * value that pc_opensrc() returned in an earlier call.
 */
void pc_closesrc(void *handle)
{
	assert(handle!=NULL);
	fclose((FILE*)handle);
}

/* pc_readsrc()
 * Reads a single line from the source file (or up to a maximum number of
 * characters if the line in the input file is too long).
 */
char *pc_readsrc(void *handle,unsigned char *target,int maxchars)
{
	return fgets((char*)target,maxchars,(FILE*)handle);
}

/* pc_writesrc()
 * Writes to to the source file. There is no automatic line ending; to end a
 * line, write a "\n".
 */
int pc_writesrc(void *handle,unsigned char *source)
{
	return fputs((char*)source,(FILE*)handle) >= 0;
}

#define MAXPOSITIONS  4
static fpos_t srcpositions[MAXPOSITIONS];
static unsigned char srcposalloc[MAXPOSITIONS];

void *pc_getpossrc(void *handle,void *position)
{
	if (position==NULL) {
		/* allocate a new slot */
		int i;
		for (i=0; i<MAXPOSITIONS && srcposalloc[i]!=0; i++)
			/* nothing */;
		assert(i<MAXPOSITIONS); /* if not, there is a queue overrun */
		if (i>=MAXPOSITIONS)
			return NULL;
		position=&srcpositions[i];
		srcposalloc[i]=1;
	} else {
		/* use the gived slot */
		assert((fpos_t*)position>=srcpositions && (fpos_t*)position<srcpositions+sizeof(srcpositions));
	} /* if */
	fgetpos((FILE*)handle,(fpos_t*)position);
	return position;
}

/* pc_resetsrc()
 * "position" may only hold a pointer that was previously obtained from
 * pc_getpossrc()
 */
void pc_resetsrc(void *handle,void *position)
{
	assert(handle!=NULL);
	assert(position!=NULL);
	fsetpos((FILE*)handle,(fpos_t *)position);
	/* note: the item is not cleared from the pool */
}

int pc_eofsrc(void *handle)
{
	return feof((FILE*)handle);
}

/* should return a pointer, which is used as a "magic cookie" to all I/O
 * functions; return NULL for failure
 */
void *pc_openasm(char *filename)
{
	#if defined __MSDOS__ || defined SC_LIGHT
		return fopen(filename,"w+t");
	#else
		return mfcreate(filename);
	#endif
}

void pc_closeasm(void *handle, int deletefile)
{
	#if defined __MSDOS__ || defined SC_LIGHT
		if (handle!=NULL)
			fclose((FILE*)handle);
		if (deletefile)
			remove(outfname);
	#else
		if (handle!=NULL) {
			if (!deletefile)
				mfdump((MEMFILE*)handle);
			mfclose((MEMFILE*)handle);
		} /* if */
	#endif
}

void pc_resetasm(void *handle)
{
	assert(handle!=NULL);
	#if defined __MSDOS__ || defined SC_LIGHT
		fflush((FILE*)handle);
		fseek((FILE*)handle,0,SEEK_SET);
	#else
		mfseek((MEMFILE*)handle,0,SEEK_SET);
	#endif
}

int pc_writeasm(void *handle,char *string)
{
	#if defined __MSDOS__ || defined SC_LIGHT
		return fputs(string,(FILE*)handle) >= 0;
	#else
		return mfputs((MEMFILE*)handle,string);
	#endif
}

char *pc_readasm(void *handle, char *string, int maxchars)
{
	#if defined __MSDOS__ || defined SC_LIGHT
		return fgets(string,maxchars,(FILE*)handle);
	#else
		return mfgets((MEMFILE*)handle,string,maxchars);
	#endif
}

extern memfile_t *bin_file;

/* Should return a pointer, which is used as a "magic cookie" to all I/O
 * functions; return NULL for failure.
 */
void *pc_openbin(char *filename)
{
	return memfile_creat(filename, 1);
}

void pc_closebin(void *handle,int deletefile)
{
	if (deletefile)
	{
		memfile_destroy((memfile_t *)handle);
		bin_file = NULL;
	} else {
		bin_file = (memfile_t *)handle;
	}
}

/* pc_resetbin()
 * Can seek to any location in the file.
 * The offset is always from the start of the file.
 */
void pc_resetbin(void *handle,long offset)
{
	memfile_seek((memfile_t *)handle, offset);
}

int pc_writebin(void *handle,void *buffer,int size)
{
	return memfile_write((memfile_t *)handle, buffer, size);
}

long pc_lengthbin(void *handle)
{
	return memfile_tell((memfile_t *)handle);
}
