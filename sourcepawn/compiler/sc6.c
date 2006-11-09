/*  Pawn compiler - Binary code generation (the "assembler")
 *
 *  Copyright (c) ITB CompuPhase, 1997-2006
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id: sc6.c 3633 2006-08-11 16:20:18Z thiadmer $
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>     /* for macro max() */
#include <stddef.h>     /* for macro offsetof() */
#include <string.h>
#include <ctype.h>
#if defined FORTIFY
  #include <alloc/fortify.h>
#endif
#include "lstring.h"
#include "sc.h"
#include "amxdbg.h"
#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
  #include <sclinux.h>
#endif


static void append_dbginfo(FILE *fout);


typedef cell (*OPCODE_PROC)(FILE *fbin,char *params,cell opcode);

typedef struct {
  cell opcode;
  char *name;
  int segment;          /* sIN_CSEG=parse in cseg, sIN_DSEG=parse in dseg */
  OPCODE_PROC func;
} OPCODEC;

static cell codeindex;  /* similar to "code_idx" */
static cell *lbltab;    /* label table */
static int writeerror;
static int bytes_in, bytes_out;
static jmp_buf compact_err;

/* apparently, strtol() does not work correctly on very large (unsigned)
 * hexadecimal values */
static ucell hex2long(const char *s,char **n)
{
  ucell result=0L;
  int negate=FALSE;
  int digit;

  /* ignore leading whitespace */
  while (*s==' ' || *s=='\t')
    s++;

  /* allow a negation sign to create the two's complement of numbers */
  if (*s=='-') {
    negate=TRUE;
    s++;
  } /* if */

  assert((*s>='0' && *s<='9') || (*s>='a' && *s<='f') || (*s>='a' && *s<='f'));
  for ( ;; ) {
    if (*s>='0' && *s<='9')
      digit=*s-'0';
    else if (*s>='a' && *s<='f')
      digit=*s-'a' + 10;
    else if (*s>='A' && *s<='F')
      digit=*s-'A' + 10;
    else
      break;    /* probably whitespace */
    result=(result<<4) | digit;
    s++;
  } /* for */
  if (n!=NULL)
    *n=(char*)s;
  if (negate)
    result=(~result)+1; /* take two's complement of the result */
  return (ucell)result;
}

static ucell getparam(const char *s,char **n)
{
  ucell result=0;
  for ( ;; ) {
    result+=hex2long(s,(char**)&s);
    if (*s!='+')
      break;
    s++;
  } /* for */
  if (n!=NULL)
    *n=(char*)s;
  return result;
}

#if BYTE_ORDER==BIG_ENDIAN
static uint16_t *align16(uint16_t *v)
{
  unsigned char *s = (unsigned char *)v;
  unsigned char t;

  /* swap two bytes */
  t=s[0];
  s[0]=s[1];
  s[1]=t;
  return v;
}

static uint32_t *align32(uint32_t *v)
{
  unsigned char *s = (unsigned char *)v;
  unsigned char t;

  /* swap outer two bytes */
  t=s[0];
  s[0]=s[3];
  s[3]=t;
  /* swap inner two bytes */
  t=s[1];
  s[1]=s[2];
  s[2]=t;
  return v;
}

#if PAWN_CELL_SIZE>=64
static uint64_t *align64(uint64_t *v)
{
	unsigned char *s = (unsigned char *)v;
	unsigned char t;

	t=s[0];
	s[0]=s[7];
	s[7]=t;

	t=s[1];
	s[1]=s[6];
	s[6]=t;

	t=s[2];
	s[2]=s[5];
	s[5]=t;

	t=s[3];
	s[3]=s[4];
	s[4]=t;

	return v;
}
#endif

  #if PAWN_CELL_SIZE==16
    #define aligncell(v)  align16(v)
  #elif PAWN_CELL_SIZE==32
    #define aligncell(v)  align32(v)
  #elif PAWN_CELL_SIZE==64
    #define aligncell(v)  align64(v)
  #endif
#else
  #define align16(v)    (v)
  #define align32(v)    (v)
  #define aligncell(v)  (v)
#endif

static char *skipwhitespace(char *str)
{
  while (isspace(*str))
    str++;
  return str;
}

static char *stripcomment(char *str)
{
  char *ptr=strchr(str,';');
  if (ptr!=NULL) {
    *ptr++='\n';        /* terminate the line, but leave the '\n' */
    *ptr='\0';
  } /* if */
  return str;
}

static void write_encoded(FILE *fbin,ucell *c,int num)
{
  #if PAWN_CELL_SIZE == 16
    #define ENC_MAX   3     /* a 16-bit cell is encoded in max. 3 bytes */
    #define ENC_MASK  0x03  /* after 2x7 bits, 2 bits remain to make 16 bits */
  #elif PAWN_CELL_SIZE == 32
    #define ENC_MAX   5     /* a 32-bit cell is encoded in max. 5 bytes */
    #define ENC_MASK  0x0f  /* after 4x7 bits, 4 bits remain to make 32 bits */
  #elif PAWN_CELL_SIZE == 64
    #define ENC_MAX   10    /* a 32-bit cell is encoded in max. 10 bytes */
    #define ENC_MASK  0x01  /* after 9x7 bits, 1 bit remains to make 64 bits */
  #endif

  assert(fbin!=NULL);
  while (num-->0) {
    if (sc_compress) {
      ucell p=(ucell)*c;
      unsigned char t[ENC_MAX];
      unsigned char code;
      int index;
      for (index=0; index<ENC_MAX; index++) {
        t[index]=(unsigned char)(p & 0x7f);     /* store 7 bits */
        p>>=7;
      } /* for */
      /* skip leading zeros */
      while (index>1 && t[index-1]==0 && (t[index-2] & 0x40)==0)
        index--;
      /* skip leading -1s */
      if (index==ENC_MAX && t[index-1]==ENC_MASK && (t[index-2] & 0x40)!=0)
        index--;
      while (index>1 && t[index-1]==0x7f && (t[index-2] & 0x40)!=0)
        index--;
      /* write high byte first, write continuation bits */
      assert(index>0);
      while (index-->0) {
        code=(unsigned char)((index==0) ? t[index] : (t[index]|0x80));
        writeerror |= !pc_writebin(fbin,&code,1);
        bytes_out++;
      } /* while */
      bytes_in+=sizeof *c;
      assert(AMX_COMPACTMARGIN>2);
      if (bytes_out-bytes_in>=AMX_COMPACTMARGIN-2)
        longjmp(compact_err,1);
    } else {
      assert((pc_lengthbin(fbin) % sizeof(cell)) == 0);
      writeerror |= !pc_writebin(fbin,aligncell(c),sizeof *c);
    } /* if */
    c++;
  } /* while */
}

