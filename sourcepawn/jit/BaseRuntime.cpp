#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseRuntime.h"
#include "sp_vm_engine.h"
#include "x86/jit_x86.h"
#include "sp_vm_basecontext.h"
#include "engine2.h"

using namespace SourcePawn;

int GlobalDebugBreak(BaseContext *ctx, uint32_t frm, uint32_t cip)
{
	g_engine1.RunTracer(ctx, frm, cip);

	return SP_ERROR_NONE;
}

BaseRuntime::BaseRuntime(sp_plugin_t *pl) : m_Debug(pl), m_pPlugin(pl)
{
	m_pCtx = new BaseContext(this);

	if (m_pPlugin->info.publics_num > 0)
	{
		m_PubFuncs = new CFunction *[m_pPlugin->info.publics_num];
		memset(m_PubFuncs, 0, sizeof(CFunction *) * m_pPlugin->info.publics_num);
	}
	else
	{
		m_PubFuncs = NULL;
	}

	m_pPlugin->dbreak = GlobalDebugBreak;
	m_pPlugin->profiler = g_engine2.GetProfiler();
}

BaseRuntime::~BaseRuntime()
{
	for (uint32_t i = 0; i < m_pPlugin->info.publics_num; i++)
	{
		delete m_PubFuncs[i];
		m_PubFuncs[i] = NULL;
	}
	delete [] m_PubFuncs;

	ClearCompile();

	free(m_pPlugin->base);
	delete [] m_pPlugin->memory;
	delete m_pPlugin;
}

void BaseRuntime::ClearCompile()
{
	g_Jit1.FreeContextVars(m_pCtx->GetCtx());
	g_Jit1.FreePluginVars(m_pPlugin);
}

