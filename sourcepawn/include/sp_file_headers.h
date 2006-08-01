#ifndef _INCLUDE_SPFILE_HEADERS_H
#define _INCLUDE_SPFILE_HEADERS_H

#include <stddef.h>
#if defined __GNUC__ || defined HAVE_STDINT_
#include <stdint.h>
#else
 #if !defined HAVE_STDINT_H
	typedef unsigned __int64	uint64_t;
	typedef __int64				int64_t;
	typedef unsigned __int32	uint32_t;
	typedef __int32				int32_t;
	typedef unsigned __int16	uint16_t;
	typedef __int16				int16_t;
	typedef unsigned __int8		uint8_t;
	typedef __int8				int8_t;
 #define HAVE_STDINT_H
 #endif
#endif

#define SPFILE_MAGIC	0x53504646		/* Source Pawn File Format (SPFF) */
//#define SPFILE_VERSION	0x0100
#define SPFILE_VERSION	0x0101			/* Uncompressed bytecode */

//:TODO: better compiler/nix support
#if defined __linux__
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#else
	#pragma pack(push)
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#endif

#define SPFILE_COMPRESSION_NONE		0
#define SPFILE_COMPRESSION_GZ		1

typedef struct sp_file_section_s
{
	uint32_t	nameoffs;	/* rel offset into global string table */
	uint32_t	dataoffs;
	uint32_t	size;
} sp_file_section_t;

/**
 * If compression is 0, then
 *  disksize may be 0 to mean that
 *  only the imagesize is needed.
 */
typedef struct sp_file_hdr_s
{
	uint32_t	magic;		/* magic number */
	uint16_t	version;	/* version code */
	uint8_t		compression;/* compression algorithm */
	uint32_t	disksize;	/* size on disk */
	uint32_t	imagesize;	/* size in memory */
	uint8_t		sections;	/* number of sections */
	uint32_t	stringtab;	/* offset to string table */
	uint32_t	dataoffs;	/* offset to file proper (any compression starts here) */
} sp_file_hdr_t;

typedef enum
{
	SP_FILE_NONE = 0,
	SP_FILE_DEBUG = 1,
} sp_file_flags_t;

/* section is ".code" */
typedef struct sp_file_code_s
{
	uint32_t	codesize;		/* codesize in bytes */
	uint8_t		cellsize;		/* cellsize in bytes */
	uint8_t		codeversion;	/* version of opcodes supported */
	uint16_t	flags;			/* flags */
	uint32_t	main;			/* address to "main" if any */
	uint32_t	code;			/* rel offset to code */
} sp_file_code_t;

/* section is .data */
typedef struct sp_file_data_s
{
	uint32_t	datasize;		/* size of data section in memory */
	uint32_t	memsize;		/* total mem required (includes data) */
	uint32_t	data;			/* file offset to data (helper) */
} sp_file_data_t;

/* section is .publics */
typedef struct sp_file_publics_s
{
	uint32_t	address;		/* address rel to code section */
	uint32_t	name;			/* index into nametable */
} sp_file_publics_t;

/* section is .natives */
typedef struct sp_file_natives_s
{
	uint32_t	name;			/* name of native at index */
} sp_file_natives_t;

/* section is .pubvars */
typedef struct sp_file_pubvars_s
{
	uint32_t	address;		/* address rel to dat section */
	uint32_t	name;			/* index into nametable */
} sp_file_pubvars_t;

#if defined __linux__
	#pragma pack()    /* reset default packing */
#else
	#pragma pack(pop) /* reset previous packing */
#endif

typedef struct sp_fdbg_info_s
{
	uint32_t	num_files;	/* number of files */
	uint32_t	num_lines;	/* number of lines */
	uint32_t	num_syms;	/* number of symbols */
	uint32_t	num_arrays;	/* number of symbols which are arrays */
} sp_fdbg_info_t;

/**
 * Debug information structures
 */
typedef struct sp_fdbg_file_s
{
	uint32_t	addr;		/* address into code */
	uint32_t	name;		/* offset into debug nametable */
} sp_fdbg_file_t;

typedef struct sp_fdbg_line_s
{
	uint32_t	addr;		/* address into code */
	uint32_t	line;		/* line number */
} sp_fdbg_line_t;

#define SP_SYM_VARIABLE  1  /* cell that has an address and that can be fetched directly (lvalue) */
#define SP_SYM_REFERENCE 2  /* VARIABLE, but must be dereferenced */
#define SP_SYM_ARRAY     3
#define SP_SYM_REFARRAY  4  /* an array passed by reference (i.e. a pointer) */
#define SP_SYM_FUNCTION	 9

typedef struct sp_fdbg_symbol_s
{
	int32_t		addr;		/* address rel to DAT or stack frame */
	int16_t		tagid;		/* tag id */
	uint32_t	codestart;	/* start scope validity in code */
	uint32_t	codeend;	/* end scope validity in code */
	uint8_t		ident;		/* variable type */
	uint8_t		vclass;		/* scope class (local vs global) */
	uint16_t	dimcount;	/* dimension count (for arrays) */
	uint32_t	name;		/* offset into debug nametable */
} sp_fdbg_symbol_t;

typedef struct sp_fdbg_arraydim_s
{
	int16_t		tagid;		/* tag id */
	uint32_t	size;		/* size of dimension */
} sp_fdbg_arraydim_t;

/* section is .names */
typedef char * sp_file_nametab_t;

#endif //_INCLUDE_SPFILE_HEADERS_H
