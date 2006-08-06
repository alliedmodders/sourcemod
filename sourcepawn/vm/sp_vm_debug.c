#include "sp_vm.h"
#include "sp_vm_debug.h"

int SP_DbgLookupFile(sp_context_t *ctx, ucell_t addr, const char **filename)
{
	uint32_t mid, low, high;
	int diff;

	high = ctx->plugin->debug.files_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = ctx->files[mid].addr - addr;
		if (diff == 0)
		{
			*filename = ctx->files[mid].name;
			return SP_ERR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERR_NOT_FOUND;
}

int SP_DbgLookupFunction(sp_context_t *ctx, ucell_t addr, const char **name)
{
	uint32_t iter, max = ctx->plugin->debug.syms_num;

	for (iter=0; iter<max; iter++)
	{
		if ((ctx->symbols[iter].sym->ident == SP_SYM_FUNCTION) 
			&& (ctx->symbols[iter].codestart <= addr) 
			&& (ctx->symbols[iter].codeend > addr))
		{
			break;
		}
	}

	if (iter >= max)
	{
		return SP_ERR_NOT_FOUND;
	}

	*name = ctx->symbols[iter].name;

	return SP_ERR_NONE;
}

int SP_DbgLookupLine(sp_context_t *ctx, ucell_t addr, uint32_t *line)
{
	uint32_t mid, low, high;
	int diff;

	high = ctx->plugin->debug.lines_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = ctx->lines[mid].addr - addr;
		if (diff == 0)
		{
			*line = ctx->lines[mid].line;
			return SP_ERR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERR_NOT_FOUND;
}

int SP_DbgInstallBreak(sp_context_t *ctx, SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn)
{
	if (ctx->dbreak)
		*oldpfn = ctx->dbreak;

	ctx->dbreak = newpfn;

	return SP_ERR_NONE;
}
