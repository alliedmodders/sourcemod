#include <limits.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "sp_vm.h"

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

/*int main()
{
	/** temporary testing area */
	/*sp_context_t ctx;
	cell_t l, *p;
	cell_t arr1[] = {1,3,3,7};
	cell_t arr2[] = {123,1234,12345,123456};
	const char *str = "hat hat";
	char buf[20];

	ctx.data = (uint8_t *)malloc(50000);
	ctx.memory = 50000;
	ctx.heapbase = 200;
	ctx.hp = ctx.heapbase;
	ctx.sp = 45000;

	assert(SP_HeapAlloc(&ctx, 500, &l, &p) == SP_ERR_NONE);
	assert(SP_HeapPop(&ctx, l) == SP_ERR_NONE);
	assert(SP_HeapRelease(&ctx, l) == SP_ERR_NONE);
	assert(SP_HeapRelease(&ctx, 4) == SP_ERR_INVALID_ADDRESS);
	assert(SP_HeapAlloc(&ctx, 500, &l, &p) == SP_ERR_NONE);
	assert(SP_HeapRelease(&ctx, l) == SP_ERR_NONE);
	assert(SP_PushCell(&ctx, 1337) == SP_ERR_NONE);
	assert(SP_PushCellArray(&ctx, &l, &p, arr1, 4) == SP_ERR_NONE);
	assert(SP_HeapRelease(&ctx, l) == SP_ERR_NONE);
	assert(SP_PushCellsFromArray(&ctx, arr2, 4) == SP_ERR_NONE);
	assert(SP_PushString(&ctx, &l, &p, str) == SP_ERR_NONE);
	assert(SP_LocalToString(&ctx, l, NULL, buf, 20) == SP_ERR_NONE);
	assert(SP_HeapRelease(&ctx, l) == SP_ERR_NONE);

	return 0;
}*/

int SP_HeapAlloc(sp_context_t *ctx, unsigned int cells, cell_t *local_addr, cell_t **phys_addr)
{
	cell_t *addr;
	ucell_t realmem;

#if 0
	if (cells > CELLBOUNDMAX)
	{
		return SP_ERR_PARAM;
	}
#else
	assert(cells < CELLBOUNDMAX);
#endif

	realmem = cells * sizeof(cell_t);

	/**
	 * Check if the space between the heap and stack is sufficient.
	 */
	if ((cell_t)(ctx->sp - ctx->hp - realmem) < STACKMARGIN)
	{
		return SP_ERR_HEAPLOW;
	}

	addr = (cell_t *)(ctx->data + ctx->hp);
	/* store size of allocation in cells */
	*addr = (cell_t)cells;
	addr++;
	ctx->hp += sizeof(cell_t);

	*local_addr = ctx->hp;

	if (phys_addr)
	{
		*phys_addr = addr;
	}

	ctx->hp += realmem;

	return SP_ERR_NONE;
}

int SP_HeapPop(sp_context_t *ctx, cell_t local_addr)
{
	cell_t cellcount;
	cell_t *addr;

	/* check the bounds of this address */
	local_addr -= sizeof(cell_t);
	if (local_addr < ctx->heapbase || local_addr >= ctx->sp)
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	addr = (cell_t *)(ctx->data + local_addr);
	cellcount = (*addr) * sizeof(cell_t);
	/* check if this memory count looks valid */
	if (ctx->hp - cellcount - sizeof(cell_t) != local_addr)
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	ctx->hp = local_addr;

	return SP_ERR_NONE;
}

int SP_HeapRelease(sp_context_t *ctx, cell_t local_addr)
{
	if (local_addr < ctx->heapbase)
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	ctx->hp = local_addr - sizeof(cell_t);

	return SP_ERR_NONE;
}