#if defined __BORLANDC__ || defined __WATCOMC__
  #pragma argsused
#endif
static cell noop(FILE *fbin,char *params,cell opcode)
{
  return 0;
}

#if defined __BORLANDC__ || defined __WATCOMC__
  #pragma argsused
#endif
static cell set_currentfile(FILE *fbin,char *params,cell opcode)
{
  fcurrent=(short)getparam(params,NULL);
  return 0;
}

#if defined __BORLANDC__ || defined __WATCOMC__
  #pragma argsused
#endif
static cell parm0(FILE *fbin,char *params,cell opcode)
{
  if (fbin!=NULL)
    write_encoded(fbin,(ucell*)&opcode,1);
  return opcodes(1);
}

static cell parm1(FILE *fbin,char *params,cell opcode)
{
  ucell p=getparam(params,NULL);
  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p,1);
  } /* if */
  return opcodes(1)+opargs(1);
}

static cell parm2(FILE *fbin,char *params,cell opcode)
{
  ucell p1=getparam(params,&params);
  ucell p2=getparam(params,NULL);
  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p1,1);
    write_encoded(fbin,&p2,1);
  } /* if */
  return opcodes(1)+opargs(2);
}

static cell parm3(FILE *fbin,char *params,cell opcode)
{
  ucell p1=getparam(params,&params);
  ucell p2=getparam(params,&params);
  ucell p3=getparam(params,NULL);
  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p1,1);
    write_encoded(fbin,&p2,1);
    write_encoded(fbin,&p3,1);
  } /* if */
  return opcodes(1)+opargs(3);
}

static cell parm4(FILE *fbin,char *params,cell opcode)
{
  ucell p1=getparam(params,&params);
  ucell p2=getparam(params,&params);
  ucell p3=getparam(params,&params);
  ucell p4=getparam(params,NULL);
  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p1,1);
    write_encoded(fbin,&p2,1);
    write_encoded(fbin,&p3,1);
    write_encoded(fbin,&p4,1);
  } /* if */
  return opcodes(1)+opargs(4);
}

static cell parm5(FILE *fbin,char *params,cell opcode)
{
  ucell p1=getparam(params,&params);
  ucell p2=getparam(params,&params);
  ucell p3=getparam(params,&params);
  ucell p4=getparam(params,&params);
  ucell p5=getparam(params,NULL);
  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p1,1);
    write_encoded(fbin,&p2,1);
    write_encoded(fbin,&p3,1);
    write_encoded(fbin,&p4,1);
    write_encoded(fbin,&p5,1);
  } /* if */
  return opcodes(1)+opargs(5);
}

#if defined __BORLANDC__ || defined __WATCOMC__
  #pragma argsused
#endif
static cell do_dump(FILE *fbin,char *params,cell opcode)
{
  ucell p;
  int num = 0;

  while (*params!='\0') {
    p=getparam(params,&params);
    if (fbin!=NULL)
      write_encoded(fbin,&p,1);
    num++;
    while (isspace(*params))
      params++;
  } /* while */
  return num*sizeof(cell);
}

static cell do_call(FILE *fbin,char *params,cell opcode)
{
  char name[sNAMEMAX+1];
  int i;
  symbol *sym;
  ucell p;

  for (i=0; !isspace(*params); i++,params++) {
    assert(*params!='\0');
    assert(i<sNAMEMAX);
    name[i]=*params;
  } /* for */
  name[i]='\0';

  if (name[0]=='l' && name[1]=='.') {
    /* this is a label, not a function symbol */
    i=(int)hex2long(name+2,NULL);
    assert(i>=0 && i<sc_labnum);
    if (fbin!=NULL) {
      assert(lbltab!=NULL);
      p=lbltab[i];
    } /* if */
  } else {
    /* look up the function address; note that the correct file number must
     * already have been set (in order for static globals to be found).
     */
    sym=findglb(name,sGLOBAL);
    assert(sym!=NULL);
    assert(sym->ident==iFUNCTN || sym->ident==iREFFUNC);
    assert(sym->vclass==sGLOBAL);
    p=sym->addr;
  } /* if */

  if (fbin!=NULL) {
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p,1);
  } /* if */
  return opcodes(1)+opargs(1);
}

static cell do_jump(FILE *fbin,char *params,cell opcode)
{
  int i;
  ucell p;

  i=(int)hex2long(params,NULL);
  assert(i>=0 && i<sc_labnum);

  if (fbin!=NULL) {
    assert(lbltab!=NULL);
    p=lbltab[i];
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p,1);
  } /* if */
  return opcodes(1)+opargs(1);
}

static cell do_switch(FILE *fbin,char *params,cell opcode)
{
  int i;
  ucell p;

  i=(int)hex2long(params,NULL);
  assert(i>=0 && i<sc_labnum);

  if (fbin!=NULL) {
    assert(lbltab!=NULL);
    p=lbltab[i];
    write_encoded(fbin,(ucell*)&opcode,1);
    write_encoded(fbin,&p,1);
  } /* if */
  return opcodes(1)+opargs(1);
}

#if defined __BORLANDC__ || defined __WATCOMC__
  #pragma argsused
#endif
static cell do_case(FILE *fbin,char *params,cell opcode)
{
  int i;
  ucell p,v;

  v=hex2long(params,&params);
  i=(int)hex2long(params,NULL);
  assert(i>=0 && i<sc_labnum);

  if (fbin!=NULL) {
    assert(lbltab!=NULL);
    p=lbltab[i];
    write_encoded(fbin,&v,1);
    write_encoded(fbin,&p,1);
  } /* if */
  return opcodes(0)+opargs(2);
}

