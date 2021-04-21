/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn JIT SDK
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
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
	inline cell_t peek_cell()
	{
		return *inptr;
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
	inline void write_ushort(unsigned short c)
	{
		if (outbase)
		{
			*(unsigned short *)outptr = c;
		}
		outptr += sizeof(unsigned short);
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
	inline void write_int64(jit_int64_t c)
	{
		if (outbase)
		{
			*(jit_int64_t *)outptr = c;
		}
		outptr += sizeof(jit_int64_t);
	}
	inline void write_uint64(jit_uint64_t c)
	{
		if (outbase)
		{
			*(jit_uint64_t *)outptr = c;
		}
		outptr += sizeof(jit_uint64_t);
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
