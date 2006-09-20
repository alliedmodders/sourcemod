#ifndef _INCLUDE_AMX_JIT_H
#define _INCLUDE_AMX_JIT_H

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

//functions for writing to a JIT
typedef struct tagJITWRITEFUNCS
{
	//Reading from the input stream
	cell_t (*read_cell)(struct tagJITINFO *);
	cell_t *(*read_cellptr)(struct tagJITINFO *);
	//Writing to the output stream
	void (*write_ubyte)(struct tagJITINFO *, jit_uint8_t);
	void (*write_byte)(struct tagJITINFO *, jit_int8_t);
	void (*write_int32)(struct tagJITINFO *, jit_int32_t);
	void (*write_int64)(struct tagJITINFO *, jit_int64_t);
	//helpers
	jitoffs_t (*jit_curpos)(struct tagJITINFO *);
} jitwritefuncs_t;

typedef struct tagJITINFO
{
	jitwritefuncs_t wrfuncs;
	cell_t *inptr;		/* input pointer */
	jitcode_t outbase;	/* output pointer */
	jitcode_t outptr;	/* output base */
} jitinfo_t;

#endif //_INCLUDE_AMX_JIT_H
