#include <string.h>
#include <assert.h>
#include <limits.h>
#include "sp_vm_api.h"
#include "sp_vm_basecontext.h"

using namespace SourcePawn;

#define CELLBOUNDMAX	(INT_MAX/sizeof(cell_t))
#define STACKMARGIN		((cell_t)(16*sizeof(cell_t)))

BaseContext::BaseContext(sp_context_t *_ctx)
{
	ctx = _ctx;
}

IVirtualMachine *BaseContext::GetVirtualMachine()
{
	return (IVirtualMachine *)ctx->vmbase;
}

sp_context_t *BaseContext::GetContext()
{
	return ctx;
}

bool BaseContext::IsDebugging()
{
	return (ctx->flags & SPFLAG_PLUGIN_DEBUG);
}

int BaseContext::SetDebugBreak(SPVM_DEBUGBREAK newpfn, SPVM_DEBUGBREAK *oldpfn)
{
	if (!IsDebugging())
	{
		return SP_ERR_NOTDEBUGGING;
	}

	*oldpfn = ctx->dbreak;
	ctx->dbreak = newpfn;

	return SP_ERR_NONE;
}

IPluginDebugInfo *BaseContext::GetDebugInfo()
{
	return this;
}

int BaseContext::Execute(uint32_t public_func, cell_t *result)
{
	IVirtualMachine *vm = (IVirtualMachine *)ctx->vmbase;

	int err;
	sp_public_t *pubfunc;
	if ((err=GetPublicByIndex(public_func, &pubfunc)) != SP_ERR_NONE)
	{
		return err;
	}

	PushCell(ctx->pushcount);
	ctx->pushcount = 0;

	cell_t save_sp = ctx->sp;
	cell_t save_hp = ctx->hp;

	err = vm->ContextExecute(ctx, pubfunc->offs, result);

	ctx->sp = save_sp;
	ctx->hp = save_hp;

	return err;
}

int BaseContext::HeapAlloc(unsigned int cells, cell_t *local_addr, cell_t **phys_addr)
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

int BaseContext::HeapPop(cell_t local_addr)
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


int BaseContext::HeapRelease(cell_t local_addr)
{
	if (local_addr < ctx->heapbase)
	{
		return SP_ERR_INVALID_ADDRESS;
	}

	ctx->hp = local_addr - sizeof(cell_t);

	return SP_ERR_NONE;
}

int BaseContext::FindNativeByName(const char *name, uint32_t *index)
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

int BaseContext::GetNativeByIndex(uint32_t index, sp_native_t **native)
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


uint32_t BaseContext::GetNativesNum()
{
	return ctx->plugin->info.natives_num;
}

int BaseContext::FindPublicByName(const char *name, uint32_t *index)
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

int BaseContext::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
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

uint32_t BaseContext::GetPublicsNum()
{
	return ctx->plugin->info.publics_num;
}

int BaseContext::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
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

int BaseContext::FindPubvarByName(const char *name, uint32_t *index)
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

int BaseContext::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	if (index >= ctx->plugin->info.pubvars_num)
	{
		return SP_ERR_INDEX;
	}

	*local_addr = ctx->plugin->info.pubvars[index].address;
	*phys_addr = ctx->pubvars[index].offs;

	return SP_ERR_NONE;
}

uint32_t BaseContext::GetPubVarsNum()
{
	return ctx->plugin->info.pubvars_num;
}

int BaseContext::BindNatives(sp_nativeinfo_t *natives, unsigned int num, int overwrite)
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



int BaseContext::BindNative(sp_nativeinfo_t *native, uint32_t status)
{
	uint32_t index;
	int err;

	if ((err = FindNativeByName(native->name, &index)) != SP_ERR_NONE)
	{
		return err;
	}

	ctx->natives[index].pfn = native->func;
	ctx->natives[index].status = status;

	return SP_ERR_NONE;
}

int BaseContext::BindNativeToAny(SPVM_NATIVE_FUNC native)
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

int BaseContext::LocalToPhysAddr(cell_t local_addr, cell_t **phys_addr)
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

int BaseContext::PushCell(cell_t value)
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

int BaseContext::PushCellsFromArray(cell_t array[], unsigned int numcells)
{
	unsigned int i;
	int err;

	for (i=0; i<numcells; i++)
	{
		if ((err = PushCell(array[i])) != SP_ERR_NONE)
		{
			ctx->sp += (cell_t)(i * sizeof(cell_t));
			ctx->pushcount -= i;
			return err;
		}
	}

	return SP_ERR_NONE;
}

int BaseContext::PushCellArray(cell_t *local_addr, cell_t **phys_addr, cell_t array[], unsigned int numcells)
{
	cell_t *ph_addr;
	int err;

	if ((err = HeapAlloc(numcells, local_addr, &ph_addr)) != SP_ERR_NONE)
	{
		return err;
	}

	memcpy(ph_addr, array, numcells * sizeof(cell_t));

	if ((err = PushCell(*local_addr)) != SP_ERR_NONE)
	{
		HeapRelease(*local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERR_NONE;
}

int BaseContext::LocalToString(cell_t local_addr, char *buffer, size_t maxlength, int *chars)
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

int BaseContext::PushString(cell_t *local_addr, cell_t **phys_addr, const char *string)
{
	cell_t *ph_addr;
	int err;
	unsigned int i, numcells = strlen(string);

	if ((err = HeapAlloc(numcells+1, local_addr, &ph_addr)) != SP_ERR_NONE)
	{
		return err;
	}

	for (i=0; i<numcells; i++)
	{
		ph_addr[i] = (cell_t)string[i];
	}
	ph_addr[numcells] = '\0';

	if ((err = PushCell(*local_addr)) != SP_ERR_NONE)
	{
		HeapRelease(*local_addr);
		return err;
	}

	if (phys_addr)
	{
		*phys_addr = ph_addr;
	}

	return SP_ERR_NONE;
}

int BaseContext::StringToLocal(cell_t local_addr, size_t chars, const char *source)
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

#define USHR(x) ((unsigned int)(x)>>1)

int BaseContext::LookupFile(ucell_t addr, const char **filename)
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

int BaseContext::LookupFunction(ucell_t addr, const char **name)
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

int BaseContext::LookupLine(ucell_t addr, uint32_t *line)
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
