/**
 * vim: set ts=4 :
 * =============================================================================
 * SourcePawn
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

#ifndef _INCLUDE_SPFILE_HEADERS_H
#define _INCLUDE_SPFILE_HEADERS_H

/** 
 * @file sp_file_headers.h
 * @brief Defines the structure present in a SourcePawn compiled binary.
 *
 * Note: These structures should be 1-byte packed to match the file format.
 */

#include <stddef.h>
#if defined __GNUC__ || defined HAVE_STDINT_
#include <stdint.h>
#else
 #if !defined HAVE_STDINT_H
	typedef unsigned __int64	uint64_t;		/**< 64bit unsigned integer */
	typedef __int64				int64_t;		/**< 64bit signed integer */
	typedef unsigned __int32	uint32_t;		/**< 32bit unsigned integer */
	typedef __int32				int32_t;		/**< 32bit signed integer */
	typedef unsigned __int16	uint16_t;		/**< 16bit unsigned integer */
	typedef __int16				int16_t;		/**< 16bit signed integer */
	typedef unsigned __int8		uint8_t;		/**< 8bit unsigned integer */
	typedef __int8				int8_t;			/**< 8bit signed integer */
 #define HAVE_STDINT_H
 #endif
#endif

#define SPFILE_MAGIC	0x53504646		/**< Source Pawn File Format (SPFF) */
#define SPFILE_VERSION	0x0101			/**< Uncompressed bytecode */

//:TODO: better compiler/nix support
#if defined __linux__
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#else
	#pragma pack(push)
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#endif

#define SPFILE_COMPRESSION_NONE		0		/**< No compression in file */
#define SPFILE_COMPRESSION_GZ		1		/**< GZ compression */

/**
 * @brief File section header format.
 */
typedef struct sp_file_section_s
{
	uint32_t	nameoffs;	/**< Relative offset into global string table */
	uint32_t	dataoffs;	/**< Offset into the data section of the file */
	uint32_t	size;		/**< Size of the section's entry in the data section */
} sp_file_section_t;

/**
 * @brief File header format.  If compression is 0, then disksize may be 0 
 * to mean that only the imagesize is needed.
 */
typedef struct sp_file_hdr_s
{
	uint32_t	magic;		/**< Magic number */
	uint16_t	version;	/**< Version code */
	uint8_t		compression;/**< Compression algorithm */
	uint32_t	disksize;	/**< Size on disk */
	uint32_t	imagesize;	/**< Size in memory */
	uint8_t		sections;	/**< Number of sections */
	uint32_t	stringtab;	/**< Offset to string table */
	uint32_t	dataoffs;	/**< Offset to file proper (any compression starts here) */
} sp_file_hdr_t;

#define SP_FLAG_DEBUG	(1<<0)		/**< Debug information is present in the file */

#define SP_CODEVERS_JIT1       9    /**< Code version for JIT1 */
#define SP_CODEVERS_JIT2      10    /**< Code version for JIT2 */    

/**
 * @brief File-encoded format of the ".code" section.
 */
typedef struct sp_file_code_s
{
	uint32_t	codesize;		/**< Codesize in bytes */
	uint8_t		cellsize;		/**< Cellsize in bytes */
	uint8_t		codeversion;	/**< Version of opcodes supported */
	uint16_t	flags;			/**< Flags */
	uint32_t	main;			/**< Address to "main," if any */
	uint32_t	code;			/**< Relative offset to code */
} sp_file_code_t;

/** 
 * @brief File-encoded format of the ".data" section.
 */
typedef struct sp_file_data_s
{
	uint32_t	datasize;		/**< Size of data section in memory */
	uint32_t	memsize;		/**< Total mem required (includes data) */
	uint32_t	data;			/**< File offset to data (helper) */
} sp_file_data_t;

/**
 * @brief File-encoded format of the ".publics" section.
 */
typedef struct sp_file_publics_s
{
	uint32_t	address;		/**< Address relative to code section */
	uint32_t	name;			/**< Index into nametable */
} sp_file_publics_t;

/**
 * @brief File-encoded format of the ".natives" section.
 */
typedef struct sp_file_natives_s
{
	uint32_t	name;			/**< Index into nametable */
} sp_file_natives_t;

/**
 * @brief File-encoded format of the ".pubvars" section.
 */
