#include "sp_vm.h"
#include "sp_vm_debug.h"

#define USHR(x) ((unsigned int)(x)>>1)

int SP_DbgLookupFile(sp_context_t *ctx, ucell_t addr, const char **filename)
{
	int high, low, mid;

	high = ctx->plugin->debug.files_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (ctx->files[mid].addr <= addr)
		{
			low = mid;
		} else {
			high = mid;
		}
	}

	if (low == -1)
	{
		return SP_ERR_NOT_FOUND;
	}

	*filename = ctx->files[low].name;

	return SP_ERR_NONE;
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
	int high, low, mid;

	high = ctx->plugin->debug.lines_num;
	low = -1;

	while (high - low > 1)
	{
		mid = USHR(low + high);
		if (ctx->lines[mid].addr <= addr)
		{
			low = mid;
		} else {
			high = mid;
		}
	}

	if (low == -1)
	{
		return SP_ERR_NOT_FOUND;
	}

	*line = ctx->lines[low].line;

	return SP_ERR_NONE;
}

int SP_DbgInstallBreak(sp_context_t *ctx, SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn)
{
	if (ctx->dbreak)
		*oldpfn = ctx->dbreak;

	ctx->dbreak = newpfn;

	return SP_ERR_NONE;
}
