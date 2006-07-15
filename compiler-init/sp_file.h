#ifndef _INCLUDE_SPFILE_H
#define _INCLUDE_SPFILE_H

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

#define SPFILE_MAGIC	0xDEADC0D3
#define SPFILE_VERSION	0x0100

//:TODO: better compiler/nix support
#if defined __linux__
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#else
	#pragma pack(push)
	#pragma pack(1)         /* structures must be packed (byte-aligned) */
#endif

typedef struct sp_file_section_s
{
	uint32_t	nameoffs;	/* rel offset into global string table */
	uint32_t	dataoffs;
	uint32_t	size;
} sp_file_section_t;

typedef struct sp_file_hdr_s
{
	uint32_t	magic;
	uint16_t	version;
	uint32_t	imagesize;
	uint8_t		sections;
	uint32_t	stringtab;
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
	uint32_t	datasize;		/* size of data section */
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


/* section is .names */
typedef char * sp_file_nametab_t;

#endif //_INCLUDE_SPFILE_H
