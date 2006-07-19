#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "sp_file.h"
#include "memfile.h"

void *mf_open(const char *name);
void mf_close(void *handle);
size_t mf_write(const void *buf, size_t size, size_t count, void *handle);
size_t mf_read(void *buf, size_t size, size_t count, void *handle);
size_t mf_getpos(void *handle);
int mf_setpos(void *handle, size_t pos);

sp_writefuncs_t cstd_funcs = 
{
	mf_open,
	mf_close,
	mf_write,
	mf_read,
	mf_getpos,
	mf_setpos
};

sp_file_t *spfw_create(const char *name, sp_writefuncs_t *optfuncs)
{
	sp_file_t file;
	sp_file_t *pFile;

	if (!optfuncs)
	{
		optfuncs = &cstd_funcs;
	}

	file.handle = optfuncs->fnOpen(name);
	if (!file.handle)
	{
		return NULL;
	}

	pFile = (sp_file_t *)malloc(sizeof(sp_file_t));

	pFile->handle = file.handle;
	memcpy(&pFile->funcs, optfuncs, sizeof(sp_writefuncs_t));
	pFile->curoffs = 0;
	pFile->header.magic = SPFILE_MAGIC;
	pFile->header.sections = 0;
	pFile->header.stringtab = 0;
	pFile->header.version = SPFILE_VERSION;
	pFile->header.imagesize = 0;
	pFile->header.disksize = 0;
	pFile->header.compression = SPFILE_COMPRESSION_NONE;
	pFile->header.dataoffs = 0;
	pFile->lastsection = 0;
	pFile->offsets = NULL;
	pFile->sections = NULL;
	pFile->state = -1;
	pFile->nametab = NULL;
	pFile->nametab_idx = 0;

	return pFile;
}

void spfw_destroy(sp_file_t *spf)
{
	free(spf->sections);
	free(spf->nametab);
	free(spf->offsets);
	spf->funcs.fnClose(spf->handle);
	free(spf);
}

uint8_t spfw_add_section(sp_file_t *spf, const char *name)
{
	size_t namelen;
	uint8_t s;
	if (spf->state != -1)
	{
		return 0;
	}

	namelen = strlen(name) + 1;

	if (spf->header.sections == 0)
	{
		/** allocate for first section */
		spf->sections = (sp_file_section_t *)malloc(sizeof(sp_file_section_t));
		spf->offsets = (size_t *)malloc(sizeof(size_t));
		spf->nametab = (char *)malloc(namelen);
	} else {
		uint16_t num = spf->header.sections + 1;
		spf->sections = (sp_file_section_t *)realloc(spf->sections, sizeof(sp_file_section_t) * num);
		spf->offsets = (size_t *)realloc(spf->offsets, sizeof(size_t) * num);
		spf->nametab = (char *)realloc(spf->nametab, spf->nametab_idx + namelen);
	}

	s = spf->header.sections;

	spf->sections[s].nameoffs = spf->nametab_idx;
	/** 
	 * "fix" offset will be the second uint2 slot, which is after the previous sections after the header.
	 */
	spf->offsets[s] = sizeof(spf->header) + (sizeof(sp_file_section_t) * spf->header.sections) + sizeof(uint32_t);
	strcpy(&spf->nametab[spf->nametab_idx], name);
	spf->nametab_idx += namelen;

	return ++spf->header.sections;
}

int spfw_finalize_header(sp_file_t *spf)
{
	uint32_t size;
	if (spf->state != -1)
	{
		return -1;
	}

	size = sizeof(sp_file_section_t) * spf->header.sections;

	spf->header.stringtab = sizeof(spf->header) + size;
	spf->header.dataoffs = spf->header.stringtab + spf->nametab_idx;
	if (spf->funcs.fnWrite(&spf->header, sizeof(spf->header), 1, spf->handle) != 1)
	{
		return -1;
	}
	if (spf->funcs.fnWrite(spf->sections, sizeof(sp_file_section_t), spf->header.sections, spf->handle) != 
		spf->header.sections)
	{
		return -1;
	}
	if (spf->funcs.fnWrite(spf->nametab, sizeof(char), spf->nametab_idx, spf->handle) != spf->nametab_idx)
	{
		return -1;
	}
	spf->curoffs = spf->funcs.fnGetPos(spf->handle);
	spf->lastsection = spf->curoffs;
	spf->state++;

	return 0;
}

int spfw_next_section(sp_file_t *spf)
{
	uint8_t s;
	uint32_t rest[2];

	if (spf->state < 0 || spf->state > spf->header.sections)
	{
		return -1;
	}

	if (spf->state == (int)spf->header.sections)
	{
		return 0;
	}

	s = (uint8_t)spf->state;

	spf->curoffs = spf->funcs.fnGetPos(spf->handle);
	spf->funcs.fnSetPos(spf->handle, spf->offsets[s]);
	
	rest[0] = spf->lastsection;
	rest[1] = spf->curoffs - spf->lastsection;
	if (spf->funcs.fnWrite(rest, sizeof(uint32_t), 2, spf->handle) != 2)
	{
		return -1;
	}

	spf->funcs.fnSetPos(spf->handle, spf->curoffs);
	spf->lastsection = spf->curoffs;

	spf->state++;

	return 1;
}

int spfw_finalize_all(sp_file_t *spf)
{
	uint8_t offs;

	if (spf->state < spf->header.sections)
	{
		return -1;
	}

	offs = offsetof(sp_file_hdr_t, imagesize);
	spf->header.disksize = spf->funcs.fnGetPos(spf->handle);
	spf->header.imagesize = spf->funcs.fnGetPos(spf->handle);
	spf->funcs.fnSetPos(spf->handle, offs);
	spf->funcs.fnWrite(&spf->header.imagesize, sizeof(uint32_t), 1, spf->handle);
	spf->funcs.fnSetPos(spf->handle, spf->header.imagesize);

	return 1;
}

/**
 * More memory file operations
 */

void *mf_open(const char *name)
{
	return memfile_creat(name, 1024);
}

void mf_close(void *handle)
{
	memfile_destroy((memfile_t *)handle);
}

size_t mf_write(const void *buf, size_t size, size_t count, void *handle)
{
	if (!count)
	{
		return 0;
	}

	if (memfile_write((memfile_t *)handle, buf, size*count))
	{
		return count;
	}

	return 0;
}

size_t mf_read(void *buf, size_t size, size_t count, void *handle)
{
	return memfile_read((memfile_t *)handle, buf, size*count) / count;
}

size_t mf_getpos(void *handle)
{
	return (long)memfile_tell((memfile_t *)handle);
}

int mf_setpos(void *handle, size_t pos)
{
	memfile_seek((memfile_t *)handle, (long)pos);
	return 1;
}


#if UNUSED_FOR_NOW
/**
 * Default file operations...
 * Based on C standard library calls.
 */

void *fp_open(const char *name)
{
	return fopen(name, "wb");
}

void fp_close(void *handle)
{
	fclose((FILE *)handle);
}

size_t fp_write(const void *buf, size_t size, size_t count, void *handle)
{
	return fwrite(buf, size, count, (FILE *)handle);
}

size_t fp_read(void *buf, size_t size, size_t count, void *handle)
{
	return fread(buf, size, count, (FILE *)handle);
}

size_t fp_getpos(void *handle)
{
	return (size_t)ftell((FILE *)handle);
}

int fp_setpos(void *handle, size_t pos)
{
	return fseek((FILE *)handle, (long)pos, SEEK_SET);
}
#endif
