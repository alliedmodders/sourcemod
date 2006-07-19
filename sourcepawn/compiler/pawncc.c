#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include "memfile.h"
#include "sp_file.h"
#include "amx.h"
#include "osdefs.h"
#include "zlib/zlib.h"

enum FileSections
{
	FS_Code,			/* required */
	FS_Data,			/* required */
	FS_Publics,
	FS_Pubvars,
	FS_Natives,
	FS_Nametable,		/* required */
	FS_DbgFile,
	FS_DbgSymbol,
	FS_DbgLine,
	FS_DbgTags,
	FS_DbgAutomaton,
	FS_DbgState,
	/* --- */
	FS_Number,
};

int pc_printf(const char *message,...);
int pc_compile(int argc, char **argv);
void sfwrite(const void *buf, size_t size, size_t count, sp_file_t *spf);

memfile_t *bin_file = NULL;
jmp_buf brkout;

int main(int argc, char *argv[])
{
	if (pc_compile(argc,argv) == 0)
	{
		AMX_HEADER *hdr;
		int err;
		uint32_t i;
		sp_file_t *spf;
		uint32_t sections[FS_Number] = {1,1,0,0,0,1,0,0,0,0,0,0};

		if (bin_file == NULL)
		{
			return 0;
		}

		hdr = (AMX_HEADER *)bin_file->base;

		if ((spf=spfw_create(bin_file->name, NULL)) == NULL)
		{
			pc_printf("Error creating binary file!\n");
			memfile_destroy(bin_file);
			return 0;
		}

		if ((err=setjmp(brkout))!=0)
		{
			goto write_error;
		}

		spfw_add_section(spf, ".code");
		spfw_add_section(spf, ".data");

		sections[FS_Publics] = (hdr->natives - hdr->publics) / hdr->defsize;
		if (sections[FS_Publics])
		{
			spfw_add_section(spf, ".publics");
		}
		sections[FS_Pubvars] = (hdr->tags - hdr->pubvars) / hdr->defsize;
		if (sections[FS_Pubvars])
		{
			spfw_add_section(spf, ".pubvars");
		}
		sections[FS_Natives] = (hdr->libraries - hdr->natives) / hdr->defsize;
		if (sections[FS_Natives])
		{
			spfw_add_section(spf, ".natives");
		}

		spfw_add_section(spf, ".names");

		spfw_finalize_header(spf);
		
		/** 
		 * Begin writing each of our known tables out
		 */
		
		if (sections[FS_Code])
		{
			sp_file_code_t cod;
			cell cip;
			unsigned char *cbase;
			uint8_t *tbase, *tptr;
			cell real_codesize;
			uint8_t op;

			cod.cellsize = sizeof(cell);

			cod.codesize = 0;
			cod.codeversion = hdr->amx_version;
			cod.flags = 0;
			if (hdr->flags & AMX_FLAG_DEBUG)
			{
				cod.flags |= SP_FILE_DEBUG;
			}
			cod.code = sizeof(cod);
			cod.main = hdr->cip;

			/* write the code in our newer format */
			cbase = (unsigned char *)hdr + hdr->cod;
			real_codesize = hdr->dat - hdr->cod;
			tbase = (uint8_t *)malloc(real_codesize);
			tptr = tbase;
			for (cip = 0; cip < real_codesize;)
			{
#define DBGPARAM(v)     ( (v)=*(cell *)(cbase+(int)cip), cip+=sizeof(cell) )
				op=(uint8_t) *(ucell *)(cbase+(int)cip);
				cip += sizeof(cell);
				*tptr++ = op;
				switch (op)
				{
				case OP_PUSH5_C:    /* instructions with 5 parameters */
				case OP_PUSH5:
				case OP_PUSH5_S:
				case OP_PUSH5_ADR:
					{
						memcpy(tptr, cbase+(int)cip, sizeof(cell)*5);
						cip += sizeof(cell)*5;
						tptr += sizeof(cell)*5;
						break;
					}
				case OP_PUSH4_C:    /* instructions with 4 parameters */
				case OP_PUSH4:
				case OP_PUSH4_S:
				case OP_PUSH4_ADR:
					{
						memcpy(tptr, cbase+(int)cip, sizeof(cell)*4);
						cip += sizeof(cell)*4;
						tptr += sizeof(cell)*4;
						break;
					}
				case OP_PUSH3_C:    /* instructions with 3 parameters */
				case OP_PUSH3:
				case OP_PUSH3_S:
				case OP_PUSH3_ADR:
					{
						memcpy(tptr, cbase+(int)cip, sizeof(cell)*3);
						cip += sizeof(cell)*3;
						tptr += sizeof(cell)*3;
						break;
					}
				case OP_PUSH2_C:    /* instructions with 2 parameters */
				case OP_PUSH2:
				case OP_PUSH2_S:
				case OP_PUSH2_ADR:
				case OP_LOAD_BOTH:
				case OP_LOAD_S_BOTH:
				case OP_CONST:
				case OP_CONST_S:
				case OP_SYSREQ_N:
					{
						memcpy(tptr, cbase+(int)cip, sizeof(cell)*2);
						cip += sizeof(cell)*2;
						tptr += sizeof(cell)*2;
						break;
					}
				case OP_LOAD_PRI:   /* instructions with 1 parameter */
				case OP_LOAD_ALT:
				case OP_LOAD_S_PRI:
				case OP_LOAD_S_ALT:
				case OP_LREF_PRI:
				case OP_LREF_ALT:
				case OP_LREF_S_PRI:
				case OP_LREF_S_ALT:
				case OP_LODB_I:
				case OP_CONST_PRI:
				case OP_CONST_ALT:
				case OP_ADDR_PRI:
				case OP_ADDR_ALT:
				case OP_STOR_PRI:
				case OP_STOR_ALT:
				case OP_STOR_S_PRI:
				case OP_STOR_S_ALT:
				case OP_SREF_PRI:
				case OP_SREF_ALT:
				case OP_SREF_S_PRI:
				case OP_SREF_S_ALT:
				case OP_STRB_I:
				case OP_LIDX_B:
				case OP_IDXADDR_B:
				case OP_ALIGN_PRI:
				case OP_ALIGN_ALT:
				case OP_LCTRL:
				case OP_SCTRL:
				case OP_PUSH_R:
				case OP_PUSH_C:
				case OP_PUSH:
				case OP_PUSH_S:
				case OP_STACK:
				case OP_HEAP:
				case OP_JREL:
				case OP_SHL_C_PRI:
				case OP_SHL_C_ALT:
				case OP_SHR_C_PRI:
				case OP_SHR_C_ALT:
				case OP_ADD_C:
				case OP_SMUL_C:
				case OP_ZERO:
				case OP_ZERO_S:
				case OP_EQ_C_PRI:
				case OP_EQ_C_ALT:
				case OP_INC:
				case OP_INC_S:
				case OP_DEC:
				case OP_DEC_S:
				case OP_MOVS:
				case OP_CMPS:
				case OP_FILL:
				case OP_HALT:
				case OP_BOUNDS:
				case OP_PUSH_ADR:
				case OP_CALL:				   /* opcodes that need relocation */
				case OP_JUMP:
				case OP_JZER:
				case OP_JNZ:
				case OP_JEQ:
				case OP_JNEQ:
				case OP_JLESS:
				case OP_JLEQ:
				case OP_JGRTR:
				case OP_JGEQ:
				case OP_JSLESS:
				case OP_JSLEQ:
				case OP_JSGRTR:
				case OP_JSGEQ:
				case OP_SWITCH:
				case OP_SYSREQ_C:
					{
						*(cell *)tptr = *(cell *)(cbase + (int)cip);
						cip += sizeof(cell);
						tptr += sizeof(cell);
						break;
					}
				case OP_LOAD_I:				 /* instructions without parameters */
				case OP_STOR_I:
				case OP_LIDX:
				case OP_IDXADDR:
				case OP_MOVE_PRI:
				case OP_MOVE_ALT:
				case OP_XCHG:
				case OP_PUSH_PRI:
				case OP_PUSH_ALT:
				case OP_POP_PRI:
				case OP_POP_ALT:
				case OP_PROC:
				case OP_RET:
				case OP_RETN:
				case OP_CALL_PRI:
				case OP_SHL:
				case OP_SHR:
				case OP_SSHR:
				case OP_SMUL:
				case OP_SDIV:
				case OP_SDIV_ALT:
				case OP_UMUL:
				case OP_UDIV:
				case OP_UDIV_ALT:
				case OP_ADD:
				case OP_SUB:
				case OP_SUB_ALT:
				case OP_AND:
				case OP_OR:
				case OP_XOR:
				case OP_NOT:
				case OP_NEG:
				case OP_INVERT:
				case OP_ZERO_PRI:
				case OP_ZERO_ALT:
				case OP_SIGN_PRI:
				case OP_SIGN_ALT:
				case OP_EQ:
				case OP_NEQ:
				case OP_LESS:
				case OP_LEQ:
				case OP_GRTR:
				case OP_GEQ:
				case OP_SLESS:
				case OP_SLEQ:
				case OP_SGRTR:
				case OP_SGEQ:
				case OP_INC_PRI:
				case OP_INC_ALT:
				case OP_INC_I:
				case OP_DEC_PRI:
				case OP_DEC_ALT:
				case OP_DEC_I:
				case OP_SYSREQ_PRI:
				case OP_JUMP_PRI:
				case OP_SWAP_PRI:
				case OP_SWAP_ALT:
				case OP_NOP:
				case OP_BREAK:
					break;
				case OP_CASETBL:
					{
						cell num;
						DBGPARAM(*(cell *)tptr);
						num = *(cell *)tptr;
						tptr += sizeof(cell);
						memcpy(tptr, cbase+(int)cip, (2*num+1)*sizeof(cell));
						tptr += (2*num+1) * sizeof(cell);
						cip += (2*num+1) * sizeof(cell);
						break;
					}
				default:
					{
						assert(0);
					}
#undef DBGPARAM
				}
			}
			cod.codesize = (uint32_t)(tptr - tbase);
			cod.disksize = cod.codesize;
			cod.compression = SPFILE_COMPRESSION_NONE;
			sfwrite(&cod, sizeof(cod), 1, spf);
			sfwrite(tbase, cod.codesize, 1, spf);
			free(tbase);

			spfw_next_section(spf);
		}

		if (sections[FS_Data])
		{
			sp_file_data_t dat;
			unsigned char *dbase;
			Bytef *cmp_dbase;
			uLong disksize;
			int err;

			dat.datasize = hdr->hea - hdr->dat;
			dat.memsize = hdr->stp;
			dat.data = sizeof(dat);
			dat.compression = SPFILE_COMPRESSION_GZ;

			if (dat.datasize)
			{
				dat.disksize = (uint32_t)compressBound((uLong)dat.datasize);

				dbase = (unsigned char *)hdr + hdr->dat;
				cmp_dbase = (Bytef *)malloc(dat.disksize);

				/* compress */
				err = compress2(cmp_dbase, &disksize, (Bytef *)dbase, (uLong)dat.datasize, Z_BEST_COMPRESSION);

				if (err != Z_OK)
				{
					pc_printf("Failed to compress DAT section with error: %d\n", err);
					pc_printf("Defaulting to no compression.\n");
					dat.compression = SPFILE_COMPRESSION_NONE;
					dat.disksize = dat.datasize;

					/* write header */
					sfwrite(&dat, sizeof(dat), 1, spf);
					/* write data */
					sfwrite(&dbase, dat.datasize, 1, spf);
				} else {
					dat.disksize = (uint32_t)disksize;

					/* write header */
					sfwrite(&dat, sizeof(dat), 1, spf);
					/* write data */
					sfwrite(cmp_dbase, dat.disksize, 1, spf);
				}
				
				free(cmp_dbase);
			} else {
				/* should be 0 */
				dat.disksize = dat.datasize;
				sfwrite(&dat, sizeof(dat), 1, spf);
			}

			spfw_next_section(spf);
		}

		if (sections[FS_Publics])
		{
			sp_file_publics_t *pbtbl;
			AMX_FUNCSTUBNT *stub;
			uint32_t publics = sections[FS_Publics];

			pbtbl = (sp_file_publics_t *)malloc(sizeof(sp_file_publics_t) * publics);
			stub = (AMX_FUNCSTUBNT *)((unsigned char *)hdr + hdr->publics);

			for (i=0; i<publics; i++)
			{
				pbtbl[i].address = stub->address;
				pbtbl[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

				stub += hdr->defsize;
			}
			if (publics)
			{
				sfwrite(pbtbl, sizeof(sp_file_publics_t), publics, spf);
			}
			free(pbtbl);

			spfw_next_section(spf);
		}

		if (sections[FS_Pubvars])
		{
			sp_file_pubvars_t *pbvars;
			AMX_FUNCSTUBNT *stub;
			uint32_t pubvars = sections[FS_Pubvars];

			pbvars = (sp_file_pubvars_t *)malloc(sizeof(sp_file_pubvars_t) * pubvars);
			stub = (AMX_FUNCSTUBNT *)((unsigned char *)hdr + hdr->pubvars);

			for (i=0; i<pubvars; i++)
			{
				pbvars[i].address = stub->address;
				pbvars[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

				stub += hdr->defsize;
			}
			if (pubvars)
			{
				sfwrite(pbvars, sizeof(sp_file_pubvars_t), pubvars, spf);
			}
			free(pbvars);
			spfw_next_section(spf);
		}

		if (sections[FS_Natives])
		{
			sp_file_natives_t *nvtbl;
			AMX_FUNCSTUBNT *stub;
			uint32_t natives = (hdr->libraries - hdr->natives) / hdr->defsize;

			nvtbl = (sp_file_natives_t *)malloc(sizeof(sp_file_natives_t) * natives);
			stub = (AMX_FUNCSTUBNT *)((unsigned char *)hdr + hdr->natives);

			for (i=0; i<natives; i++)
			{
				nvtbl[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

				stub += hdr->defsize;
			}
			if (natives)
			{
				sfwrite(nvtbl, sizeof(sp_file_natives_t), natives, spf);
			}
			free(nvtbl);
			spfw_next_section(spf);
		}

		if (sections[FS_Nametable])
		{
			unsigned char *base;
			uint32_t namelen;

			/* write the entire block */
			base = (unsigned char *)hdr + hdr->nametable + sizeof(uint16_t);
			/**
			 * note - the name table will be padded to sizeof(cell) bytes.
			 * this may clip at most an extra three bytes in!
			 */
			namelen = hdr->cod - hdr->nametable;
			sfwrite(base, namelen, 1, spf);
			spfw_next_section(spf);
		}

		spfw_finalize_all(spf);
		spfw_destroy(spf);

		return 0;

write_error:
		pc_printf("Error writing to file: %s", bin_file->name);
		unlink(bin_file->name);

		memfile_destroy(bin_file);
		bin_file = NULL;

		return 1;
	}

	return 1;
}

void sfwrite(const void *buf, size_t size, size_t count, sp_file_t *spf)
{
	if (spf->funcs.fnWrite(buf, size, count, spf->handle) != count)
	{
		longjmp(brkout, 1);
	}
}
