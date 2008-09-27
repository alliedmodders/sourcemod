#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
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

	if (plugin->debug.ntv != NULL)
	{
		assert(plugin->debug.ntv->num_entries == plugin->num_natives);
		dc->natives = new FunctionInfo[plugin->num_natives];
		memset(dc->natives, 0, sizeof(FunctionInfo) * plugin->num_natives);

		/* Unfurl this mess. */
		uint8_t *cursor = (uint8_t *)plugin->debug.ntv;

		cursor += sizeof(sp_fdbg_ntvtab_t);
		for (uint32_t i = 0; i < plugin->num_natives; i++)
		{
			sp_fdbg_native_t *native = (sp_fdbg_native_t *)cursor;

			uint32_t idx = native->index;

			assert(dc->natives[idx].name[0] == '\0');

			Sp_Format(dc->natives[idx].name, 
				sizeof(dc->natives[idx].name),
				"%s",
				plugin->debug.stringbase + native->name);
			dc->natives[idx].tag = Sp_FindTag(plugin, native->tagid);
			dc->natives[idx].num_known_args = native->nargs;
			
			cursor += sizeof(sp_fdbg_native_t);

			for (uint32_t j = 0; j < dc->natives[idx].num_known_args; j++)
			{
				sp_fdbg_ntvarg_t *arg = (sp_fdbg_ntvarg_t *)cursor;
				dc->natives[idx].known_args[j].name = 
					plugin->debug.stringbase + arg->name;
				dc->natives[idx].known_args[j].tag = 
					Sp_FindTag(plugin, arg->tagid);
				dc->natives[idx].known_args[j].dimcount = arg->dimcount;

				cursor += sizeof(sp_fdbg_ntvarg_t);
				if (arg->dimcount != 0)
				{
					dc->natives[idx].known_args[j].dims = (sp_fdbg_arraydim_t *)cursor;
					cursor += sizeof(sp_fdbg_arraydim_t) * arg->dimcount;
				}
			}
		}
	}

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
	delete [] decomp->natives;
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

	Grow(len + 1);

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
	Grow(len + 1);

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
