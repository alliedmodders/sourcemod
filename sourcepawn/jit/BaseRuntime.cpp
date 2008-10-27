#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BaseRuntime.h"
#include "sp_vm_engine.h"
#include "x86/jit_x86.h"
#include "sp_vm_basecontext.h"
#include "engine2.h"

using namespace SourcePawn;

BaseRuntime::BaseRuntime() : m_Debug(&m_plugin), m_pPlugin(&m_plugin), m_pCtx(NULL), 
m_PubFuncs(NULL), m_PubJitFuncs(NULL), m_pCo(NULL), m_CompSerial(0)
{
	memset(&m_plugin, 0, sizeof(m_plugin));

	m_FuncCache = NULL;
	m_MaxFuncs = 0;
	m_NumFuncs = 0;
}

BaseRuntime::~BaseRuntime()
{
	for (uint32_t i = 0; i < m_pPlugin->num_publics; i++)
	{
		delete m_PubFuncs[i];
		m_PubFuncs[i] = NULL;
	}
	delete [] m_PubFuncs;
	delete [] m_PubJitFuncs;

	for (unsigned int i = 0; i < m_NumFuncs; i++)
	{
		delete m_FuncCache[i];
	}
	free(m_FuncCache);

	delete m_pCtx;
	if (m_pCo != NULL)
	{
		m_pCo->Abort();
	}

	free(m_pPlugin->base);
	delete [] m_pPlugin->memory;
	delete [] m_pPlugin->publics;
	delete [] m_pPlugin->pubvars;
	delete [] m_pPlugin->natives;
	free(m_pPlugin->name);
}

static cell_t InvalidNative(IPluginContext *pCtx, const cell_t *params)
{
	return pCtx->ThrowNativeErrorEx(SP_ERROR_INVALID_NATIVE, "Invalid native");
}

