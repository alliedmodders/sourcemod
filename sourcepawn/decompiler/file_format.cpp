#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zlib.h"
#include "file_format.h"

void Sp_FreePlugin(sp_file_t *plugin)
{
	if (plugin == NULL)
	{
		return;
	}

	free(plugin->base);
	delete [] plugin->publics;
	delete [] plugin->pubvars;
	delete [] plugin->natives;
	delete plugin;
}

sp_file_t *Sp_ReadPlugin(const char *file, int *err)
{
	sp_file_t *plugin;
	sp_file_hdr_t hdr;
	uint8_t *base;
	int z_result;
	int error;

	FILE *fp = fopen(file, "rb");

	plugin = NULL;

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

	plugin = new sp_file_t;
	memset(plugin, 0, sizeof(sp_file_t));
	plugin->base = base;

	{
		int set_err;
		char *nameptr;
		uint8_t sectnum = 0;
		sp_file_section_t *secptr = (sp_file_section_t *)(base + sizeof(sp_file_hdr_t));

		plugin->base_size = hdr.imagesize;
		set_err = SP_ERROR_NONE;

		/* We have to read the name section first */
		for (sectnum = 0; sectnum < hdr.sections; sectnum++)
		{
			nameptr = (char *)(base + hdr.stringtab + secptr[sectnum].nameoffs);
			if (strcmp(nameptr, ".names") == 0)
			{
				plugin->stringbase = (const char *)(base + secptr[sectnum].dataoffs);
				break;
			}
		}

		sectnum = 0;

		/* Now read the rest of the sections */
		while (sectnum < hdr.sections)
		{
			nameptr = (char *)(base + hdr.stringtab + secptr->nameoffs);

			if (!(plugin->pcode) && !strcmp(nameptr, ".code"))
			{
				sp_file_code_t *cod = (sp_file_code_t *)(base + secptr->dataoffs);

				if (cod->codeversion < SP_CODEVERS_JIT1)
				{
					error = SP_ERROR_CODE_TOO_OLD;
					goto return_error;
				}
				else if (cod->codeversion > SP_CODEVERS_JIT2)
				{
					error = SP_ERROR_CODE_TOO_NEW;
					goto return_error;
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
						plugin->pubvars[i].offs = (cell_t *)(plugin->data + pubvars[i].address);
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
						plugin->natives[i].pfn = NULL;
						plugin->natives[i].status = SP_NATIVE_UNBOUND;
						plugin->natives[i].user = NULL;
						plugin->natives[i].name = plugin->stringbase + natives[i].name;
					}
				}
			}
			else if ((plugin->tags == NULL) && !strcmp(nameptr, ".tags"))
			{
				sp_file_tag_s *tags;

				tags = (sp_file_tag_s *)(base + secptr->dataoffs);
				plugin->num_tags = secptr->size / sizeof(sp_file_tag_s);

				if (plugin->num_tags > 0)
				{
					plugin->tags = new sp_tag_t[plugin->num_tags];

					for (uint32_t i = 0; i < plugin->num_tags; i++)
					{
						plugin->tags[i].id = tags[i].tag_id;
						plugin->tags[i].name = plugin->stringbase + tags[i].name;
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
			else if (plugin->debug.ntv == NULL && !strcmp(nameptr, ".dbg.natives"))
			{
				plugin->debug.ntv = (sp_fdbg_ntvtab_s *)(base + secptr->dataoffs);
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
			error = SP_ERROR_FILE_FORMAT;
			goto return_error;
		}

		if ((plugin->flags & SP_FLAG_DEBUG) && (!(plugin->debug.files) || !(plugin->debug.lines) || !(plugin->debug.symbols)))
		{
			error = SP_ERROR_FILE_FORMAT;
			goto return_error;
		}
	}

	fclose(fp);

	return plugin;

return_error:
	Sp_FreePlugin(plugin);
	*err = error;
	fclose(fp);

	return NULL;
}

sp_fdbg_symbol_t *Sp_FindFunctionSym(sp_file_t *plugin, ucell_t addr)
{
	uint32_t iter;
	sp_fdbg_symbol_t *sym;
	uint8_t *cursor = (uint8_t *)(plugin->debug.symbols);

	for (iter = 0; iter < plugin->debug.syms_num; iter++)
	{
		sym = (sp_fdbg_symbol_t *)cursor;

		if (sym->ident == SP_SYM_FUNCTION
			&& sym->codestart <= addr
			&& sym->codeend > addr)
		{
			return sym;
		}

		cursor += sizeof(sp_fdbg_symbol_t);

		if (sym->dimcount > 0)
		{
			cursor += sizeof(sp_fdbg_arraydim_t) * sym->dimcount;
		}
	}

	return NULL;
}

sp_tag_t *Sp_FindTag(sp_file_t *plugin, uint32_t tag_id)
{
	for (uint32_t i = 0; i < plugin->num_tags; i++)
	{
		if ((plugin->tags[i].id & 0xFFFF) == tag_id)
		{
			return &plugin->tags[i];
		}
	}

	return NULL;
}
