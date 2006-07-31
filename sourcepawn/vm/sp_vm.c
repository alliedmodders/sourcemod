#include <limits.h>
#include <assert.h>
#include "sp_vm.h"

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

int main()
{
	/** temporary testing area */
	sp_context_t ctx;
	cell_t l, *p;

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