static OPCODEC opcodelist[] = {
  /* node for "invalid instruction" */
  {  0, NULL,         0,        noop },
  /* opcodes in sorted order */
  { 78, "add",        sIN_CSEG, parm0 },
  { 87, "add.c",      sIN_CSEG, parm1 },
  { 14, "addr.alt",   sIN_CSEG, parm1 },
  { 13, "addr.pri",   sIN_CSEG, parm1 },
  { 30, "align.alt",  sIN_CSEG, parm1 },
  { 29, "align.pri",  sIN_CSEG, parm1 },
  { 81, "and",        sIN_CSEG, parm0 },
  {121, "bounds",     sIN_CSEG, parm1 },
  {137, "break",      sIN_CSEG, parm0 },  /* version 8 */
  { 49, "call",       sIN_CSEG, do_call },
  { 50, "call.pri",   sIN_CSEG, parm0 },
  {  0, "case",       sIN_CSEG, do_case },
  {130, "casetbl",    sIN_CSEG, parm0 },  /* version 1 */
  {118, "cmps",       sIN_CSEG, parm1 },
  {  0, "code",       sIN_CSEG, set_currentfile },
  {156, "const",      sIN_CSEG, parm2 },  /* version 9 */
  { 12, "const.alt",  sIN_CSEG, parm1 },
  { 11, "const.pri",  sIN_CSEG, parm1 },
  {157, "const.s",    sIN_CSEG, parm2 },  /* version 9 */
  {  0, "data",       sIN_DSEG, set_currentfile },
  {114, "dec",        sIN_CSEG, parm1 },
  {113, "dec.alt",    sIN_CSEG, parm0 },
  {116, "dec.i",      sIN_CSEG, parm0 },
  {112, "dec.pri",    sIN_CSEG, parm0 },
  {115, "dec.s",      sIN_CSEG, parm1 },
  {  0, "dump",       sIN_DSEG, do_dump },
  { 95, "eq",         sIN_CSEG, parm0 },
  {106, "eq.c.alt",   sIN_CSEG, parm1 },
  {105, "eq.c.pri",   sIN_CSEG, parm1 },
/*{124, "file",       sIN_CSEG, do_file }, */
  {119, "fill",       sIN_CSEG, parm1 },
  {162, "genarray",   sIN_CSEG, parm1 },
  {163, "genarray.z", sIN_CSEG, parm1 },
  {100, "geq",        sIN_CSEG, parm0 },
  { 99, "grtr",       sIN_CSEG, parm0 },
  {120, "halt",       sIN_CSEG, parm1 },
  { 45, "heap",       sIN_CSEG, parm1 },
  { 27, "idxaddr",    sIN_CSEG, parm0 },
  { 28, "idxaddr.b",  sIN_CSEG, parm1 },
  {109, "inc",        sIN_CSEG, parm1 },
  {108, "inc.alt",    sIN_CSEG, parm0 },
  {111, "inc.i",      sIN_CSEG, parm0 },
  {107, "inc.pri",    sIN_CSEG, parm0 },
  {110, "inc.s",      sIN_CSEG, parm1 },
  { 86, "invert",     sIN_CSEG, parm0 },
  { 55, "jeq",        sIN_CSEG, do_jump },
  { 60, "jgeq",       sIN_CSEG, do_jump },
  { 59, "jgrtr",      sIN_CSEG, do_jump },
  { 58, "jleq",       sIN_CSEG, do_jump },
  { 57, "jless",      sIN_CSEG, do_jump },
  { 56, "jneq",       sIN_CSEG, do_jump },
  { 54, "jnz",        sIN_CSEG, do_jump },
  { 52, "jrel",       sIN_CSEG, parm1 },  /* always a number */
  { 64, "jsgeq",      sIN_CSEG, do_jump },
  { 63, "jsgrtr",     sIN_CSEG, do_jump },
  { 62, "jsleq",      sIN_CSEG, do_jump },
  { 61, "jsless",     sIN_CSEG, do_jump },
  { 51, "jump",       sIN_CSEG, do_jump },
  {128, "jump.pri",   sIN_CSEG, parm0 },  /* version 1 */
  { 53, "jzer",       sIN_CSEG, do_jump },
  { 31, "lctrl",      sIN_CSEG, parm1 },
  { 98, "leq",        sIN_CSEG, parm0 },
  { 97, "less",       sIN_CSEG, parm0 },
  { 25, "lidx",       sIN_CSEG, parm0 },
  { 26, "lidx.b",     sIN_CSEG, parm1 },
/*{125, "line",       sIN_CSEG, parm2 }, */
  {  2, "load.alt",   sIN_CSEG, parm1 },
  {154, "load.both",  sIN_CSEG, parm2 },  /* version 9 */
  {  9, "load.i",     sIN_CSEG, parm0 },
  {  1, "load.pri",   sIN_CSEG, parm1 },
  {  4, "load.s.alt", sIN_CSEG, parm1 },
  {155, "load.s.both",sIN_CSEG, parm2 },  /* version 9 */
  {  3, "load.s.pri", sIN_CSEG, parm1 },
  { 10, "lodb.i",     sIN_CSEG, parm1 },
  {  6, "lref.alt",   sIN_CSEG, parm1 },
  {  5, "lref.pri",   sIN_CSEG, parm1 },
  {  8, "lref.s.alt", sIN_CSEG, parm1 },
  {  7, "lref.s.pri", sIN_CSEG, parm1 },
  { 34, "move.alt",   sIN_CSEG, parm0 },
  { 33, "move.pri",   sIN_CSEG, parm0 },
  {117, "movs",       sIN_CSEG, parm1 },
  { 85, "neg",        sIN_CSEG, parm0 },
  { 96, "neq",        sIN_CSEG, parm0 },
  {134, "nop",        sIN_CSEG, parm0 },  /* version 6 */
  { 84, "not",        sIN_CSEG, parm0 },
  { 82, "or",         sIN_CSEG, parm0 },
  { 43, "pop.alt",    sIN_CSEG, parm0 },
  { 42, "pop.pri",    sIN_CSEG, parm0 },
  { 46, "proc",       sIN_CSEG, parm0 },
  { 40, "push",       sIN_CSEG, parm1 },
  {133, "push.adr",   sIN_CSEG, parm1 },  /* version 4 */
  { 37, "push.alt",   sIN_CSEG, parm0 },
  { 39, "push.c",     sIN_CSEG, parm1 },
  { 36, "push.pri",   sIN_CSEG, parm0 },
  { 38, "push.r",     sIN_CSEG, parm1 },  /* obsolete (never generated) */
  { 41, "push.s",     sIN_CSEG, parm1 },
  {139, "push2",      sIN_CSEG, parm2 },  /* version 9 */
  {141, "push2.adr",  sIN_CSEG, parm2 },  /* version 9 */
  {138, "push2.c",    sIN_CSEG, parm2 },  /* version 9 */
  {140, "push2.s",    sIN_CSEG, parm2 },  /* version 9 */
  {143, "push3",      sIN_CSEG, parm3 },  /* version 9 */
  {145, "push3.adr",  sIN_CSEG, parm3 },  /* version 9 */
  {142, "push3.c",    sIN_CSEG, parm3 },  /* version 9 */
  {144, "push3.s",    sIN_CSEG, parm3 },  /* version 9 */
  {147, "push4",      sIN_CSEG, parm4 },  /* version 9 */
  {149, "push4.adr",  sIN_CSEG, parm4 },  /* version 9 */
  {146, "push4.c",    sIN_CSEG, parm4 },  /* version 9 */
  {148, "push4.s",    sIN_CSEG, parm4 },  /* version 9 */
  {151, "push5",      sIN_CSEG, parm5 },  /* version 9 */
  {153, "push5.adr",  sIN_CSEG, parm5 },  /* version 9 */
  {150, "push5.c",    sIN_CSEG, parm5 },  /* version 9 */
  {152, "push5.s",    sIN_CSEG, parm5 },  /* version 9 */
  { 47, "ret",        sIN_CSEG, parm0 },
  { 48, "retn",       sIN_CSEG, parm0 },
  { 32, "sctrl",      sIN_CSEG, parm1 },
  { 73, "sdiv",       sIN_CSEG, parm0 },
  { 74, "sdiv.alt",   sIN_CSEG, parm0 },
  {104, "sgeq",       sIN_CSEG, parm0 },
  {103, "sgrtr",      sIN_CSEG, parm0 },
  { 65, "shl",        sIN_CSEG, parm0 },
  { 69, "shl.c.alt",  sIN_CSEG, parm1 },
  { 68, "shl.c.pri",  sIN_CSEG, parm1 },
  { 66, "shr",        sIN_CSEG, parm0 },
  { 71, "shr.c.alt",  sIN_CSEG, parm1 },
  { 70, "shr.c.pri",  sIN_CSEG, parm1 },
  { 94, "sign.alt",   sIN_CSEG, parm0 },
  { 93, "sign.pri",   sIN_CSEG, parm0 },
  {102, "sleq",       sIN_CSEG, parm0 },
  {101, "sless",      sIN_CSEG, parm0 },
  { 72, "smul",       sIN_CSEG, parm0 },
  { 88, "smul.c",     sIN_CSEG, parm1 },
/*{127, "srange",     sIN_CSEG, parm2 }, -- version 1 */
  { 20, "sref.alt",   sIN_CSEG, parm1 },
  { 19, "sref.pri",   sIN_CSEG, parm1 },
  { 22, "sref.s.alt", sIN_CSEG, parm1 },
  { 21, "sref.s.pri", sIN_CSEG, parm1 },
  { 67, "sshr",       sIN_CSEG, parm0 },
  { 44, "stack",      sIN_CSEG, parm1 },
  {  0, "stksize",    0,        noop },
  { 16, "stor.alt",   sIN_CSEG, parm1 },
  { 23, "stor.i",     sIN_CSEG, parm0 },
  { 15, "stor.pri",   sIN_CSEG, parm1 },
  { 18, "stor.s.alt", sIN_CSEG, parm1 },
  { 17, "stor.s.pri", sIN_CSEG, parm1 },
  { 24, "strb.i",     sIN_CSEG, parm1 },
  { 79, "sub",        sIN_CSEG, parm0 },
  { 80, "sub.alt",    sIN_CSEG, parm0 },
  {132, "swap.alt",   sIN_CSEG, parm0 },  /* version 4 */
  {131, "swap.pri",   sIN_CSEG, parm0 },  /* version 4 */
  {129, "switch",     sIN_CSEG, do_switch }, /* version 1 */
/*{126, "symbol",     sIN_CSEG, do_symbol }, */
/*{136, "symtag",     sIN_CSEG, parm1 },  -- version 7 */
  {123, "sysreq.c",   sIN_CSEG, parm1 },
  {135, "sysreq.n",   sIN_CSEG, parm2 },  /* version 9 (replaces SYSREQ.d from earlier version) */
  {122, "sysreq.pri", sIN_CSEG, parm0 },
  {161, "tracker.pop.setheap", sIN_CSEG, parm0 },
  {160, "tracker.push.c", sIN_CSEG, parm1 },
  { 76, "udiv",       sIN_CSEG, parm0 },
  { 77, "udiv.alt",   sIN_CSEG, parm0 },
  { 75, "umul",       sIN_CSEG, parm0 },
  { 35, "xchg",       sIN_CSEG, parm0 },
  { 83, "xor",        sIN_CSEG, parm0 },
  { 91, "zero",       sIN_CSEG, parm1 },
  { 90, "zero.alt",   sIN_CSEG, parm0 },
  { 89, "zero.pri",   sIN_CSEG, parm0 },
  { 92, "zero.s",     sIN_CSEG, parm1 },
};

