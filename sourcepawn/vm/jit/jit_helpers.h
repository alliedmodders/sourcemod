#ifndef _INCLUDE_SOURCEPAWN_JIT_HELPERS_H_
#define _INCLUDE_SOURCEPAWN_JIT_HELPERS_H_

#include <sp_vm_types.h>

#if defined HAVE_STDINT_H && !defined WIN32
#include <stdint.h>
typedef int8_t jit_int8_t;
typedef uint8_t jit_uint8_t;
typedef int32_t jit_int32_t;
typedef uint32_t jit_uint32_t;
typedef int64_t jit_int64_t;
typedef uint64_t jit_uint64_t;
#elif defined WIN32
typedef __int8 jit_int8_t;
typedef unsigned __int8 jit_uint8_t;
typedef __int32 jit_int32_t;
typedef unsigned __int32 jit_uint32_t;
typedef __int64 jit_int64_t;
typedef unsigned __int64 jit_uint64_t;
#endif

typedef char * jitcode_t;
typedef unsigned int jitoffs_t;

class JitWriter
{
public:
	inline cell_t read_cell()
	{
		cell_t val = *(inptr);
		inptr++;
		return val;
	}
	inline cell_t *read_cellptr()
	{
		cell_t *val = *(cell_t **)(inptr);
		inptr++;
		return val;
	}
	inline void write_ubyte(jit_uint8_t c)
	{
		if (outptr)
		{
			*outptr = c;
		}
		outptr++;
	}
	inline void write_byte(jit_int8_t c)
	{
		if (outptr)
		{
			*outptr = c;
		}
		outptr++;
	}
	inline void write_int32(jit_int32_t c)
	{
		if (outptr)
		{
			*(jit_int32_t *)outptr = c;
		}
		outptr += sizeof(jit_int32_t);
	}
	inline jitoffs_t jit_curpos()
	{
		return (outptr - outbase);
	}
public:
	cell_t *inptr;		/* input pointer */
	jitcode_t outbase;	/* output pointer */
	jitcode_t outptr;	/* output base */
};

#endif //_INCLUDE_SOURCEPAWN_JIT_HELPERS_H_
