#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include "memfile.h"
#include "sp_file.h"
#include "amx.h"
#include "amxdbg.h"
#include "osdefs.h"
#include "zlib/zlib.h"
#if defined LINUX || defined DARWIN
#include <unistd.h>
#elif defined WIN32
#include <io.h>
#endif

enum FileSections
{
  FS_Code,      /* required */
  FS_Data,      /* required */
  FS_Publics,
  FS_Pubvars,
  FS_Natives,
  FS_Nametable,    /* required */
  FS_DbgFile,
  FS_DbgSymbol,
  FS_DbgLine,
  FS_DbgTags,
  FS_DbgNatives,
  FS_DbgAutomaton,
  FS_DbgState,
  FS_DbgStrings,
  FS_DbgInfo,
  FS_Tags,
  /* --- */
  FS_Number,
};

int pc_printf(const char *message,...);
int pc_compile(int argc, char **argv);
void sfwrite(const void *buf, size_t size, size_t count, sp_file_t *spf);

memfile_t *bin_file = NULL;
jmp_buf brkout;

#define sARGS_MAX    32  /* number of arguments a function can have, max */
#define sDIMEN_MAX     4    /* maximum number of array dimensions */

typedef struct t_arg_s
{
  uint8_t ident;
  int16_t tag;
  char *name;
  uint16_t dimcount;
  sp_fdbg_arraydim_t dims[sDIMEN_MAX];
} t_arg;

typedef struct t_native_s
{
  char *name;
  int16_t ret_tag;
  uint16_t num_args;
  t_arg args[sARGS_MAX];
} t_native;

t_native *native_list = NULL;

