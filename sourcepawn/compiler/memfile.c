#include "memfile.h"
#include <string.h>
#include "osdefs.h"

memfile_t *memfile_creat(const char *name, size_t init)
{
	memfile_t mf;
	memfile_t *pmf;

	mf.size = init;
	mf.base = (char *)malloc(init);
	mf.usedoffs = 0;
	if (!mf.base)
	{
		return NULL;
	}

	mf.offs = 0;
	mf._static = 0;

	pmf = (memfile_t *)malloc(sizeof(memfile_t));
	memcpy(pmf, &mf, sizeof(memfile_t));

	pmf->name = strdup(name);

	return pmf;
}

void memfile_destroy(memfile_t *mf)
{
	if (!mf->_static)
	{
		free(mf->name);
		free(mf->base);
		free(mf);
	}
}

void memfile_seek(memfile_t *mf, long seek)
{
	mf->offs = seek;
}

long memfile_tell(memfile_t *mf)
{
	return mf->offs;
}

size_t memfile_read(memfile_t *mf, void *buffer, size_t maxsize)
{
	if (!maxsize || mf->offs >= mf->usedoffs)
	{
		return 0;
	}

	if (mf->usedoffs - mf->offs < (long)maxsize)
	{
		maxsize = mf->usedoffs - mf->offs;
		if (!maxsize)
		{
			return 0;
		}
	}

	memcpy(buffer, mf->base + mf->offs, maxsize);

	mf->offs += maxsize;

	return maxsize;
}

int memfile_write(memfile_t *mf, void *buffer, size_t size)
{
	if (mf->offs + size > mf->size)
	{
		size_t newsize = (mf->size + size) * 2;
		if (mf->_static)
		{
			char *oldbase = mf->base;
			mf->base = (char *)malloc(newsize);
			if (!mf->base)
			{
				return 0;
			}
			memcpy(mf->base, oldbase, mf->size);
		} else {
			mf->base = (char *)realloc(mf->base, newsize);
			if (!mf->base)
			{
				return 0;
			}
		}
		mf->_static = 0;
		mf->size = newsize;
	}
	memcpy(mf->base + mf->offs, buffer, size);
	mf->offs += size;

	if (mf->offs > mf->usedoffs)
	{
		mf->usedoffs = mf->offs;
	}

	return 1;
}
