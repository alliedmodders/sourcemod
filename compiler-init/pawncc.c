#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include "memfile.h"
#include "sp_file.h"
#include "amx.h"
#include "osdefs.h"

#define NUM_SECTIONS	6

int pc_printf(const char *message,...);
int pc_compile(int argc, char **argv);
void sfwrite(const void *buf, size_t size, size_t count, FILE *fp);

memfile_t *bin_file = NULL;
jmp_buf brkout;

int main(int argc, char *argv[])
{
	if (pc_compile(argc,argv) == 0)
	{
		FILE *fp;
		AMX_HEADER *hdr;
		sp_file_hdr_t shdr;
		uint32_t curoffs = 0;
		uint32_t lastsection = 0;
		int err;
		uint8_t i8;
		uint32_t i;
		const char *tables[NUM_SECTIONS] = {".code", ".data", ".publics", ".pubvars", ".natives", ".names"};
		uint32_t offsets[NUM_SECTIONS] = {0,0,0,0,0,0};
		sp_file_section_t sh;

		if (bin_file == NULL)
		{
			return 0;
		}

		hdr = (AMX_HEADER *)bin_file->base;
		shdr.version = SPFILE_VERSION;
		shdr.magic = SPFILE_MAGIC;

		if ((fp=fopen(bin_file->name, "wb")) == NULL)
		{
			pc_printf("Error writing to file: %s", bin_file->name);
			return 1;
		}

		if ((err=setjmp(brkout))!=0)
		{
			goto write_error;
		}

		shdr.sections = NUM_SECTIONS;
		shdr.stringtab = sizeof(shdr) + (sizeof(sp_file_section_t) * shdr.sections);

		/**
		 * write the header
		 * unwritten values:
		 *  imagesize
		 */
		sfwrite(&shdr, sizeof(shdr), 1, fp);

		curoffs = shdr.stringtab;

		/**
		 * write the sections 
		 * unwritten values:
		 *  dataoffs
		 *  size
		 */
		for (i8=0; i8<shdr.sections; i8++)
		{
			/* set name offset to next in string table */
			sh.nameoffs = curoffs - shdr.stringtab;
			/* save offset to this section */
			offsets[i8] = (uint32_t)ftell(fp) + sizeof(sh.nameoffs);
			/* update `end of file` offset */
			curoffs += strlen(tables[i8]) + 1;
			sfwrite(&sh, sizeof(sh), 1, fp);
		}

		/** write the string table */
		for (i8=0; i8<shdr.sections; i8++)
		{
			sfwrite(tables[i8], 1, strlen(tables[i8])+1, fp);
		}

		lastsection = curoffs;

		/** write the code table */
		if (strcmp(tables[0], ".code") == 0)
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
						int i;
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
			sfwrite(&cod, sizeof(cod), 1, fp);
			sfwrite(tbase, cod.codesize, 1, fp);
			free(tbase);

			/* backtrack and write this section's header info */
			curoffs = (uint32_t)ftell(fp);
			fseek(fp, offsets[0], SEEK_SET);
			sfwrite(&lastsection, sizeof(lastsection), 1, fp);
			cod.codesize += sizeof(cod);
			sfwrite(&cod.codesize, sizeof(cod.codesize), 1, fp);
			fseek(fp, curoffs, SEEK_SET);
			lastsection = curoffs;
		}

		if (strcmp(tables[1], ".data") == 0)
		{
			sp_file_data_t dat;
			unsigned char *dbase;

			dat.datasize = hdr->hea - hdr->dat;
			dat.memsize = hdr->stp;
			dat.data = sizeof(dat);
			dbase = (unsigned char *)hdr + hdr->dat;

			sfwrite(&dat, sizeof(dat), 1, fp);
			if (dat.datasize)
			{
				sfwrite(dbase, dat.datasize, 1, fp);
			}

			/* backtrack and write this section's header info */
			curoffs = ftell(fp);
			fseek(fp, offsets[1], SEEK_SET);
			sfwrite(&lastsection, sizeof(lastsection), 1, fp);
			dat.datasize += sizeof(dat);
			sfwrite(&dat.datasize, sizeof(dat.datasize),1, fp);
			fseek(fp, curoffs, SEEK_SET);
			lastsection = curoffs;
		}

		fclose(fp);

		return 0;

write_error:
		pc_printf("Error writing to file: %s", bin_file->name);
		unlink(bin_file->name);
		fclose(fp);

		return 1;
	}

	return 1;
}

void sfwrite(const void *buf, size_t size, size_t count, FILE *fp)
{
	if (fwrite(buf, size, count, fp) != count)
	{
		longjmp(brkout, 1);
	}
}
