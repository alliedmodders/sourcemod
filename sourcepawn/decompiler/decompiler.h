#ifndef _INCLUDE_SP_DECOMP_ENGINE_H_
#define _INCLUDE_SP_DECOMP_ENGINE_H_

#include <stdio.h>
#include "file_format.h"
#include "code_analyzer.h"

class PrintBuffer
{
public:
	PrintBuffer();
	~PrintBuffer();
public:
	void Set(const char *fmt, ...);
	void Append(const char *fmt, ...);
	void Prepend(const char *fmt, ...);
	void Reset();
	void Dump(FILE *fp);
private:
	void Grow(size_t len);
private:
	char *printbuf;
	size_t printbuf_alloc;
	size_t printbuf_pos;
};

struct sp_decomp_t
{
	sp_file_t *plugin;
	sp_opdef_t opdef[OP_TOTAL+1];
	FunctionInfo *funcs;
	cell_t *pcode_map;
	FunctionInfo *natives;
};

sp_decomp_t *Sp_InitDecomp(const char *file, int *err);
void Sp_FreeDecomp(sp_decomp_t *decomp);

#endif //_INCLUDE_SP_DECOMP_ENGINE_H_