#define MAX_INSTR_LEN   30
static int findopcode(char *instr,int maxlen)
{
  int low,high,mid,cmp;
  char str[MAX_INSTR_LEN];

  if (maxlen>=MAX_INSTR_LEN)
    return 0;
  strlcpy(str,instr,maxlen+1);
  /* look up the instruction with a binary search
   * the assembler is case insensitive to instructions (but case sensitive
   * to symbols)
   */
  low=1;                /* entry 0 is reserved (for "not found") */
  high=(sizeof opcodelist / sizeof opcodelist[0])-1;
  while (low<high) {
    mid=(low+high)/2;
    assert(opcodelist[mid].name!=NULL);
    cmp=stricmp(str,opcodelist[mid].name);
    if (cmp>0)
      low=mid+1;
    else
      high=mid;
  } /* while */

  assert(low==high);
  if (stricmp(str,opcodelist[low].name)==0)
    return low;         /* found */
  return 0;             /* not found, return special index */
}

SC_FUNC int assemble(FILE *fout,FILE *fin)
{
  AMX_HEADER hdr;
  AMX_FUNCSTUBNT func;
  int numpublics,numnatives,numlibraries,numpubvars,numtags,padding;
  long nametablesize,nameofs;
  #if PAWN_CELL_SIZE > 32
    char line[512];
  #else
    char line[256];
  #endif
  char *instr,*params;
  int i,pass,size;
  int16_t count;
  symbol *sym, **nativelist;
  constvalue *constptr;
  cell mainaddr;
  char nullchar;

  /* if compression failed, restart the assembly with compaction switched off */
  if (setjmp(compact_err)!=0) {
    assert(sc_compress);  /* cannot arrive here if compact encoding was disabled */
    sc_compress=FALSE;
    pc_resetbin(fout,0);
    error(232);           /* disabled compact encoding */
  } /* if */

  #if !defined NDEBUG
    /* verify that the opcode list is sorted (skip entry 1; it is reserved
     * for a non-existant opcode)
     */
    assert(opcodelist[1].name!=NULL);
    for (i=2; i<(sizeof opcodelist / sizeof opcodelist[0]); i++) {
      assert(opcodelist[i].name!=NULL);
      assert(stricmp(opcodelist[i].name,opcodelist[i-1].name)>0);
    } /* for */
  #endif

  writeerror=FALSE;
  nametablesize=sizeof(int16_t);
  numpublics=0;
  numnatives=0;
  numpubvars=0;
  mainaddr=-1;
  /* count number of public and native functions and public variables */
  for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
    int match=0;
    if (sym->ident==iFUNCTN) {
      if ((sym->usage & uNATIVE)!=0 && (sym->usage & uREAD)!=0 && sym->addr>=0)
        match=++numnatives;
      if ((sym->usage & uPUBLIC)!=0 && (sym->usage & uDEFINE)!=0)
        match=++numpublics;
      if (strcmp(sym->name,uMAINFUNC)==0) {
        assert(sym->vclass==sGLOBAL);
        mainaddr=sym->addr;
      } /* if */
    } else if (sym->ident==iVARIABLE || sym->ident == iARRAY || sym->ident == iREFARRAY) {
      if ((sym->usage & uPUBLIC)!=0 && (sym->usage & (uREAD | uWRITTEN))!=0)
        match=++numpubvars;
    } /* if */
    if (match) {
      char alias[sNAMEMAX+1];
      assert(sym!=NULL);
      if ((sym->usage & uNATIVE)==0 || !lookup_alias(alias,sym->name)) {
        assert(strlen(sym->name)<=sNAMEMAX);
        strcpy(alias,sym->name);
      } /* if */
      nametablesize+=strlen(alias)+1;
    } /* if */
  } /* for */
  assert(numnatives==ntv_funcid);

  /* count number of libraries */
  numlibraries=0;
  if (pc_addlibtable) {
    for (constptr=libname_tab.next; constptr!=NULL; constptr=constptr->next) {
      if (constptr->value>0) {
        assert(strlen(constptr->name)>0);
        numlibraries++;
        nametablesize+=strlen(constptr->name)+1;
      } /* if */
    } /* for */
  } /* if */

  /* count number of public tags */
  numtags=0;
  for (constptr=tagname_tab.next; constptr!=NULL; constptr=constptr->next) {
    if ((constptr->value & PUBLICTAG)!=0) {
      assert(strlen(constptr->name)>0);
      numtags++;
      nametablesize+=strlen(constptr->name)+1;
    } /* if */
  } /* for */

  /* pad the header to sc_dataalign
   * => thereby the code segment is aligned
   * => since the code segment is padded to a sc_dataalign boundary, the data segment is aligned
   * => and thereby the stack top is aligned too
   */
  assert(sc_dataalign!=0);
  padding= (int)(sc_dataalign - (sizeof hdr + nametablesize) % sc_dataalign);
  if (padding==sc_dataalign)
    padding=0;

  /* write the abstract machine header */
  memset(&hdr, 0, sizeof hdr);
  hdr.magic=(unsigned short)AMX_MAGIC;
  hdr.file_version=(char)((pc_optimize<=sOPTIMIZE_NOMACRO) ? MAX_FILE_VER_JIT : CUR_FILE_VERSION);
  hdr.amx_version=(char)((pc_optimize<=sOPTIMIZE_NOMACRO) ? MIN_AMX_VER_JIT : MIN_AMX_VERSION);
  hdr.flags=(short)(sc_debug & sSYMBOLIC);
  if (sc_compress)
    hdr.flags|=AMX_FLAG_COMPACT;
  if (sc_debug==0)
    hdr.flags|=AMX_FLAG_NOCHECKS;
  if (pc_memflags & suSLEEP_INSTR)
    hdr.flags|=AMX_FLAG_SLEEP;
  hdr.defsize=sizeof(AMX_FUNCSTUBNT);
  hdr.publics=sizeof hdr; /* public table starts right after the header */
  hdr.natives=hdr.publics + numpublics*sizeof(AMX_FUNCSTUBNT);
  hdr.libraries=hdr.natives + numnatives*sizeof(AMX_FUNCSTUBNT);
  hdr.pubvars=hdr.libraries + numlibraries*sizeof(AMX_FUNCSTUBNT);
  hdr.tags=hdr.pubvars + numpubvars*sizeof(AMX_FUNCSTUBNT);
  hdr.nametable=hdr.tags + numtags*sizeof(AMX_FUNCSTUBNT);
  hdr.cod=hdr.nametable + nametablesize + padding;
  hdr.dat=hdr.cod + code_idx;
  hdr.hea=hdr.dat + glb_declared*sizeof(cell);
  hdr.stp=hdr.hea + pc_stksize*sizeof(cell);
  hdr.cip=mainaddr;
  hdr.size=hdr.hea; /* preset, this is incorrect in case of compressed output */
  pc_writebin(fout,&hdr,sizeof hdr);

  /* dump zeros up to the rest of the header, so that we can easily "seek" */
  nullchar='\0';
  for (nameofs=sizeof hdr; nameofs<hdr.cod; nameofs++)
    pc_writebin(fout,&nullchar,1);
  nameofs=hdr.nametable+sizeof(int16_t);

  /* write the public functions table */
  count=0;
  for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
    if (sym->ident==iFUNCTN
        && (sym->usage & uPUBLIC)!=0 && (sym->usage & uDEFINE)!=0)
    {
      assert(sym->vclass==sGLOBAL);
      func.address=sym->addr;
      func.nameofs=nameofs;
      #if BYTE_ORDER==BIG_ENDIAN
        align32(&func.address);
        align32(&func.nameofs);
      #endif
      pc_resetbin(fout,hdr.publics+count*sizeof(AMX_FUNCSTUBNT));
      pc_writebin(fout,&func,sizeof func);
      pc_resetbin(fout,nameofs);
      pc_writebin(fout,sym->name,strlen(sym->name)+1);
      nameofs+=strlen(sym->name)+1;
      count++;
    } /* if */
  } /* for */

  /* write the natives table */
  /* The native functions must be written in sorted order. (They are
   * sorted on their "id", not on their name). A nested loop to find
   * each successive function would be an O(n^2) operation. But we
   * do not really need to sort, because the native function id's
   * are sequential and there are no duplicates. So we first walk
   * through the complete symbol list and store a pointer to every
   * native function of interest in a temporary table, where its id
   * serves as the index in the table. Now we can walk the table and
   * have all native functions in sorted order.
   */
  if (numnatives>0) {
    nativelist=(symbol **)malloc(numnatives*sizeof(symbol *));
    if (nativelist==NULL)
      error(103);               /* insufficient memory */
    #if !defined NDEBUG
      memset(nativelist,0,numnatives*sizeof(symbol *)); /* for NULL checking */
    #endif
    for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
      if (sym->ident==iFUNCTN && (sym->usage & uNATIVE)!=0 && (sym->usage & uREAD)!=0 && sym->addr>=0) {
        assert(sym->addr < numnatives);
        nativelist[(int)sym->addr]=sym;
      } /* if */
    } /* for */
    count=0;
    for (i=0; i<numnatives; i++) {
      char alias[sNAMEMAX+1];
      sym=nativelist[i];
      assert(sym!=NULL);
      if (!lookup_alias(alias,sym->name)) {
        assert(strlen(sym->name)<=sNAMEMAX);
        strcpy(alias,sym->name);
      } /* if */
      assert(sym->vclass==sGLOBAL);
      func.address=0;
      func.nameofs=nameofs;
      #if BYTE_ORDER==BIG_ENDIAN
        align32(&func.address);
        align32(&func.nameofs);
      #endif
      pc_resetbin(fout,hdr.natives+count*sizeof(AMX_FUNCSTUBNT));
      pc_writebin(fout,&func,sizeof func);
      pc_resetbin(fout,nameofs);
      pc_writebin(fout,alias,strlen(alias)+1);
      nameofs+=strlen(alias)+1;
      count++;
    } /* for */
    free(nativelist);
  } /* if */

  /* write the libraries table */
  if (pc_addlibtable) {
    count=0;
    for (constptr=libname_tab.next; constptr!=NULL; constptr=constptr->next) {
      if (constptr->value>0) {
        assert(strlen(constptr->name)>0);
        func.address=0;
        func.nameofs=nameofs;
        #if BYTE_ORDER==BIG_ENDIAN
          align32(&func.address);
          align32(&func.nameofs);
        #endif
        pc_resetbin(fout,hdr.libraries+count*sizeof(AMX_FUNCSTUBNT));
        pc_writebin(fout,&func,sizeof func);
        pc_resetbin(fout,nameofs);
        pc_writebin(fout,constptr->name,strlen(constptr->name)+1);
        nameofs+=strlen(constptr->name)+1;
        count++;
      } /* if */
    } /* for */
  } /* if */

  /* write the public variables table */
  count=0;
  for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
    if ((sym->ident==iVARIABLE || sym->ident==iARRAY || sym->ident==iREFARRAY)
        && (sym->usage & uPUBLIC)!=0 && (sym->usage & (uREAD | uWRITTEN))!=0) {
      //removed until structs don't seem to mess this up
      //assert((sym->usage & uDEFINE)!=0);
      assert(sym->vclass==sGLOBAL);
      func.address=sym->addr;
      func.nameofs=nameofs;
      #if BYTE_ORDER==BIG_ENDIAN
        align32(&func.address);
        align32(&func.nameofs);
      #endif
      pc_resetbin(fout,hdr.pubvars+count*sizeof(AMX_FUNCSTUBNT));
      pc_writebin(fout,&func,sizeof func);
      pc_resetbin(fout,nameofs);
      pc_writebin(fout,sym->name,strlen(sym->name)+1);
      nameofs+=strlen(sym->name)+1;
      count++;
    } /* if */
  } /* for */

  /* write the public tagnames table */
  count=0;
  for (constptr=tagname_tab.next; constptr!=NULL; constptr=constptr->next) {
    if ((constptr->value & PUBLICTAG)!=0) {
      assert(strlen(constptr->name)>0);
      func.address=constptr->value & TAGMASK;
      func.nameofs=nameofs;
      #if BYTE_ORDER==BIG_ENDIAN
        align32(&func.address);
        align32(&func.nameofs);
      #endif
      pc_resetbin(fout,hdr.tags+count*sizeof(AMX_FUNCSTUBNT));
      pc_writebin(fout,&func,sizeof func);
      pc_resetbin(fout,nameofs);
      pc_writebin(fout,constptr->name,strlen(constptr->name)+1);
      nameofs+=strlen(constptr->name)+1;
      count++;
    } /* if */
  } /* for */

  /* write the "maximum name length" field in the name table */
  assert(nameofs==hdr.nametable+nametablesize);
  pc_resetbin(fout,hdr.nametable);
  count=sNAMEMAX;
  #if BYTE_ORDER==BIG_ENDIAN
    align16(&count);
  #endif
  pc_writebin(fout,&count,sizeof count);
  pc_resetbin(fout,hdr.cod);

  /* First pass: relocate all labels */
  /* This pass is necessary because the code addresses of labels is only known
   * after the peephole optimization flag. Labels can occur inside expressions
   * (e.g. the conditional operator), which are optimized.
   */
  lbltab=NULL;
  if (sc_labnum>0) {
    /* only very short programs have zero labels; no first pass is needed
     * if there are no labels */
    lbltab=(cell *)malloc(sc_labnum*sizeof(cell));
    if (lbltab==NULL)
      error(103);               /* insufficient memory */
    codeindex=0;
    pc_resetasm(fin);
    while (pc_readasm(fin,line,sizeof line)!=NULL) {
      stripcomment(line);
      instr=skipwhitespace(line);
      /* ignore empty lines */
      if (*instr=='\0')
        continue;
      if (tolower(*instr)=='l' && *(instr+1)=='.') {
        int lindex=(int)hex2long(instr+2,NULL);
        assert(lindex>=0 && lindex<sc_labnum);
        lbltab[lindex]=codeindex;
      } else {
        /* get to the end of the instruction (make use of the '\n' that fgets()
         * added at the end of the line; this way we will *always* drop on a
         * whitespace character) */
        for (params=instr; *params!='\0' && !isspace(*params); params++)
          /* nothing */;
        assert(params>instr);
        i=findopcode(instr,(int)(params-instr));
        if (opcodelist[i].name==NULL) {
          *params='\0';
          error(104,instr);     /* invalid assembler instruction */
        } /* if */
        if (opcodelist[i].segment==sIN_CSEG)
          codeindex+=opcodelist[i].func(NULL,skipwhitespace(params),opcodelist[i].opcode);
      } /* if */
    } /* while */
  } /* if */

  /* Second pass (actually 2 more passes, one for all code and one for all data) */
  bytes_in=0;
  bytes_out=0;
  for (pass=sIN_CSEG; pass<=sIN_DSEG; pass++) {
    pc_resetasm(fin);
    while (pc_readasm(fin,line,sizeof line)!=NULL) {
      stripcomment(line);
      instr=skipwhitespace(line);
      /* ignore empty lines and labels (labels have a special syntax, so these
       * must be parsed separately) */
      if (*instr=='\0' || tolower(*instr)=='l' && *(instr+1)=='.')
        continue;
      /* get to the end of the instruction (make use of the '\n' that fgets()
       * added at the end of the line; this way we will *always* drop on a
       * whitespace character) */
      for (params=instr; *params!='\0' && !isspace(*params); params++)
        /* nothing */;
      assert(params>instr);
      i=findopcode(instr,(int)(params-instr));
      assert(opcodelist[i].name!=NULL);
      if (opcodelist[i].segment==pass)
        opcodelist[i].func(fout,skipwhitespace(params),opcodelist[i].opcode);
    } /* while */
  } /* for */
  if (bytes_out-bytes_in>0)
    error(106);         /* compression buffer overflow */

  if (lbltab!=NULL) {
    free(lbltab);
    #if !defined NDEBUG
      lbltab=NULL;
    #endif
  } /* if */

  if (sc_compress)
    hdr.size=pc_lengthbin(fout);/* get this value before appending debug info */
  if (!writeerror && (sc_debug & sSYMBOLIC)!=0)
    append_dbginfo(fout);       /* optionally append debug file */

  if (writeerror)
    error(101,"disk full");

  /* adjust the header */
  size=(int)hdr.cod;    /* save, the value in the header may be swapped */
  #if BYTE_ORDER==BIG_ENDIAN
    align32(&hdr.size);
    align16(&hdr.magic);
    align16(&hdr.flags);
    align16(&hdr.defsize);
    align32(&hdr.publics);
    align32(&hdr.natives);
    align32(&hdr.libraries);
    align32(&hdr.pubvars);
    align32(&hdr.tags);
    align32(&hdr.nametable);
    align32(&hdr.cod);
    align32(&hdr.dat);
    align32(&hdr.hea);
    align32(&hdr.stp);
    align32(&hdr.cip);
  #endif
  pc_resetbin(fout,0);
  pc_writebin(fout,&hdr,sizeof hdr);

  /* return the size of the header (including name tables, but excluding code
   * or data sections)
   */
  return size;
}