int BaseRuntime::FindNativeByName(const char *name, uint32_t *index)
{
	int high;

	high = m_pPlugin->info.natives_num - 1;

	for (uint32_t i=0; i<m_pPlugin->info.natives_num; i++)
	{
		if (strcmp(m_pPlugin->natives[i].name, name) == 0)
		{
			if (index)
			{
				*index = i;
			}
			return SP_ERROR_NONE;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseRuntime::GetNativeByIndex(uint32_t index, sp_native_t **native)
{
	if (index >= m_pPlugin->info.natives_num)
	{
		return SP_ERROR_INDEX;
	}

	if (native)
	{
		*native = &(m_pPlugin->natives[index]);
	}

	return SP_ERROR_NONE;
}


uint32_t BaseRuntime::GetNativesNum()
{
	return m_pPlugin->info.natives_num;
}

int BaseRuntime::FindPublicByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = m_pPlugin->info.publics_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(m_pPlugin->publics[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERROR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseRuntime::GetPublicByIndex(uint32_t index, sp_public_t **pblic)
{
	if (index >= m_pPlugin->info.publics_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pblic)
	{
		*pblic = &(m_pPlugin->publics[index]);
	}

	return SP_ERROR_NONE;
}

uint32_t BaseRuntime::GetPublicsNum()
{
	return m_pPlugin->info.publics_num;
}

int BaseRuntime::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
	if (index >= m_pPlugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	if (pubvar)
	{
		*pubvar = &(m_pPlugin->pubvars[index]);
	}

	return SP_ERROR_NONE;
}

int BaseRuntime::FindPubvarByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = m_pPlugin->info.pubvars_num - 1;
	low = 0;

	while (low <= high)
	{
		mid = (low + high) / 2;
		diff = strcmp(m_pPlugin->pubvars[mid].name, name);
		if (diff == 0)
		{
			if (index)
			{
				*index = mid;
			}
			return SP_ERROR_NONE;
		} else if (diff < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseRuntime::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	if (index >= m_pPlugin->info.pubvars_num)
	{
		return SP_ERROR_INDEX;
	}

	*local_addr = m_pPlugin->info.pubvars[index].address;
	*phys_addr = m_pPlugin->pubvars[index].offs;

	return SP_ERROR_NONE;
}

uint32_t BaseRuntime::GetPubVarsNum()
{
	return m_pPlugin->info.pubvars_num;
}

IPluginContext *BaseRuntime::GetDefaultContext()
{
	return m_pCtx;
}

IPluginDebugInfo *BaseRuntime::GetDebugInfo()
{
	if (!IsDebugging())
	{
		return NULL;
	}

	return &m_Debug;
}

void BaseRuntime::RefreshFunctionCache()
{
	if (m_PubFuncs != NULL)
	{
		sp_public_t *pub;
		for (uint32_t i = 0; i < m_pPlugin->info.publics_num; i++)
		{
			if (m_PubFuncs[i] == NULL)
			{
				continue;
			}
			if (GetPublicByIndex(i, &pub) != SP_ERROR_NONE)
			{
				continue;
			}
			m_PubFuncs[i]->Set(pub->code_offs, this, (i << 1) | 1, i);
		}
	}

}

IPluginFunction *BaseRuntime::GetFunctionById(funcid_t func_id)
{
	CFunction *pFunc = NULL;

	if (func_id & 1)
	{
		func_id >>= 1;
		if (func_id >= m_pPlugin->info.publics_num)
		{
			return NULL;
		}
		pFunc = m_PubFuncs[func_id];
		if (!pFunc)
		{
			m_PubFuncs[func_id] = new CFunction(m_pPlugin->publics[func_id].code_offs, 
				this, 
				(func_id << 1) | 1,
				func_id);
			pFunc = m_PubFuncs[func_id];
		}
		else if (pFunc->IsInvalidated())
		{
			pFunc->Set(m_pPlugin->publics[func_id].code_offs, 
				this,
				(func_id << 1) | 1,
				func_id);
		}
	}

	return pFunc;
}

IPluginFunction *BaseRuntime::GetFunctionByName(const char *public_name)
{
	uint32_t index;

	if (FindPublicByName(public_name, &index) != SP_ERROR_NONE)
	{
		return NULL;
	}

	CFunction *pFunc = m_PubFuncs[index];
	if (!pFunc)
	{
		sp_public_t *pub = NULL;
		GetPublicByIndex(index, &pub);
		if (pub)
		{
			m_PubFuncs[index] = new CFunction(pub->code_offs, this, (index << 1) | 1, index);
		}
		pFunc = m_PubFuncs[index];
	}
	else if (pFunc->IsInvalidated())
	{
		sp_public_t *pub = NULL;
		GetPublicByIndex(index, &pub);
		if (pub)
		{
			pFunc->Set(pub->code_offs, this, (index << 1) | 1, index);
		}
		else
		{
			pFunc = NULL;
		}
	}

	return pFunc;
}

bool BaseRuntime::IsDebugging()
{
	return ((m_pPlugin->run_flags & SPFLAG_PLUGIN_DEBUG) == SPFLAG_PLUGIN_DEBUG);
}

void BaseRuntime::SetPauseState(bool paused)
{
	if (paused)
	{
		m_pPlugin->run_flags |= SPFLAG_PLUGIN_PAUSED;
	}
	else
	{
		m_pPlugin->run_flags &= ~SPFLAG_PLUGIN_PAUSED;
	}
}

bool BaseRuntime::IsPaused()
{
	return ((m_pPlugin->run_flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED);
}

size_t BaseRuntime::GetMemUsage()
{
	size_t mem = 0;

	mem += sizeof(this);
	mem += sizeof(sp_plugin_t);
	mem += sizeof(BaseContext);
	mem += m_pPlugin->base_size;
	mem += m_pPlugin->jit_codesize;
	mem += m_pPlugin->jit_memsize;

	return mem;
}

BaseContext *BaseRuntime::GetBaseContext()
{
	return m_pCtx;
}

