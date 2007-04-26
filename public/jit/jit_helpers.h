/**
 * vim: set ts=4 :
 * ================================================================
 * SourcePawn (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_JIT_HELPERS_H_
#define _INCLUDE_SOURCEPAWN_JIT_HELPERS_H_

#include <sp_vm_types.h>
#include <sp_vm_api.h>

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
typedef signed int jitrel_t;

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
		if (outbase)
		{
			*outptr = c;
		}
		outptr++;
	}
	inline void write_byte(jit_int8_t c)
	{
		if (outbase)
		{
			*outptr = c;
		}
		outptr++;
	}
	inline void write_int32(jit_int32_t c)
	{
		if (outbase)
		{
			*(jit_int32_t *)outptr = c;
		}
		outptr += sizeof(jit_int32_t);
	}
	inline void write_uint32(jit_uint32_t c)
	{
		if (outbase)
		{
			*(jit_uint32_t *)outptr = c;
		}
		outptr += sizeof(jit_uint32_t);
	}
	inline jitoffs_t get_outputpos()
	{
		return (outptr - outbase);
	}
	inline void set_outputpos(jitoffs_t offs)
	{
		outptr = outbase + offs;
	}
	inline jitoffs_t get_inputpos()
	{
		return (jitoffs_t)((char *)inptr - (char *)inbase);
	}
public:
	cell_t *inptr;		/* input pointer */
	cell_t *inbase;		/* input base */
	jitcode_t outbase;	/* output pointer */
	jitcode_t outptr;	/* output base */
	SourcePawn::ICompilation *data;	/* compiler live info */
};

#endif //_INCLUDE_SOURCEPAWN_JIT_HELPERS_H_