int main(int argc, char *argv[])
{
  if (pc_compile(argc,argv) != 0)
    return 1;

  AMX_HEADER *hdr;
  AMX_DBG_HDR *dbg = NULL;
  int err;
  uint32_t i;
  sp_file_t *spf;
  memfile_t *dbgtab = NULL;    //dbgcrab
  unsigned char *dbgptr = NULL;
  uint32_t sections[FS_Number] = {1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0};
  FILE *fp;

  if (bin_file == NULL)
    return 0;

  hdr = (AMX_HEADER *)bin_file->base;

  if ((spf=spfw_create(bin_file->name, NULL)) == NULL) {
    pc_printf("Error creating binary file!\n");
    memfile_destroy(bin_file);
    return 0;
  }

  if ((err=setjmp(brkout))!=0)
    goto write_error;

  spfw_add_section(spf, ".code");
  spfw_add_section(spf, ".data");

  sections[FS_Publics] = (hdr->natives - hdr->publics) / hdr->defsize;
  if (sections[FS_Publics])
    spfw_add_section(spf, ".publics");
  sections[FS_Pubvars] = (hdr->tags - hdr->pubvars) / hdr->defsize;
  if (sections[FS_Pubvars])
    spfw_add_section(spf, ".pubvars");
  sections[FS_Natives] = (hdr->libraries - hdr->natives) / hdr->defsize;
  if (sections[FS_Natives])
    spfw_add_section(spf, ".natives");
  sections[FS_Tags] = (hdr->nametable - hdr->tags) / hdr->defsize;
  if (sections[FS_Tags])
    spfw_add_section(spf, ".tags");

  spfw_add_section(spf, ".names");

  if (hdr->flags & AMX_FLAG_DEBUG) {
    dbg = (AMX_DBG_HDR *)((unsigned char *)hdr + hdr->size);
    if (dbg->magic != AMX_DBG_MAGIC) {
      pc_printf("Error reading AMX_DBG_HDR, debug data will not be written.");
    } else {
      dbgtab = memfile_creat("", 512);
      dbgptr = (unsigned char *)dbg + sizeof(AMX_DBG_HDR);
      if ((sections[FS_DbgNatives] = sections[FS_Natives]) > 0)
        spfw_add_section(spf, ".dbg.natives");
      if (dbg->files) {
        spfw_add_section(spf, ".dbg.files");
        sections[FS_DbgFile] = dbg->files;
      }
      if (dbg->lines) {
        spfw_add_section(spf, ".dbg.lines");
        sections[FS_DbgLine] = dbg->lines;
      }
      if (dbg->symbols) {
        spfw_add_section(spf, ".dbg.symbols");
        sections[FS_DbgSymbol] = dbg->symbols;
      }
      sections[FS_DbgInfo] = 1;
      sections[FS_DbgStrings] = 1;
      spfw_add_section(spf, ".dbg.info");
      spfw_add_section(spf, ".dbg.strings");
    }
  }

  spfw_finalize_header(spf);
  
  /** 
   * Begin writing each of our known tables out
   */
  
  if (sections[FS_Code]) {
    sp_file_code_t cod;
    unsigned char *cbase;

    cod.cellsize = sizeof(cell);

    cod.codesize = hdr->dat - hdr->cod;
    cod.codeversion = hdr->amx_version;
    cod.flags = 0;
    if (hdr->flags & AMX_FLAG_DEBUG)
    {
      cod.flags |= SP_FLAG_DEBUG;
    }
    cod.code = sizeof(cod);
    cod.main = hdr->cip;

    /* write the code */
    cbase = (unsigned char *)hdr + hdr->cod;
    sfwrite(&cod, sizeof(cod), 1, spf);
    sfwrite(cbase, cod.codesize, 1, spf);

    spfw_next_section(spf);
  }

  if (sections[FS_Data]) {
    sp_file_data_t dat;
    unsigned char *dbase = (unsigned char *)hdr + hdr->dat;

    dat.datasize = hdr->hea - hdr->dat;
    dat.memsize = hdr->stp;
    dat.data = sizeof(dat);

    /* write header */
    sfwrite(&dat, sizeof(dat), 1, spf);

    if (dat.datasize) {
      /* write data */
      sfwrite(dbase, dat.datasize, 1, spf);
    }

    spfw_next_section(spf);
  }

  if (sections[FS_Publics]) {
    sp_file_publics_t *pbtbl;
    AMX_FUNCSTUBNT *stub;
    unsigned char *stubptr;
    uint32_t publics = sections[FS_Publics];

    pbtbl = (sp_file_publics_t *)malloc(sizeof(sp_file_publics_t) * publics);
    stubptr = (unsigned char *)hdr + hdr->publics;

    for (i=0; i<publics; i++) {
      stub = (AMX_FUNCSTUBNT *)stubptr;
      pbtbl[i].address = stub->address;
      pbtbl[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

      stubptr += hdr->defsize;
    }
    if (publics)
      sfwrite(pbtbl, sizeof(sp_file_publics_t), publics, spf);
    free(pbtbl);

    spfw_next_section(spf);
  }

  if (sections[FS_Pubvars]) {
    sp_file_pubvars_t *pbvars;
    AMX_FUNCSTUBNT *stub;
    unsigned char *stubptr;
    uint32_t pubvars = sections[FS_Pubvars];

    pbvars = (sp_file_pubvars_t *)malloc(sizeof(sp_file_pubvars_t) * pubvars);
    stubptr = (unsigned char *)hdr + hdr->pubvars;

    for (i=0; i<pubvars; i++) {
      stub = (AMX_FUNCSTUBNT *)stubptr;
      pbvars[i].address = stub->address;
      pbvars[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

      stubptr += hdr->defsize;
    }
    if (pubvars)
      sfwrite(pbvars, sizeof(sp_file_pubvars_t), pubvars, spf);
    free(pbvars);
    spfw_next_section(spf);
  }

  if (sections[FS_Natives]) {
    sp_file_natives_t *nvtbl;
    AMX_FUNCSTUBNT *stub;
    unsigned char *stubptr;
    uint32_t natives = (hdr->libraries - hdr->natives) / hdr->defsize;

    nvtbl = (sp_file_natives_t *)malloc(sizeof(sp_file_natives_t) * natives);
    stubptr = (unsigned char *)hdr + hdr->natives;

    for (i=0; i<natives; i++) {
      stub = (AMX_FUNCSTUBNT *)stubptr;
      nvtbl[i].name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));

      stubptr += hdr->defsize;
    }
    if (natives) {
      sfwrite(nvtbl, sizeof(sp_file_natives_t), natives, spf);
    }
    free(nvtbl);
    spfw_next_section(spf);
  }

  if (sections[FS_Tags]) {
    uint32_t numTags = (uint32_t)sections[FS_Tags];
    AMX_FUNCSTUBNT *stub;
    sp_file_tag_t tag;

    for (i=0; i<numTags; i++) {
      stub = (AMX_FUNCSTUBNT *)((unsigned char *)hdr + hdr->tags + (i * hdr->defsize));
      tag.tag_id = stub->address;
      tag.name = stub->nameofs - (hdr->nametable + sizeof(uint16_t));
      sfwrite(&tag, sizeof(sp_file_tag_t), 1, spf);
    }
    spfw_next_section(spf);
  }

  if (sections[FS_Nametable]) {
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

  if (hdr->flags & AMX_FLAG_DEBUG) {
    sp_fdbg_info_t info;

    memset(&info, 0, sizeof(sp_fdbg_info_t));

    if (sections[FS_Natives]) {
      uint16_t j;
      uint32_t idx;
      uint32_t name;
      uint32_t natives = (hdr->libraries - hdr->natives) / hdr->defsize;

      sfwrite(&natives, sizeof(uint32_t), 1, spf);
      for (idx=0; idx<natives; idx++) {
        sfwrite(&idx, sizeof(uint32_t), 1, spf);
        name = (uint32_t)memfile_tell(dbgtab);
        memfile_write(dbgtab, native_list[idx].name, strlen(native_list[idx].name) + 1);
        sfwrite(&name, sizeof(uint32_t), 1, spf);
        sfwrite(&native_list[idx].ret_tag, sizeof(int16_t), 1, spf);
        sfwrite(&native_list[idx].num_args, sizeof(uint16_t), 1, spf);

        /* Go through arguments */
        for (j = 0; j < native_list[idx].num_args; j++) {
          sfwrite(&native_list[idx].args[j].ident, sizeof(uint8_t), 1, spf);
          sfwrite(&native_list[idx].args[j].tag, sizeof(int16_t), 1, spf);
          sfwrite(&native_list[idx].args[j].dimcount, sizeof(uint16_t), 1, spf);
          name = (uint32_t)memfile_tell(dbgtab);
          sfwrite(&name, sizeof(uint32_t), 1, spf);
          memfile_write(dbgtab, 
            native_list[idx].args[j].name, 
            strlen(native_list[idx].args[j].name) + 1);
          if (native_list[idx].args[j].dimcount) {
            sfwrite(native_list[idx].args[j].dims,
              sizeof(sp_fdbg_arraydim_t),
              native_list[idx].args[j].dimcount,
              spf);
          }
          free(native_list[idx].args[j].name);
        }
        free(native_list[idx].name);
      }
      free(native_list);
      spfw_next_section(spf);
    }

    if (sections[FS_DbgFile]) {
      uint32_t idx;
      sp_fdbg_file_t dbgfile;
      AMX_DBG_FILE *_ptr;
      uint32_t len;
      for (idx=0; idx<sections[FS_DbgFile]; idx++) {
        /* get entry info */
        _ptr = (AMX_DBG_FILE *)dbgptr;
        len = strlen(_ptr->name);
        /* store */
        dbgfile.addr = _ptr->address;
        dbgfile.name = (uint32_t)memfile_tell(dbgtab);
        sfwrite(&dbgfile, sizeof(sp_fdbg_file_t), 1, spf);
        /* write to tab, then move to next */
        memfile_write(dbgtab, _ptr->name, len + 1);
        dbgptr += sizeof(AMX_DBG_FILE) + len;
        info.num_files++;
      }
      spfw_next_section(spf);
    }

    if (sections[FS_DbgLine]) {
      uint32_t idx;
      AMX_DBG_LINE *line;
      sp_fdbg_line_t dbgline;
      for (idx=0; idx<sections[FS_DbgLine]; idx++) {
        /* get entry info */
        line = (AMX_DBG_LINE *)dbgptr;
        /* store */
        dbgline.addr = (uint32_t)line->address;
        dbgline.line = (uint32_t)line->line;
        sfwrite(&dbgline, sizeof(sp_fdbg_line_t), 1, spf);
        /* move to next */
        dbgptr += sizeof(AMX_DBG_LINE);
        info.num_lines++;
      }
      spfw_next_section(spf);
    }

    if (sections[FS_DbgSymbol]) {
      uint32_t idx;
      uint32_t dnum;
      AMX_DBG_SYMBOL *sym;
      AMX_DBG_SYMDIM *dim;
      sp_fdbg_symbol_t dbgsym;
      sp_fdbg_arraydim_t dbgdim;
      uint32_t len;

      for (idx=0; idx<sections[FS_DbgSymbol]; idx++) {
        /* get entry info */
        sym = (AMX_DBG_SYMBOL *)dbgptr;
        /* store */
        dbgsym.addr = (int32_t)sym->address;
        dbgsym.tagid = sym->tag;
        dbgsym.codestart = (uint32_t)sym->codestart;
        dbgsym.codeend = (uint32_t)sym->codeend;
        dbgsym.dimcount = (uint16_t)sym->dim;
        dbgsym.vclass = (uint8_t)sym->vclass;
        dbgsym.ident = (uint8_t)sym->ident;
        dbgsym.name = (uint32_t)memfile_tell(dbgtab);
        sfwrite(&dbgsym, sizeof(sp_fdbg_symbol_t), 1, spf);
        /* write to tab */
        len = strlen(sym->name);
        memfile_write(dbgtab, sym->name, len + 1);
        /* move to next */
        dbgptr += sizeof(AMX_DBG_SYMBOL) + len;
        /* look for any dimensions */
        info.num_syms++;
        for (dnum=0; dnum<dbgsym.dimcount; dnum++)
        {
          /* get entry info */
          dim = (AMX_DBG_SYMDIM *)dbgptr;
          /* store */
          dbgdim.size = (uint32_t)dim->size;
          dbgdim.tagid = dim->tag;
          sfwrite(&dbgdim, sizeof(sp_fdbg_arraydim_t), 1, spf);
          /* move to next */
          dbgptr += sizeof(AMX_DBG_SYMDIM);
          info.num_arrays++;
        }
      }
      spfw_next_section(spf);
    }

    sfwrite(&info, sizeof(sp_fdbg_info_t), 1, spf);
    spfw_next_section(spf);

    if (sections[FS_DbgStrings]) {
      sfwrite(dbgtab->base, sizeof(char), dbgtab->usedoffs, spf);
      spfw_next_section(spf);
    }
  }

  spfw_finalize_all(spf);

  /** 
   * do compression
   * new block for scoping only
   */
  {
    memfile_t *pOrig = (memfile_t *)spf->handle;
    sp_file_hdr_t *pHdr;
    unsigned char *proper;
    size_t size;
    Bytef *zcmp;
    uLong disksize;
    size_t header_size;
    int err = Z_OK;

    /* reuse this memory block! */
    memfile_reset(bin_file);

    /* copy tip of header */
    memfile_write(bin_file, pOrig->base, sizeof(sp_file_hdr_t));

    /* get pointer to header */
    pHdr = (sp_file_hdr_t *)bin_file->base;

    /* copy the rest of the header */
    memfile_write(bin_file, 
            (unsigned char *)pOrig->base + sizeof(sp_file_hdr_t),
            pHdr->dataoffs - sizeof(sp_file_hdr_t));

    header_size = pHdr->dataoffs;
    size = pHdr->imagesize - header_size;
    proper = (unsigned char *)pOrig->base + header_size;

    /* get initial size estimate */
    disksize = compressBound(pHdr->imagesize);
    pHdr->disksize = (uint32_t)disksize;
    zcmp = (Bytef *)malloc(pHdr->disksize);

    if ((err=compress2(zcmp, 
        &disksize, 
        (Bytef *)proper,
        (uLong)size,
        Z_BEST_COMPRESSION))
      != Z_OK)
    {
      free(zcmp);
      pc_printf("Unable to compress (Z): error %d\n", err);
      pc_printf("Falling back to no compression.");
      memfile_write(bin_file, 
            proper,
            size);
    } else {
      pHdr->disksize = (uint32_t)disksize + header_size;
      pHdr->compression = SPFILE_COMPRESSION_GZ;
      memfile_write(bin_file,
            (unsigned char *)zcmp,
            disksize);
      free(zcmp);
    }
  }

  spfw_destroy(spf);
  memfile_destroy(dbgtab);

  /** 
   * write file 
   */
  if ((fp=fopen(bin_file->name, "wb")) != NULL) {
    fwrite(bin_file->base, bin_file->usedoffs, 1, fp);
    fclose(fp);
  } else {
    pc_printf("Unable to open %s for writing!", bin_file->name);
  }

  memfile_destroy(bin_file);

  return 0;

write_error:
  pc_printf("Error writing to file: %s", bin_file->name);
  
  spfw_destroy(spf);
  unlink(bin_file->name);
  memfile_destroy(bin_file);
  memfile_destroy(dbgtab);

  return 1;
}

void sfwrite(const void *buf, size_t size, size_t count, sp_file_t *spf)
{
  if (spf->funcs.fnWrite(buf, size, count, spf->handle) != count)
    longjmp(brkout, 1);
}

void sp_fdbg_ntv_start(int num_natives)
{
  if (num_natives == 0)
    return;

  native_list = (t_native *)malloc(sizeof(t_native) * num_natives);
  memset(native_list, 0, sizeof(t_native) * num_natives);
}

#include "sc.h"
void sp_fdbg_ntv_hook(int index, symbol *sym)
{
  int i, j;
  t_native *native;

  native = &native_list[index];
  native->name = strdup(sym->name);
  
  for (i = 0; i < sMAXARGS; i++) {
    if (sym->dim.arglist[i].ident == 0)
      break;
    native->num_args++;
    native->args[i].tag = sym->dim.arglist[i].tags == NULL ? 0 : sym->dim.arglist[i].tags[0];
    native->args[i].name = strdup(sym->dim.arglist[i].name);
    native->args[i].ident = sym->dim.arglist[i].ident;
    native->args[i].dimcount = sym->dim.arglist[i].numdim;
    for (j = 0; j < native->args[i].dimcount; j++) {
      native->args[i].dims[j].size = sym->dim.arglist[i].dim[j];
      native->args[i].dims[j].tagid = sym->dim.arglist[i].idxtag[j];
    }
  }

  native->ret_tag = sym->tag;
}


#if defined __linux__ || defined __APPLE__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif
