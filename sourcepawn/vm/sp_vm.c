#include <limits.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "sp_vm.h"

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

int main()
{
	/** temporary testing area */
	sp_context_t ctx;
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
}

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
	uint32_t mid, low, high;
	int diff;

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
	uint32_t mid, low, high;
	int diff;

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
	uint32_t mid, low, high;
	int diff;

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
		return SP_ERR_HEAPLOW;
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
			ctx->sp += i * sizeof(cell_t);
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
	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	if ((err = SP_PushCell(ctx, *local_addr)) != SP_ERR_NONE)
	{
		SP_HeapRelease(ctx, *local_addr);
		return err;
	}

	return SP_ERR_NONE;
}

int SP_LocalToString(sp_context_t *ctx, cell_t local_addr, int *chars, char *buffer, size_t maxlength)
{
	size_t len = 0;
	cell_t *src;

	if (((local_addr >= ctx->hp) && (local_addr < ctx->sp)) || (local_addr < 0) || ((ucell_t)local_addr >= ctx->memory))
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	src = (cell_t *)(ctx->data + local_addr);
	while ((*src != '\0') && (len < maxlength))
	{
		buffer[len++] = (char)*src++;
	}

	if (len >= maxlength)
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

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	if ((err = SP_PushCell(ctx, *local_addr)) != SP_ERR_NONE)
	{
		SP_HeapRelease(ctx, *local_addr);
		return err;
	}

	return SP_ERR_NONE;
}