int BaseRuntime::CreateFromMemory(sp_file_hdr_t *hdr, uint8_t *base)
{
	int set_err;
	char *nameptr;
	uint8_t sectnum = 0;
	sp_plugin_t *plugin = m_pPlugin;
	sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

	memset(plugin, 0, sizeof(sp_plugin_t));

	plugin->base = base;
	plugin->base_size = hdr->imagesize;
	set_err = SP_ERROR_NONE;

	/* We have to read the name section first */
	for (sectnum = 0; sectnum < hdr->sections; sectnum++)
	{
		nameptr = (char *)(base + hdr->stringtab + secptr[sectnum].nameoffs);
		if (strcmp(nameptr, ".names") == 0)
		{
			plugin->stringbase = (const char *)(base + secptr[sectnum].dataoffs);
			break;
		}
	}

	sectnum = 0;

	/* Now read the rest of the sections */
	while (sectnum < hdr->sections)
	{
		nameptr = (char *)(base + hdr->stringtab + secptr->nameoffs);

		if (!(plugin->pcode) && !strcmp(nameptr, ".code"))
		{
			sp_file_code_t *cod = (sp_file_code_t *)(base + secptr->dataoffs);

			if (cod->codeversion < SP_CODEVERS_JIT1)
			{
				return SP_ERROR_CODE_TOO_OLD;
			}
			else if (cod->codeversion > SP_CODEVERS_JIT2)
			{
				return SP_ERROR_CODE_TOO_NEW;
			}

			plugin->pcode = base + secptr->dataoffs + cod->code;
			plugin->pcode_size = cod->codesize;
			plugin->flags = cod->flags;
			plugin->pcode_version = cod->codeversion;
		}
		else if (!(plugin->data) && !strcmp(nameptr, ".data"))
		{
			sp_file_data_t *dat = (sp_file_data_t *)(base + secptr->dataoffs);
			plugin->data = base + secptr->dataoffs + dat->data;
			plugin->data_size = dat->datasize;
			plugin->mem_size = dat->memsize;
			plugin->memory = new uint8_t[plugin->mem_size];
			memcpy(plugin->memory, plugin->data, plugin->data_size);
		}
		else if ((plugin->publics == NULL) && !strcmp(nameptr, ".publics"))
		{
			sp_file_publics_t *publics;

			publics = (sp_file_publics_t *)(base + secptr->dataoffs);
			plugin->num_publics = secptr->size / sizeof(sp_file_publics_t);

			if (plugin->num_publics > 0)
			{
				plugin->publics = new sp_public_t[plugin->num_publics];

				for (uint32_t i = 0; i < plugin->num_publics; i++)
				{
					plugin->publics[i].code_offs = publics[i].address;
					plugin->publics[i].funcid = (i << 1) | 1;
					plugin->publics[i].name = plugin->stringbase + publics[i].name;
				}
			}
		}
		else if ((plugin->pubvars == NULL) && !strcmp(nameptr, ".pubvars"))
		{
			sp_file_pubvars_t *pubvars;

			pubvars = (sp_file_pubvars_t *)(base + secptr->dataoffs);
			plugin->num_pubvars = secptr->size / sizeof(sp_file_pubvars_t);

			if (plugin->num_pubvars > 0)
			{
				plugin->pubvars = new sp_pubvar_t[plugin->num_pubvars];

				for (uint32_t i = 0; i < plugin->num_pubvars; i++)
				{
					plugin->pubvars[i].name = plugin->stringbase + pubvars[i].name;
					plugin->pubvars[i].offs = (cell_t *)(plugin->memory + pubvars[i].address);
				}
			}
		}
		else if ((plugin->natives == NULL) && !strcmp(nameptr, ".natives"))
		{
			sp_file_natives_t *natives;

			natives = (sp_file_natives_t *)(base + secptr->dataoffs);
			plugin->num_natives = secptr->size / sizeof(sp_file_natives_t);

			if (plugin->num_natives > 0)
			{
				plugin->natives = new sp_native_t[plugin->num_natives];

				for (uint32_t i = 0; i < plugin->num_natives; i++)
				{
					plugin->natives[i].flags = 0;
					plugin->natives[i].pfn = InvalidNative;
					plugin->natives[i].status = SP_NATIVE_UNBOUND;
					plugin->natives[i].user = NULL;
					plugin->natives[i].name = plugin->stringbase + natives[i].name;
				}
			}
		}
		else if (!(plugin->debug.files) && !strcmp(nameptr, ".dbg.files"))
		{
			plugin->debug.files = (sp_fdbg_file_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.lines) && !strcmp(nameptr, ".dbg.lines"))
		{
			plugin->debug.lines = (sp_fdbg_line_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.symbols) && !strcmp(nameptr, ".dbg.symbols"))
		{
			plugin->debug.symbols = (sp_fdbg_symbol_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->debug.lines_num) && !strcmp(nameptr, ".dbg.info"))
		{
			sp_fdbg_info_t *inf = (sp_fdbg_info_t *)(base + secptr->dataoffs);
			plugin->debug.files_num = inf->num_files;
			plugin->debug.lines_num = inf->num_lines;
			plugin->debug.syms_num = inf->num_syms;
		}
		else if (!(plugin->debug.stringbase) && !strcmp(nameptr, ".dbg.strings"))
		{
			plugin->debug.stringbase = (const char *)(base + secptr->dataoffs);
		}

		secptr++;
		sectnum++;
	}

	if (plugin->pcode == NULL || plugin->data == NULL)
	{
		return SP_ERROR_FILE_FORMAT;
	}

	if ((plugin->flags & SP_FLAG_DEBUG) && (!(plugin->debug.files) || !(plugin->debug.lines) || !(plugin->debug.symbols)))
	{
		return SP_ERROR_FILE_FORMAT;
	}

	if (m_pPlugin->num_publics > 0)
	{
		m_PubFuncs = new CFunction *[m_pPlugin->num_publics];
		memset(m_PubFuncs, 0, sizeof(CFunction *) * m_pPlugin->num_publics);
		m_PubJitFuncs = new JitFunction *[m_pPlugin->num_publics];
		memset(m_PubJitFuncs, 0, sizeof(JitFunction *) * m_pPlugin->num_publics);
	}

	m_pPlugin->profiler = g_engine2.GetProfiler();
	m_pCtx = new BaseContext(this);
	m_pCo = g_Jit.StartCompilation(this);

	return SP_ERROR_NONE;
}

int BaseRuntime::FindNativeByName(const char *name, uint32_t *index)
{
	int high;

	high = m_pPlugin->num_natives - 1;

	for (uint32_t i=0; i<m_pPlugin->num_natives; i++)
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
	if (index >= m_pPlugin->num_natives)
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
	return m_pPlugin->num_natives;
}

int BaseRuntime::FindPublicByName(const char *name, uint32_t *index)
{
	int diff, high, low;
	uint32_t mid;

	high = m_pPlugin->num_publics - 1;
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
	if (index >= m_pPlugin->num_publics)
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
	return m_pPlugin->num_publics;
}

int BaseRuntime::GetPubvarByIndex(uint32_t index, sp_pubvar_t **pubvar)
{
	if (index >= m_pPlugin->num_pubvars)
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

	high = m_pPlugin->num_pubvars - 1;
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
		}
		else if (diff < 0)
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}

	return SP_ERROR_NOT_FOUND;
}

int BaseRuntime::GetPubvarAddrs(uint32_t index, cell_t *local_addr, cell_t **phys_addr)
{
	if (index >= m_pPlugin->num_pubvars)
	{
		return SP_ERROR_INDEX;
	}

	*local_addr = (uint8_t *)m_pPlugin->pubvars[index].offs - m_pPlugin->memory;
	*phys_addr = m_pPlugin->pubvars[index].offs;

	return SP_ERROR_NONE;
}

uint32_t BaseRuntime::GetPubVarsNum()
{
	return m_pPlugin->num_pubvars;
}

IPluginContext *BaseRuntime::GetDefaultContext()
{
	return m_pCtx;
}

IPluginDebugInfo *BaseRuntime::GetDebugInfo()
{
	return &m_Debug;
}

IPluginFunction *BaseRuntime::GetFunctionById(funcid_t func_id)
{
	CFunction *pFunc = NULL;

	if (func_id & 1)
	{
		func_id >>= 1;
		if (func_id >= m_pPlugin->num_publics)
		{
			return NULL;
		}
		pFunc = m_PubFuncs[func_id];
		if (!pFunc)
		{
			m_PubFuncs[func_id] = new CFunction(this, 
				(func_id << 1) | 1,
				func_id);
			pFunc = m_PubFuncs[func_id];
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
			m_PubFuncs[index] = new CFunction(this, (index << 1) | 1, index);
		}
		pFunc = m_PubFuncs[index];
	}

	return pFunc;
}

bool BaseRuntime::IsDebugging()
{
	return true;
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

	return mem;
}

BaseContext *BaseRuntime::GetBaseContext()
{
	return m_pCtx;
}

int BaseRuntime::ApplyCompilationOptions(ICompilation *co)
{
	if (co == NULL)
	{
		return SP_ERROR_NONE;
	}

	m_pCo = g_Jit.ApplyOptions(m_pCo, co);
	m_pPlugin->prof_flags = ((CompData *)m_pCo)->profile;

	return SP_ERROR_NONE;
}

JitFunction *BaseRuntime::GetJittedFunction(uint32_t idx)
{
	assert(idx <= m_NumFuncs);

	if (idx == 0 || idx > m_NumFuncs)
	{
		return NULL;
	}

	return m_FuncCache[idx - 1];
}

uint32_t BaseRuntime::AddJittedFunction(JitFunction *fn)
{
	if (m_NumFuncs + 1 > m_MaxFuncs)
	{
		if (m_MaxFuncs == 0)
		{
			m_MaxFuncs = 8;
		}
		else
		{
			m_MaxFuncs *= 2;
		}

		m_FuncCache = (JitFunction **)realloc(
			m_FuncCache,
			sizeof(JitFunction *) * m_MaxFuncs);
	}

	m_FuncCache[m_NumFuncs++] = fn;

	return m_NumFuncs;
}
