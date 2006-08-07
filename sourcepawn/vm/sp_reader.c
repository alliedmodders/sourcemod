#include <malloc.h>
#include <string.h>
#include "sp_vm.h"
#include "zlib/zlib.h"

sp_plugin_t *_ReadPlugin(sp_file_hdr_t *hdr, uint8_t *base, sp_plugin_t *plugin, int *err)
{
	char *nameptr;
	uint8_t sectnum = 0;
	sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

	memset(plugin, 0, sizeof(sp_plugin_t));

	plugin->base = base;

	while (sectnum < hdr->sections)
	{
		nameptr = base + hdr->stringtab + secptr->nameoffs;

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
			plugin->memory = dat->memsize;
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
			plugin->info.stringbase = base + secptr->dataoffs;
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
			plugin->debug.stringbase = base + secptr->dataoffs;
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
		*err = SP_ERR_NONE;
	}

	return plugin;

return_error:
	if (err)
	{
		*err = SP_ERR_FILE_FORMAT;
	}

	return NULL;
}

sp_plugin_t *SP_LoadFromFilePointer(FILE *fp, int *err)
{
	sp_file_hdr_t hdr;
	sp_plugin_t *plugin;
	uint8_t *base;
	int z_result;
	int error;

	if (!fp)
	{
		error = SP_ERR_NOT_FOUND;
		goto return_error;
	}

	/* Rewind for safety */
	rewind(fp);
	fread(&hdr, sizeof(sp_file_hdr_t), 1, fp);

	if (hdr.magic != SPFILE_MAGIC)
	{
		error = SP_ERR_FILE_FORMAT;
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
				error = SP_ERR_DECOMPRESSOR;
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
			error = SP_ERR_DECOMPRESSOR;
			goto return_error;
		}
	}

	plugin = (sp_plugin_t *)malloc(sizeof(sp_plugin_t));
	if (!_ReadPlugin(&hdr, base, plugin, err))
	{
		free(plugin);
		free(base);
		return NULL;
	}

	return plugin;

return_error:
	if (err)
	{
		*err = error;
	}
	return NULL;
}

sp_plugin_t *SP_LoadFromMemory(void *base, sp_plugin_t *plugin, int *err)
{
	sp_file_hdr_t hdr;
	uint8_t noptr = 0;

	memcpy(&hdr, base, sizeof(sp_file_hdr_t));

	if (!plugin)
	{
		plugin = (sp_plugin_t *)malloc(sizeof(sp_plugin_t));
		noptr = 1;
	}

	if (!_ReadPlugin(&hdr, base, plugin, err))
	{
		if (noptr)
		{
			free(plugin);
		}
		return NULL;
	}

	return plugin;
}
