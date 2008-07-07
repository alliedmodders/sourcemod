#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine2.h"
#include "x86/jit_x86.h"
#include "jit_version.h"
#include "zlib/zlib.h"
#include "BaseRuntime.h"
#include "sp_vm_engine.h"

using namespace SourcePawn;

SourcePawnEngine2::SourcePawnEngine2()
{
	m_Profiler = NULL;
}

sp_plugin_t *_ReadPlugin(sp_file_hdr_t *hdr, uint8_t *base, sp_plugin_t *plugin, int *err)
{
	char *nameptr;
	uint8_t sectnum = 0;
	sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

	memset(plugin, 0, sizeof(sp_plugin_t));

	plugin->base = base;

	while (sectnum < hdr->sections)
	{
		nameptr = (char *)(base + hdr->stringtab + secptr->nameoffs);

		if (!(plugin->pcode) && !strcmp(nameptr, ".code"))
		{
			sp_file_code_t *cod = (sp_file_code_t *)(base + secptr->dataoffs);
			plugin->pcode = base + secptr->dataoffs + cod->code;
			plugin->pcode_size = cod->codesize;
			plugin->flags = cod->flags;
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
		else if (!(plugin->info.publics) && !strcmp(nameptr, ".publics"))
		{
			plugin->info.publics_num = secptr->size / sizeof(sp_file_publics_t);
			plugin->info.publics = (sp_file_publics_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.pubvars) && !strcmp(nameptr, ".pubvars"))
		{
			plugin->info.pubvars_num = secptr->size / sizeof(sp_file_pubvars_t);
			plugin->info.pubvars = (sp_file_pubvars_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.natives) && !strcmp(nameptr, ".natives"))
		{
			plugin->info.natives_num = secptr->size / sizeof(sp_file_natives_t);
			plugin->info.natives = (sp_file_natives_t *)(base + secptr->dataoffs);
		}
		else if (!(plugin->info.stringbase) && !strcmp(nameptr, ".names"))
		{
			plugin->info.stringbase = (const char *)(base + secptr->dataoffs);
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

	if (!(plugin->pcode) || !(plugin->data) || !(plugin->info.stringbase))
	{
		goto return_error;
	}

	if ((plugin->flags & SP_FLAG_DEBUG) && (!(plugin->debug.files) || !(plugin->debug.lines) || !(plugin->debug.symbols)))
	{
		goto return_error;
	}

	if (err)
	{
		*err = SP_ERROR_NONE;
	}

	return plugin;

return_error:
	if (err)
	{
		*err = SP_ERROR_FILE_FORMAT;
	}

	return NULL;
}

IPluginRuntime *SourcePawnEngine2::LoadPlugin(ICompilation *co, const char *file, int *err)
{
	sp_file_hdr_t hdr;
	sp_plugin_t *plugin;
	uint8_t *base;
	int z_result;
	int error;

	FILE *fp = fopen(file, "rb");

	if (!fp)
	{
		error = SP_ERROR_NOT_FOUND;
		goto return_error;
	}

	/* Rewind for safety */
	fread(&hdr, sizeof(sp_file_hdr_t), 1, fp);

	if (hdr.magic != SPFILE_MAGIC)
	{
		error = SP_ERROR_FILE_FORMAT;
		goto return_error;
	}

	switch (hdr.compression)
	{
	case SPFILE_COMPRESSION_GZ:
		{
			uint32_t uncompsize = hdr.imagesize - hdr.dataoffs;
			uint32_t compsize = hdr.disksize - hdr.dataoffs;
			uint32_t sectsize = hdr.dataoffs - sizeof(sp_file_hdr_t);
			uLongf destlen = uncompsize;

			char *tempbuf = (char *)malloc(compsize);
			void *uncompdata = malloc(uncompsize);
			void *sectheader = malloc(sectsize);

			fread(sectheader, sectsize, 1, fp);
			fread(tempbuf, compsize, 1, fp);

			z_result = uncompress((Bytef *)uncompdata, &destlen, (Bytef *)tempbuf, compsize);
			free(tempbuf);
			if (z_result != Z_OK)
			{
				free(sectheader);
				free(uncompdata);
				error = SP_ERROR_DECOMPRESSOR;
				goto return_error;
			}

			base = (uint8_t *)malloc(hdr.imagesize);
			memcpy(base, &hdr, sizeof(sp_file_hdr_t));
			memcpy(base + sizeof(sp_file_hdr_t), sectheader, sectsize);
			free(sectheader);
			memcpy(base + hdr.dataoffs, uncompdata, uncompsize);
			free(uncompdata);
			break;
		}
	case SPFILE_COMPRESSION_NONE:
		{
			base = (uint8_t *)malloc(hdr.imagesize);
			rewind(fp);
			fread(base, hdr.imagesize, 1, fp);
			break;
		}
	default:
		{
			error = SP_ERROR_DECOMPRESSOR;
			goto return_error;
		}
	}

	plugin = new sp_plugin_t;

	memset(plugin, 0, sizeof(sp_plugin_t));

	if (!_ReadPlugin(&hdr, base, plugin, err))
	{
		delete plugin;
		free(base);
		return NULL;
	}

	BaseRuntime *pRuntime = new BaseRuntime(plugin);

	if (co == NULL)
	{
		co = g_Jit1.StartCompilation(pRuntime);
	}


	*err = pRuntime->ApplyCompilationOptions(co);
	if (*err != SP_ERROR_NONE)
	{
		delete pRuntime;
		return NULL;
	}

	return pRuntime;

return_error:
	*err = error;

	return NULL;
}

SPVM_NATIVE_FUNC SourcePawnEngine2::CreateFakeNative(SPVM_FAKENATIVE_FUNC callback, void *pData)
{
	return g_Jit1.CreateFakeNative(callback, pData);
}

void SourcePawnEngine2::DestroyFakeNative(SPVM_NATIVE_FUNC func)
{
	g_Jit1.DestroyFakeNative(func);
}

const char *SourcePawnEngine2::GetEngineName()
{
	return "SourcePawn 1.1, jit-x86";
}

const char *SourcePawnEngine2::GetVersionString()
{
	return SVN_FULL_VERSION;
}

IProfiler *SourcePawnEngine2::GetProfiler()
{
	return m_Profiler;
}

void SourcePawnEngine2::SetProfiler(IProfiler *profiler)
{
	m_Profiler = profiler;
}

IDebugListener *SourcePawnEngine2::SetDebugListener(IDebugListener *listener)
{
	return g_engine1.SetDebugListener(listener);
}

unsigned int SourcePawnEngine2::GetAPIVersion()
{
	return SOURCEPAWN_ENGINE2_API_VERSION;
}

ICompilation *SourcePawnEngine2::StartCompilation()
{
	return g_Jit1.StartCompilation();
}