int SP_FindNativeByName(sp_context_t *ctx, const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = ctx->plugin->info.natives_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(ctx->natives[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERR_NOT_FOUND;
}

int SP_GetNativeByIndex(sp_context_t *ctx, uint32_t index, sp_native_t **native)
{
	if (index >= ctx->plugin->info.natives_num)
	{
		return SP_ERR_INDEX;
	}

	if (native)
	{
		*native = &(ctx->natives[index]);
	}

	return SP_ERR_NONE;
}

int SP_GetNativesNum(sp_context_t *ctx, uint32_t *num)
{
	*num = ctx->plugin->info.natives_num;

	return SP_ERR_NONE;
}

int SP_FindPublicByName(sp_context_t *ctx, const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = ctx->plugin->info.publics_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(ctx->publics[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERR_NOT_FOUND;
}

int SP_GetPublicByIndex(sp_context_t *ctx, uint32_t index, sp_public_t **pblic)
{
	if (index >= ctx->plugin->info.publics_num)
	{
		return SP_ERR_INDEX;
	}

	if (pblic)
	{
		*pblic = &(ctx->publics[index]);
	}

	return SP_ERR_NONE;
}

int SP_GetPublicsNum(sp_context_t *ctx, uint32_t *num)
{
	*num = ctx->plugin->info.publics_num;

	return SP_ERR_NONE;
}

int SP_GetPubvarByIndex(sp_context_t *ctx, uint32_t index, sp_pubvar_t **pubvar)
{
	if (index >= ctx->plugin->info.pubvars_num)
	{
		return SP_ERR_INDEX;
	}

	if (pubvar)
	{
		*pubvar = &(ctx->pubvars[index]);
	}

	return SP_ERR_NONE;
}

int SP_FindPubvarByName(sp_context_t *ctx, const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = ctx->plugin->info.pubvars_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(ctx->pubvars[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERR_NOT_FOUND;
}

int SP_GetPubvarAddrs(sp_context_t *ctx, uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	if (index >= ctx->plugin->info.pubvars_num)
	{
		return SP_ERR_INDEX;
	}

	*local_addr = ctx->plugin->info.pubvars[index].address;
	*phys_addr = ctx->pubvars[index].offs;

	return SP_ERR_NONE;
}

int SP_GetPubVarsNum(sp_context_t *ctx, uint32_t *num)
{
	*num = ctx->plugin->info.pubvars_num;

	return SP_ERR_NONE;
}

int SP_BindNatives(sp_context_t *ctx, sp_nativeinfo_t *natives, unsigned int num, int overwrite)
{
	uint32_t i, j, max;

	max = ctx->plugin->info.natives_num;

	for (i=0; i<max; i++)
	{
		if ((ctx->natives[i].status == SP_NATIVE_OKAY) && !overwrite)
		{
			continue;
		}

		for (j=0; (natives[j].name) && (!num || j<num); j++)
		{
			if (!strcmp(ctx->natives[i].name, natives[j].name))
			{
				ctx->natives[i].pfn = natives[j].func;
				ctx->natives[i].status = SP_NATIVE_OKAY;
			}
		}
	}

	return SP_ERR_NONE;
}

int SP_BindNative(sp_context_t *ctx, sp_nativeinfo_t *native, uint32_t status)
{
	uint32_t index;
	int err;

	if ((err = SP_FindNativeByName(ctx, native->name, &index)) != SP_ERR_NONE)
	{
		return err;
	}

	ctx->natives[index].pfn = native->func;
	ctx->natives[index].status = status;

	return SP_ERR_NONE;
}

int SP_BindNativeToAny(sp_context_t *ctx, SPVM_NATIVE_FUNC native)
{
	uint32_t nativesnum, i;

	nativesnum = ctx->plugin->info.natives_num;

	for (i=0; i<nativesnum; i++)
	{
		if (ctx->natives[i].status != SP_NATIVE_OKAY)
		{
			ctx->natives[i].pfn = native;
			ctx->natives[i].status = SP_NATIVE_PENDING;
		}
	}

	return SP_ERR_NONE;
}

int SP_LocalToPhysAddr(sp_context_t *ctx, cell_t local_addr, cell_t **phys_addr)
{
	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->memory))
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	if (phys_addr)
	{
		*phys_addr = (cell_t *)(ctx->data + local_addr);
	}

	return SP_ERR_NONE;
}

int SP_PushCell(sp_context_t *ctx, cell_t value)
{
	if ((ctx->hp + STACKMARGIN) > (cell_t)(ctx->sp - sizeof(cell_t)))
	{
		return SP_ERR_STACKERR;
	}

	ctx->sp -= sizeof(cell_t);
	*(cell_t *)(ctx->data + ctx->sp) = value;
	ctx->pushcount++;

	return SP_ERR_NONE;
}

int SP_PushCellsFromArray(sp_context_t *ctx, cell_t array[], unsigned int numcells)
{
	unsigned int i;
	int err;

	for (i=0; i<numcells; i++)
	{
		if ((err = SP_PushCell(ctx, array[i])) != SP_ERR_NONE)
		{
			ctx->sp += (cell_t)(i * sizeof(cell_t));
			ctx->pushcount -= i;
			return err;
		}
	}

	return SP_ERR_NONE;
}

int SP_PushCellArray(sp_context_t *ctx, cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
{
	cell_t *ph_addr;
	int err;

	if ((err = SP_HeapAlloc(ctx, numcells, local_addr, &ph_addr)) != SP_ERR_NONE)
	{
		return err;
	}

	memcpy(ph_addr, array, numcells * sizeof(cell_t));

	if ((err = SP_PushCell(ctx, *local_addr)) != SP_ERR_NONE)
	{
		SP_HeapRelease(ctx, *local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERR_NONE;
}

int SP_LocalToString(sp_context_t *ctx, cell_t local_addr, int *chars, char *buffer, size_t maxlength)
{
	int len = 0;
	cell_t *src;

	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->memory))
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	src = (cell_t *)(ctx->data + local_addr);
	while ((*src != '\0') && ((size_t)len < maxlength))
	{
		buffer[len++] = (char)*src++;
	}

	if ((size_t)len >= maxlength)
	{
		len = maxlength - 1;
	}
	if (len >= 0)
	{
		buffer[len] = '\0';
	}

	if (chars)
	{
		*chars = len;
	}

	return SP_ERR_NONE;
}

int SP_PushString(sp_context_t *ctx, cell_t *local_addr, cell_t **phys_addr, const char *string)
{
	cell_t *ph_addr;
	int err;
	unsigned int i, numcells = strlen(string);

	if ((err = SP_HeapAlloc(ctx, numcells+1, local_addr, &ph_addr)) != SP_ERR_NONE)
	{
		return err;
	}

	for (i=0; i<numcells; i++)
	{
		ph_addr[i] = (cell_t)string[i];
	}
	ph_addr[numcells] = '\0';

	if ((err = SP_PushCell(ctx, *local_addr)) != SP_ERR_NONE)
	{
		SP_HeapRelease(ctx, *local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERR_NONE;
}

int SP_StringToLocal(sp_context_t *ctx, cell_t local_addr, size_t chars, const char *source)
{
	cell_t *dest;
	int i, len;

	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->memory))
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	len = strlen(source);
	dest = (cell_t *)(ctx->data + local_addr);

	if ((size_t)len >= chars)
	{
		len = chars - 1;
	}

	for (i=0; i<len; i++)
	{
		dest[i] = (cell_t)source[i];
	}
	dest[len] = '\0';

	return SP_ERR_NONE;
}

int SP_CreateBaseContext(sp_plugin_t *plugin, sp_context_t **ctx)
{
	uint32_t iter, max;
	uint8_t *dat, *cursor;
	const char *strbase;
	sp_fdbg_symbol_t *sym;
	sp_fdbg_arraydim_t *arr;
	sp_context_t *context;

	context = (sp_context_t *)malloc(sizeof(sp_context_t));
	memset(context, 0, sizeof(sp_context_t));

	context->base = plugin->base;
	context->plugin = plugin;
	context->flags = plugin->flags;

	context->data = (uint8_t *)malloc(plugin->memory);
	memcpy(context->data, plugin->data, plugin->data_size);
	context->memory = plugin->memory;
	context->heapbase = (cell_t)(plugin->data_size);

	strbase = plugin->info.stringbase;

	if (max = plugin->info.publics_num)
	{
		context->publics = (sp_public_t *)malloc(sizeof(sp_public_t) * max);
		for (iter=0; iter<max; iter++)
		{
			context->publics[iter].name = strbase + plugin->info.publics[iter].name;
			context->publics[iter].offs = plugin->info.publics[iter].address;
		}
	}

	if (max = plugin->info.pubvars_num)
	{
		dat = plugin->data;
		context->pubvars = (sp_pubvar_t *)malloc(sizeof(sp_pubvar_t) * max);
		for (iter=0; iter<max; iter++)
		{
			context->pubvars[iter].name = strbase + plugin->info.pubvars[iter].name;
			context->pubvars[iter].offs = (cell_t *)(dat + plugin->info.pubvars[iter].address);
		}
	}

	if (max = plugin->info.natives_num)
	{
		context->natives = (sp_native_t *)malloc(sizeof(sp_native_t) * max);
		for (iter=0; iter<max; iter++)
		{
			context->natives[iter].name = strbase + plugin->info.natives[iter].name;
			context->natives[iter].pfn = SP_NoExecNative;
			context->natives[iter].status = SP_NATIVE_NONE;
		}
	}

	strbase = plugin->debug.stringbase;

	if (plugin->flags & SP_FLAG_DEBUG)
	{
		max = plugin->debug.files_num;
		context->files = (sp_debug_file_t *)malloc(sizeof(sp_debug_file_t) * max);
		for (iter=0; iter<max; iter++)
		{
			context->files[iter].addr = plugin->debug.files[iter].addr;
			context->files[iter].name = strbase + plugin->debug.files[iter].name;
		}

		max = plugin->debug.lines_num;
		context->lines = (sp_debug_line_t *)malloc(sizeof(sp_debug_line_t) * max);
		for (iter=0; iter<max; iter++)
		{
			context->lines[iter].addr = plugin->debug.lines[iter].addr;
			context->lines[iter].line = plugin->debug.lines[iter].line;
		}

		cursor = (uint8_t *)(plugin->debug.symbols);
		max = plugin->debug.syms_num;
		context->symbols = (sp_debug_symbol_t *)malloc(sizeof(sp_debug_symbol_t) * max);
		for (iter=0; iter<max; iter++)
		{
			sym = (sp_fdbg_symbol_t *)cursor;

			context->symbols[iter].codestart = sym->codestart;
			context->symbols[iter].codeend = sym->codeend;
			context->symbols[iter].name = strbase + sym->name;
			context->symbols[iter].sym = sym;

			if (sym->dimcount > 0)
			{
				cursor += sizeof(sp_fdbg_symbol_t);
				arr = (sp_fdbg_arraydim_t *)cursor;
				context->symbols[iter].dims = arr;
				cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
				continue;
			}

			context->symbols[iter].dims = NULL;
			cursor += sizeof(sp_fdbg_symbol_t);
		}
	}

	*ctx = context;
	return SP_ERR_NONE;
}

int SP_FreeBaseContext(sp_context_t *ctx)
{
	if (ctx->flags & SP_FLAG_DEBUG)
	{
		free(ctx->symbols);
		free(ctx->lines);
		free(ctx->files);
	}
	if (ctx->plugin->info.natives)
	{
		free(ctx->natives);
	}
	if (ctx->plugin->info.pubvars_num)
	{
		free(ctx->pubvars);
	}
	if (ctx->plugin->info.publics_num)
	{
		free(ctx->publics);
	}
	free(ctx->data);
	free(ctx);

	return SP_ERR_NONE;
}

cell_t SP_NoExecNative(sp_context_t *ctx, cell_t *params)
{
	ctx->err = SP_ERR_NATIVE_PENDING;

	return 0;
}