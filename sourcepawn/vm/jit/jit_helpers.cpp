#include <string.h>
#include <assert.h>
#include "jit_helpers.h"

cell_t jit_read_cell(jitinfo_t *jit)
{
	cell_t val = *(jit->inptr);
	jit->inptr++;
	return val;
}

cell_t jit_read_cell_null(jitinfo_t *jit)
{
	return 0;
}

cell_t *jit_read_cellptr(jitinfo_t *jit)
{
	cell_t *val = *(cell_t **)(jit->inptr);
	jit->inptr++;
	return val;
}

cell_t *jit_read_cellptr_null(jitinfo_t *jit)
{
	return NULL;
}

void jit_write_ubyte(jitinfo_t *jit, jit_uint8_t c)
{
	*(jit->outptr++) = c;
}

void jit_write_ubyte_null(jitinfo_t *jit, jit_uint8_t c)
{
	jit->outptr++;
}

void jit_write_byte(jitinfo_t *jit, jit_int8_t c)
{
	*(jit->outptr++) = c;
}

void jit_write_byte_null(jitinfo_t *jit, jit_int8_t c)
{
	jit->outptr++;
}

void jit_write_int32(jitinfo_t *jit, jit_int32_t c)
{
	jit_int32_t *ptr = (jit_int32_t *)jit->outptr;
	*ptr = c;
	jit->outptr += sizeof(jit_int32_t);
}

void jit_write_int32_null(jitinfo_t *jit, jit_int32_t c)
{
	jit->outptr += sizeof(jit_int32_t);
}

void jit_write_int64(jitinfo_t *jit, jit_int64_t c)
{
	jit_int64_t *ptr = (jit_int64_t *)jit->outptr;
	*ptr = c;
	jit->outptr += sizeof(jit_int64_t);
}

void jit_write_int64_null(jitinfo_t *jit, jit_int64_t c)
{
	jit->outptr += sizeof(jit_int64_t);
}

jitoffs_t jit_curpos(jitinfo_t *jit)
{
	return (jit->outptr - jit->outbase);
}

jitwritefuncs_t g_write_funcs = 
{
	jit_read_cell,
	jit_read_cellptr,
	jit_write_ubyte,
	jit_write_byte,
	jit_write_int32,
	jit_write_int64,
	jit_curpos
};

jitwritefuncs_t g_write_null_funcs = 
{
	jit_read_cell_null,
	jit_read_cellptr_null,
	jit_write_ubyte_null,
	jit_write_byte_null,
	jit_write_int32_null,
	jit_write_int64_null,
	jit_curpos
};