static void append_dbginfo(FILE *fout)
{
  AMX_DBG_HDR dbghdr;
  AMX_DBG_LINE dbgline;
  AMX_DBG_SYMBOL dbgsym;
  AMX_DBG_SYMDIM dbgidxtag[sDIMEN_MAX];
  int index,dim,dbgsymdim;
  char *str,*prevstr,*name,*prevname;
  ucell codeidx,previdx;
  constvalue *constptr;
  char symname[2*sNAMEMAX+16];
  int16_t id1,id2;
  ucell address;

  /* header with general information */
  memset(&dbghdr, 0, sizeof dbghdr);
  dbghdr.size=sizeof dbghdr;
  dbghdr.magic=AMX_DBG_MAGIC;
  dbghdr.file_version=CUR_FILE_VERSION;
  dbghdr.amx_version=MIN_AMX_VERSION;

  /* first pass: collect the number of items in various tables */

  /* file table */
  previdx=0;
  prevstr=NULL;
  prevname=NULL;
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='F') {
      codeidx=hex2long(str+2,&name);
      if (codeidx!=previdx) {
        if (prevstr!=NULL) {
          assert(prevname!=NULL);
          dbghdr.files++;
          dbghdr.size+=sizeof(cell)+strlen(prevname)+1;
        } /* if */
        previdx=codeidx;
      } /* if */
      prevstr=str;
      prevname=skipwhitespace(name);
    } /* if */
  } /* for */
  if (prevstr!=NULL) {
    assert(prevname!=NULL);
    dbghdr.files++;
    dbghdr.size+=sizeof(cell)+strlen(prevname)+1;
  } /* if */

  /* line number table */
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='L') {
      dbghdr.lines++;
      dbghdr.size+=sizeof(AMX_DBG_LINE);
    } /* if */
  } /* for */

  /* symbol table */
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='S') {
      dbghdr.symbols++;
      name=strchr(str+2,':');
      assert(name!=NULL);
      dbghdr.size+=sizeof(AMX_DBG_SYMBOL)+strlen(skipwhitespace(name+1));
      if ((prevstr=strchr(name,'['))!=NULL)
        while ((prevstr=strchr(prevstr+1,':'))!=NULL)
          dbghdr.size+=sizeof(AMX_DBG_SYMDIM);
    } /* if */
  } /* for */

  /* tag table */
  for (constptr=tagname_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(strlen(constptr->name)>0);
    dbghdr.tags++;
    dbghdr.size+=sizeof(AMX_DBG_TAG)+strlen(constptr->name);
  } /* for */

  /* automaton table */
  for (constptr=sc_automaton_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(constptr->index==0 && strlen(constptr->name)==0 || strlen(constptr->name)>0);
    dbghdr.automatons++;
    dbghdr.size+=sizeof(AMX_DBG_MACHINE)+strlen(constptr->name);
  } /* for */

  /* state table */
  for (constptr=sc_state_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(strlen(constptr->name)>0);
    dbghdr.states++;
    dbghdr.size+=sizeof(AMX_DBG_STATE)+strlen(constptr->name);
  } /* for */


  /* pass 2: generate the tables */
  #if BYTE_ORDER==BIG_ENDIAN
    align32((uint32_t*)&dbghdr.size);
    align16(&dbghdr.magic);
    align16(&dbghdr.flags);
    align16(&dbghdr.files);
    align16(&dbghdr.lines);
    align16(&dbghdr.symbols);
    align16(&dbghdr.tags);
    align16(&dbghdr.automatons);
    align16(&dbghdr.states);
  #endif
  writeerror |= !pc_writebin(fout,&dbghdr,sizeof dbghdr);

  /* file table */
  previdx=0;
  prevstr=NULL;
  prevname=NULL;
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='F') {
      codeidx=hex2long(str+2,&name);
      if (codeidx!=previdx) {
        if (prevstr!=NULL) {
          assert(prevname!=NULL);
          #if BYTE_ORDER==BIG_ENDIAN
            aligncell(&previdx);
          #endif
          writeerror |= !pc_writebin(fout,&previdx,sizeof previdx);
          writeerror |= !pc_writebin(fout,prevname,strlen(prevname)+1);
        } /* if */
        previdx=codeidx;
      } /* if */
      prevstr=str;
      prevname=skipwhitespace(name);
    } /* if */
  } /* for */
  if (prevstr!=NULL) {
    assert(prevname!=NULL);
    #if BYTE_ORDER==BIG_ENDIAN
      aligncell(&previdx);
    #endif
    writeerror |= !pc_writebin(fout,&previdx,sizeof previdx);
    writeerror |= !pc_writebin(fout,prevname,strlen(prevname)+1);
  } /* if */

  /* line number table */
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='L') {
      dbgline.address=hex2long(str+2,&str);
      dbgline.line=(int32_t)hex2long(str,NULL);
      #if BYTE_ORDER==BIG_ENDIAN
        aligncell(&dbgline.address);
        align32(&dbgline.line);
      #endif
      writeerror |= !pc_writebin(fout,&dbgline,sizeof dbgline);
    } /* if */
  } /* for */

  /* symbol table */
  for (index=0; (str=get_dbgstring(index))!=NULL; index++) {
    assert(str!=NULL);
    assert(str[0]!='\0' && str[1]==':');
    if (str[0]=='S') {
      dbgsym.address=hex2long(str+2,&str);
      dbgsym.tag=(int16_t)hex2long(str,&str);
      str=skipwhitespace(str);
      assert(*str==':');
      name=skipwhitespace(str+1);
      str=strchr(name,' ');
      assert(str!=NULL);
      assert((int)(str-name)<sizeof symname);
      strlcpy(symname,name,(int)(str-name)+1);
      dbgsym.codestart=hex2long(str,&str);
      dbgsym.codeend=hex2long(str,&str);
      dbgsym.ident=(char)hex2long(str,&str);
      dbgsym.vclass=(char)hex2long(str,&str);
      dbgsym.dim=0;
      str=skipwhitespace(str);
      if (*str=='[') {
        while (*(str=skipwhitespace(str+1))!=']') {
          dbgidxtag[dbgsym.dim].tag=(int16_t)hex2long(str,&str);
          str=skipwhitespace(str);
          assert(*str==':');
          dbgidxtag[dbgsym.dim].size=hex2long(str+1,&str);
          dbgsym.dim++;
        } /* while */
      } /* if */
      dbgsymdim = dbgsym.dim;
      #if BYTE_ORDER==BIG_ENDIAN
        aligncell(&dbgsym.address);
        align16(&dbgsym.tag);
        aligncell(&dbgsym.codestart);
        aligncell(&dbgsym.codeend);
        align16(&dbgsym.dim);
      #endif
      writeerror |= !pc_writebin(fout,&dbgsym,offsetof(AMX_DBG_SYMBOL, name));
      writeerror |= !pc_writebin(fout,symname,strlen(symname)+1);
      for (dim=0; dim<dbgsymdim; dim++) {
        #if BYTE_ORDER==BIG_ENDIAN
          align16(&dbgidxtag[dim].tag);
          aligncell(&dbgidxtag[dim].size);
        #endif
        writeerror |= !pc_writebin(fout,&dbgidxtag[dim],sizeof dbgidxtag[dim]);
      } /* for */
    } /* if */
  } /* for */

  /* tag table */
  for (constptr=tagname_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(strlen(constptr->name)>0);
    id1=(int16_t)(constptr->value & TAGMASK);
    #if BYTE_ORDER==BIG_ENDIAN
      align16(&id1);
    #endif
    writeerror |= !pc_writebin(fout,&id1,sizeof id1);
    writeerror |= !pc_writebin(fout,constptr->name,strlen(constptr->name)+1);
  } /* for */

  /* automaton table */
  for (constptr=sc_automaton_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(constptr->index==0 && strlen(constptr->name)==0 || strlen(constptr->name)>0);
    id1=(int16_t)constptr->index;
    address=(ucell)constptr->value;
    #if BYTE_ORDER==BIG_ENDIAN
      align16(&id1);
      aligncell(&address);
    #endif
    writeerror |= !pc_writebin(fout,&id1,sizeof id1);
    writeerror |= !pc_writebin(fout,&address,sizeof address);
    writeerror |= !pc_writebin(fout,constptr->name,strlen(constptr->name)+1);
  } /* for */

  /* state table */
  for (constptr=sc_state_tab.next; constptr!=NULL; constptr=constptr->next) {
    assert(strlen(constptr->name)>0);
    id1=(int16_t)constptr->value;
    id2=(int16_t)constptr->index;
    address=(ucell)constptr->value;
    #if BYTE_ORDER==BIG_ENDIAN
      align16(&id1);
      align16(&id2);
    #endif
    writeerror |= !pc_writebin(fout,&id1,sizeof id1);
    writeerror |= !pc_writebin(fout,&id2,sizeof id2);
    writeerror |= !pc_writebin(fout,constptr->name,strlen(constptr->name)+1);
  } /* for */

  delete_dbgstringtable();
}
