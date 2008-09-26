#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "decompiler.h"
#include "platform_util.h"

sp_decomp_t *Sp_InitDecomp(const char *file, int *err)
{
	sp_decomp_t *dc;
	sp_file_t *plugin;

	if ((plugin = Sp_ReadPlugin(file, err)) == NULL)
	{
		return NULL;
	}

	dc = new sp_decomp_t;
	memset(dc, 0, sizeof(sp_decomp_t));
	dc->plugin = plugin;
	dc->pcode_map = (cell_t *)malloc(dc->plugin->pcode_size);
	memset(dc->pcode_map, 0, dc->plugin->pcode_size);

#define OPDEF(num,enm,str,prms)	\
	dc->opdef[num].name = str;		\
	dc->opdef[num].params = prms;
#include "opcodes.tbl"
#undef OPDEF

	return dc;
}

void Sp_FreeDecomp(sp_decomp_t *decomp)
{
	FunctionInfo *info, *info_temp;

	info = decomp->funcs;
	while (info != NULL)
	{
		info_temp = info->next;
		delete info;
		info = info_temp;
	}

	Sp_FreePlugin(decomp->plugin);
	free(decomp->pcode_map);
	delete decomp;
}

PrintBuffer::PrintBuffer() : printbuf(NULL), printbuf_alloc(0), printbuf_pos(0)
{
}

PrintBuffer::~PrintBuffer()
{
	free(printbuf);
}

void PrintBuffer::Grow(size_t len)
{
	if (printbuf_pos + len > printbuf_alloc)
	{
		if (printbuf_alloc == 0)
		{
			printbuf_alloc = len * 8;
		}
		else
		{
			printbuf_alloc += len;
			printbuf_alloc *= 2;
		}

		printbuf = (char *)realloc(printbuf, printbuf_alloc);
	}
}

void PrintBuffer::Append(const char *fmt, ...)
{
	size_t len;
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	len = Sp_FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	Grow(len);

	printbuf = (char *)realloc(printbuf, printbuf_alloc);
	strcpy(&printbuf[printbuf_pos], buffer);
	printbuf_pos += len;
}

void PrintBuffer::Set(const char *fmt, ...)
{
	size_t len;
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	len = Sp_FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	Reset();
	Grow(len);

	printbuf = (char *)realloc(printbuf, printbuf_alloc);
	strcpy(&printbuf[printbuf_pos], buffer);
	printbuf_pos += len;
}

void PrintBuffer::Prepend(const char *fmt, ...)
{
	size_t len;
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	len = Sp_FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	Grow(len + 1);

	/* Move old data forward.  Move one extra byte for the null terminator. */
	memmove(&printbuf[len], printbuf, printbuf_pos + 1);
	printbuf_pos += len;
	/* Move the new data in. */
	memcpy(printbuf, buffer, len);
}

void PrintBuffer::Reset()
{
	printbuf_pos = 0;
}

void PrintBuffer::Dump(FILE *fp)
{
	fprintf(fp, "%s", printbuf);
}