typedef struct sp_file_pubvars_s
{
	uint32_t	address;		/**< Address relative to the DAT section */
	uint32_t	name;			/**< Index into nametable */
} sp_file_pubvars_t;

/**
 * @brief File-encoded tag info.
 */
typedef struct sp_file_tag_s
{
	uint32_t	tag_id;			/**< Tag ID from compiler */
	uint32_t	name;			/**< Index into nametable */
} sp_file_tag_t;

/**
 * @brief File-encoded debug information table.
 */
typedef struct sp_fdbg_info_s
{
	uint32_t	num_files;	/**< number of files */
	uint32_t	num_lines;	/**< number of lines */
	uint32_t	num_syms;	/**< number of symbols */
	uint32_t	num_arrays;	/**< number of symbols which are arrays */
} sp_fdbg_info_t;

/**
 * @brief File-encoded debug file table.
 */
typedef struct sp_fdbg_file_s
{
	uint32_t	addr;		/**< Address into code */
	uint32_t	name;		/**< Offset into debug nametable */
} sp_fdbg_file_t;

/**
 * @brief File-encoded debug line table.
 */
typedef struct sp_fdbg_line_s
{
	uint32_t	addr;		/**< Address into code */
	uint32_t	line;		/**< Line number */
} sp_fdbg_line_t;

#define SP_SYM_VARIABLE  1  /**< Cell that has an address and that can be fetched directly (lvalue) */
#define SP_SYM_REFERENCE 2  /**< VARIABLE, but must be dereferenced */
#define SP_SYM_ARRAY     3	/**< Symbol is an array */
#define SP_SYM_REFARRAY  4  /**< An array passed by reference (i.e. a pointer) */
#define SP_SYM_FUNCTION	 9  /**< Symbol is a function */
#define SP_SYM_VARARGS	11	/**< Variadic argument start. */

/**
 * @brief File-encoded debug symbol information.
 */
typedef struct sp_fdbg_symbol_s
{
	int32_t		addr;		/**< Address rel to DAT or stack frame */
	int16_t		tagid;		/**< Tag id */
	uint32_t	codestart;	/**< Start scope validity in code */
	uint32_t	codeend;	/**< End scope validity in code */
	uint8_t		ident;		/**< Variable type */
	uint8_t		vclass;		/**< Scope class (local vs global) */
	uint16_t	dimcount;	/**< Dimension count (for arrays) */
	uint32_t	name;		/**< Offset into debug nametable */
} sp_fdbg_symbol_t;

/**
 * @brief File-encoded debug symbol array dimension info.
 */
typedef struct sp_fdbg_arraydim_s
{
	int16_t		tagid;		/**< Tag id */
	uint32_t	size;		/**< Size of dimension */
} sp_fdbg_arraydim_t;

/** Typedef for .names table */
typedef char * sp_file_nametab_t;

/**
 * @brief File encoding for the dbg.natives table.
 *
 * This header is followed by variable length entries of sp_fdbg_native.
 */
typedef struct sp_fdbg_ntvtab_s
{
	uint32_t num_entries;	/**< Number of entries. */
} sp_fdbg_ntvtab_t;

#define SP_NTVDBG_VARARGS	(1<<0)		/**< Formal args are followed by '...' */

/**
 * @brief File encoding of native debug info.
 *
 * Each entry is followed by an sp_fdbg_ntvarg_t for each narg.
 */
typedef struct sp_fdbg_native_s
{
	uint32_t index;			/**< Native index in the plugin. */
	uint32_t name;			/**< Offset into debug nametable. */
	int16_t tagid;			/**< Return tag. */
	uint16_t nargs;			/**< Number of formal arguments. */
} sp_fdbg_native_t;

/**
 * @brief File encoding of native arguments.
 *
 * Each entry is followed by an sp_fdbg_arraydim_t for each dimcount.
 */
typedef struct fp_fdbg_ntvarg_s
{
	uint8_t		ident;		/**< Variable type */
	int16_t		tagid;		/**< Tag id */
	uint16_t	dimcount;	/**< Dimension count (for arrays) */
	uint32_t	name;		/**< Offset into debug nametable */
} sp_fdbg_ntvarg_t;

#if defined __linux__
#pragma pack()    /* reset default packing */
#else
#pragma pack(pop) /* reset previous packing */
#endif

#endif //_INCLUDE_SPFILE_HEADERS_H
