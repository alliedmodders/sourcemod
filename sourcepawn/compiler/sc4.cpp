// vim: set ts=8 sts=2 sw=2 tw=99 et:
/*  Pawn compiler - code generation (unoptimized "assembler" code)
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
 *  Version: $Id$
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>     /* for _MAX_PATH */
#include <string.h>
#if defined FORTIFY
  #include <alloc/fortify.h>
#endif
#include "sc.h"
#include "sctracker.h"

static int fcurseg;     /* the file number (fcurrent) for the active segment */

void load_i();

/* When a subroutine returns to address 0, the AMX must halt. In earlier
 * releases, the RET and RETN opcodes checked for the special case 0 address.
 * Today, the compiler simply generates a HALT instruction at address 0. So
 * a subroutine can savely return to 0, and then encounter a HALT.
 */
void writeleader(symbol *root)
{
  int lbl_nostate,lbl_table;
  int statecount;
  symbol *sym;
  constvalue *fsa, *state, *stlist;
  int fsa_id,listid;
  char lbl_default[sNAMEMAX+1];

  assert(code_idx==0);

  begcseg();
  stgwrite(";program exit point\n");
  stgwrite("\thalt 0\n\n");
  code_idx+=opcodes(1)+opargs(1);       /* calculate code length */

  /* check whether there are any functions that have states */
  for (sym=root->next; sym!=NULL; sym=sym->next)
    if (sym->ident==iFUNCTN && (sym->usage & (uPUBLIC | uREAD))!=0 && sym->states!=NULL)
      break;
  if (sym==NULL)
    return;             /* no function has states, nothing to do next */

  /* generate an error function that is called for an undefined state */
  stgwrite("\n;exit point for functions called from the wrong state\n");
  lbl_nostate=getlabel();
  setlabel(lbl_nostate);
  stgwrite("\thalt ");
  outval(AMX_ERR_INVSTATE,TRUE);
  code_idx+=opcodes(1)+opargs(1);       /* calculate code length */

  /* write the "state-selectors" table with all automatons (update the
   * automatons structure too, as we are now assigning the address to
   * each automaton state-selector variable)
   */
  assert(glb_declared==0);
  begdseg();
  for (fsa=sc_automaton_tab.next; fsa!=NULL; fsa=fsa->next) {
    defstorage();
    stgwrite("0\t; automaton ");
    if (strlen(fsa->name)==0)
      stgwrite("(anonymous)");
    else
      stgwrite(fsa->name);
    stgwrite("\n");
    fsa->value=glb_declared*sizeof(cell);
    glb_declared++;
  } /* for */

  /* write stubs and jump tables for all state functions */
  begcseg();
  for (sym=root->next; sym!=NULL; sym=sym->next) {
    if (sym->ident==iFUNCTN && (sym->usage & (uPUBLIC | uREAD))!=0 && sym->states!=NULL) {
      stlist=sym->states->next;
      assert(stlist!=NULL);     /* there should be at least one state item */
      listid=stlist->index;
      assert(listid==-1 || listid>0);
      if (listid==-1 && stlist->next!=NULL) {
        /* first index is the "fallback", take the next one (if available) */
        stlist=stlist->next;
        listid=stlist->index;
      } /* if */
      if (listid==-1) {
        /* first index is the fallback, there is no second... */
        strcpy(stlist->name,"0"); /* insert dummy label number */
        /* this is an error, but we postpone adding the error message until the
         * function definition
         */
        continue;
      } /* if */
      /* generate label numbers for all statelist ids */
      for (stlist=sym->states->next; stlist!=NULL; stlist=stlist->next) {
        assert(strlen(stlist->name)==0);
        strcpy(stlist->name,itoh(getlabel()));
      } /* for */
      if (strcmp(sym->name,uENTRYFUNC)==0)
        continue;               /* do not generate stubs for this special function */
      sym->addr=code_idx;       /* fix the function address now */
      /* get automaton id for this function */
      assert(listid>0);
      fsa_id=state_getfsa(listid);
      assert(fsa_id>=0);        /* automaton 0 exists */
      fsa=automaton_findid(fsa_id);
      /* count the number of states actually used; at the sane time, check
       * whether there is a default state function
       */
      statecount=0;
      strcpy(lbl_default,itoh(lbl_nostate));
      for (stlist=sym->states->next; stlist!=NULL; stlist=stlist->next) {
        if (stlist->index==-1) {
          assert(strlen(stlist->name)<sizeof lbl_default);
          strcpy(lbl_default,stlist->name);
        } else {
          statecount+=state_count(stlist->index);
        } /* if */
      } /* for */
      /* generate a stub entry for the functions */
      stgwrite("\tload.pri ");
      outval(fsa->value,FALSE);
      stgwrite("\t; ");
      stgwrite(sym->name);
      stgwrite("\n");
      code_idx+=opcodes(1)+opargs(1);   /* calculate code length */
      lbl_table=getlabel();
      ffswitch(lbl_table);
      /* generate the jump table */
      setlabel(lbl_table);
      ffcase(statecount,lbl_default,TRUE);
      for (state=sc_state_tab.next; state!=NULL; state=state->next) {
        if (state->index==fsa_id) {
          /* find the label for this list id */
          for (stlist=sym->states->next; stlist!=NULL; stlist=stlist->next) {
            if (stlist->index!=-1 && state_inlist(stlist->index,(int)state->value)) {
              ffcase(state->value,stlist->name,FALSE);
              break;
            } /* if */
          } /* for */
          if (stlist==NULL && strtol(lbl_default,NULL,16)==lbl_nostate)
            error(230,state->name,sym->name);  /* unimplemented state, no fallback */
        } /* if (state belongs to automaton of function) */
      } /* for (state) */
      stgwrite("\n");
    } /* if (is function, used & having states) */
  } /* for (sym) */
}

/*  writetrailer
 *  Not much left of this once important function.
 *
 *  Global references: pc_stksize       (referred to only)
 *                     sc_dataalign     (referred to only)
 *                     code_idx         (altered)
 *                     glb_declared     (altered)
 */
void writetrailer(void)
{
  assert(sc_dataalign % opcodes(1) == 0);   /* alignment must be a multiple of
                                             * the opcode size */
  assert(sc_dataalign!=0);

  /* pad code to align data segment */
  if ((code_idx % sc_dataalign)!=0) {
    begcseg();
    while ((code_idx % sc_dataalign)!=0)
      nooperation();
  } /* if */

  /* pad data segment to align the stack and the heap */
  assert(litidx==0);            /* literal queue should have been emptied */
  assert(sc_dataalign % sizeof(cell) == 0);
  if (((glb_declared*sizeof(cell)) % sc_dataalign)!=0) {
    begdseg();
    defstorage();
    while (((glb_declared*sizeof(cell)) % sc_dataalign)!=0) {
      stgwrite("0 ");
      glb_declared++;
    } /* while */
  } /* if */

  stgwrite("\nSTKSIZE ");       /* write stack size (align stack top) */
  outval(pc_stksize - (pc_stksize % sc_dataalign), TRUE);
}

/*
 *  Start (or restart) the CODE segment.
 *
 *  In fact, the code and data segment specifiers are purely informational;
 *  the "DUMP" instruction itself already specifies that the following values
 *  should go to the data segment. All other instructions go to the code
 *  segment.
 *
 *  Global references: curseg
 *                     fcurrent
 */
void begcseg(void)
{
  if (sc_status!=statSKIP && (curseg!=sIN_CSEG || fcurrent!=fcurseg)) {
    stgwrite("\n");
    stgwrite("CODE ");
    outval(fcurrent,FALSE);
    stgwrite("\t; ");
    outval(code_idx,TRUE);
    curseg=sIN_CSEG;
    fcurseg=fcurrent;
  } /* endif */
}

/*
 *  Start (or restart) the DATA segment.
 *
 *  Global references: curseg
 */
void begdseg(void)
{
  if (sc_status!=statSKIP && (curseg!=sIN_DSEG || fcurrent!=fcurseg)) {
    stgwrite("\n");
    stgwrite("DATA ");
    outval(fcurrent,FALSE);
    stgwrite("\t; ");
    outval((glb_declared-litidx)*sizeof(cell),TRUE);
    curseg=sIN_DSEG;
    fcurseg=fcurrent;
  } /* if */
}

void setline(int chkbounds)
{
  if (sc_asmfile) {
    stgwrite("\t; line ");
    outval(fline,TRUE);
  } /* if */
  if ((sc_debug & sSYMBOLIC)!=0 || (chkbounds && (sc_debug & sCHKBOUNDS)!=0)) {
    /* generate a "break" (start statement) opcode rather than a "line" opcode
     * because earlier versions of Small/Pawn have an incompatible version of the
     * line opcode
     */
    stgwrite("\tbreak\t; ");
    outval(code_idx,TRUE);
    code_idx+=opcodes(1);
  } /* if */
}

void setfiledirect(char *name)
{
  if (sc_status==statFIRST && sc_listing) {
    assert(name!=NULL);
    pc_writeasm(outf,"#file ");
    pc_writeasm(outf,name);
    pc_writeasm(outf,"\n");
  } /* if */
}

void setlinedirect(int line)
{
  if (sc_status==statFIRST && sc_listing) {
    char string[40];
    sprintf(string,"#line %d\n",line);
    pc_writeasm(outf,string);
  } /* if */
}

/*  setlabel
 *
 *  Post a code label (specified as a number), on a new line.
 */
void setlabel(int number)
{
  assert(number>=0);
  stgwrite("l.");
  stgwrite((char *)itoh(number));
  /* To assist verification of the assembled code, put the address of the
   * label as a comment. However, labels that occur inside an expression
   * may move (through optimization or through re-ordering). So write the
   * address only if it is known to accurate.
   */
  if (!staging) {
    stgwrite("\t\t; ");
    outval(code_idx,FALSE);
  } /* if */
  stgwrite("\n");
}

/* Write a token that signifies the start or end of an expression or special
 * statement. This allows several simple optimizations by the peephole
 * optimizer.
 */
void markexpr(optmark type,const char *name,cell offset)
{
  switch (type) {
  case sEXPR:
    stgwrite("\t;$exp\n");
    break;
  case sPARM:
    stgwrite("\t;$par\n");
    break;
  case sLDECL:
    assert(name!=NULL);
    stgwrite("\t;$lcl ");
    stgwrite(name);
    stgwrite(" ");
    outval(offset,TRUE);
    break;
  default:
    assert(0);
  } /* switch */
}

/*  startfunc   - declare a CODE entry point (function start)
 *
 *  Global references: funcstatus  (referred to only)
 */
void startfunc(char *fname)
{
  stgwrite("\tproc");
  if (sc_asmfile) {
    char symname[2*sNAMEMAX+16];
    funcdisplayname(symname,fname);
    stgwrite("\t; ");
    stgwrite(symname);
  } /* if */
  stgwrite("\n");
  code_idx+=opcodes(1);
}

/*  endfunc
 *
 *  Declare a CODE ending point (function end)
 */
void endfunc(void)
{
  stgwrite("\n");       /* skip a line */
}

/*  alignframe
 *
 *  Aligns the frame (and the stack) of the current function to a multiple
 *  of the specified byte count. Two caveats: the alignment ("numbytes") should
 *  be a power of 2, and this alignment must be done right after the frame
 *  is set up (before the first variable is declared)
 */
void alignframe(int numbytes)
{
  #if !defined NDEBUG
    /* "numbytes" should be a power of 2 for this code to work */
    int i,count=0;
    for (i=0; i<sizeof numbytes*8; i++)
      if (numbytes & (1 << i))
        count++;
    assert(count==1);
  #endif

  stgwrite("\tlctrl 4\n");      /* get STK in PRI */
  stgwrite("\tconst.alt ");     /* get ~(numbytes-1) in ALT */
  outval(~(numbytes-1),TRUE);
  stgwrite("\tand\n");          /* PRI = STK "and" ~(numbytes-1) */
  stgwrite("\tsctrl 4\n");      /* set the new value of STK ... */
  stgwrite("\tsctrl 5\n");      /* ... and FRM */
  code_idx+=opcodes(5)+opargs(4);
}

/*  rvalue
 *
 *  Generate code to get the value of a symbol into "primary".
 */
void rvalue(value *lval)
{
  symbol *sym;

  sym=lval->sym;
  if (lval->ident==iARRAYCELL) {
    /* indirect fetch, address already in PRI */
    load_i();
  } else if (lval->ident==iARRAYCHAR) {
    /* indirect fetch of a character from a pack, address already in PRI */
    stgwrite("\tlodb.i ");
    outval(sCHARBITS/8,TRUE);   /* read one or two bytes */
    code_idx+=opcodes(1)+opargs(1);
  } else if (lval->ident==iREFERENCE) {
    /* indirect fetch, but address not yet in PRI */
    assert(sym!=NULL);
    assert(sym->vclass==sLOCAL);/* global references don't exist in Pawn */
    if (sym->vclass==sLOCAL)
      stgwrite("\tlref.s.pri ");
    else
      stgwrite("\tlref.pri ");
    outval(sym->addr,TRUE);
    markusage(sym,uREAD);
    code_idx+=opcodes(1)+opargs(1);
  } else if (lval->ident==iACCESSOR) {
    invoke_getter(lval->accessor);
    lval->ident=iEXPRESSION;
    lval->accessor=NULL;
  } else {
    /* direct or stack relative fetch */
    assert(sym!=NULL);
    if (sym->vclass==sLOCAL)
      stgwrite("\tload.s.pri ");
    else
      stgwrite("\tload.pri ");
    outval(sym->addr,TRUE);
    markusage(sym,uREAD);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

// Wrapper that automatically markes lvalues as decayed if they are accessors,
// since it is illegal to evaluate them twice.
void rvalue(svalue *sval)
{
  int ident = sval->val.ident;
  rvalue(&sval->val);
  if (ident == iACCESSOR)
    sval->lvalue = FALSE;
}

/* Get the address of a symbol into the primary or alternate register (used
 * for arrays, and for passing arguments by reference).
 */
void address(symbol *sym,regid reg)
{
  assert(sym!=NULL);
  assert(reg==sPRI || reg==sALT);
  /* the symbol can be a local array, a global array, or an array
   * that is passed by reference.
   */
  if (sym->ident==iREFARRAY || sym->ident==iREFERENCE) {
    /* reference to a variable or to an array; currently this is
     * always a local variable */
    switch (reg) {
    case sPRI:
      stgwrite("\tload.s.pri ");
      break;
    case sALT:
      stgwrite("\tload.s.alt ");
      break;
    } /* switch */
  } else {
    /* a local array or local variable */
    switch (reg) {
    case sPRI:
      if (sym->vclass==sLOCAL)
        stgwrite("\taddr.pri ");
      else
        stgwrite("\tconst.pri ");
      break;
    case sALT:
      if (sym->vclass==sLOCAL)
        stgwrite("\taddr.alt ");
      else
        stgwrite("\tconst.alt ");
      break;
    } /* switch */
  } /* if */
  outval(sym->addr,TRUE);
  markusage(sym,uREAD);
  code_idx+=opcodes(1)+opargs(1);
}

static void addr_reg(int val, regid reg)
{
  if (reg == sPRI)
    stgwrite("\taddr.pri ");
  else
    stgwrite("\taddr.alt ");
  outval(val, TRUE);
  code_idx += opcodes(1) + opargs(1);
}

// Load the number of arguments into PRI. Frame layout:
//   base + 0*sizeof(cell) == previous "base"
//   base + 1*sizeof(cell) == function return address
//   base + 2*sizeof(cell) == number of arguments
//   base + 3*sizeof(cell) == first argument of the function
static void load_argcount(regid reg)
{
  if (reg == sPRI)
    stgwrite("\tload.s.pri ");
  else
    stgwrite("\tload.s.alt ");
  outval(2 * sizeof(cell), TRUE);
  code_idx += opcodes(1) + opargs(1);
}

// PRI = ALT + (PRI * cellsize)
void idxaddr()
{
  stgwrite("\tidxaddr\n");
  code_idx += opcodes(1);
}

void load_i()
{
  stgwrite("\tload.i\n");
  code_idx+=opcodes(1);
}

// Load the hidden array argument into ALT.
void load_hidden_arg()
{
  pushreg(sPRI);

  // Compute an address to the first argument, then add the argument count
  // to find the address after the final argument:
  //    addr.alt   0xc   ; Compute &first_arg
  //    load.s.alt 0x8   ; Load arg count
  //    idxaddr          ; Compute (&first_arg) + argcount
  //    load.i           ; Load *(&first_arg + argcount)
  //    move.alt         ; Move result into ALT.
  addr_reg(0xc, sALT);
  load_argcount(sPRI);
  idxaddr();
  load_i();
  move_alt();

  popreg(sPRI);
}

/*  store
 *
 *  Saves the contents of "primary" into a memory cell, either directly
 *  or indirectly (at the address given in the alternate register).
 */
void store(value *lval)
{
  symbol *sym;

  sym=lval->sym;
  if (lval->ident==iARRAYCELL) {
    /* store at address in ALT */
    stgwrite("\tstor.i\n");
    code_idx+=opcodes(1);
  } else if (lval->ident==iARRAYCHAR) {
    /* store at address in ALT */
    stgwrite("\tstrb.i ");
    outval(sCHARBITS/8,TRUE);   /* write one or two bytes */
    code_idx+=opcodes(1)+opargs(1);
  } else if (lval->ident==iREFERENCE) {
    assert(sym!=NULL);
    if (sym->vclass==sLOCAL)
      stgwrite("\tsref.s.pri ");
    else
      stgwrite("\tsref.pri ");
    outval(sym->addr,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } else if (lval->ident==iACCESSOR) {
    invoke_setter(lval->accessor, TRUE);
  } else {
    assert(sym!=NULL);
    markusage(sym,uWRITTEN);
    if (sym->vclass==sLOCAL)
      stgwrite("\tstor.s.pri ");
    else
      stgwrite("\tstor.pri ");
    outval(sym->addr,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/* Get a cell from a fixed address in memory */
void loadreg(cell address,regid reg)
{
  assert(reg==sPRI || reg==sALT);
  if (reg==sPRI)
    stgwrite("\tload.pri ");
  else
    stgwrite("\tload.alt ");
  outval(address,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/* Store a cell into a fixed address in memory */
void storereg(cell address,regid reg)
{
  assert(reg==sPRI || reg==sALT);
  if (reg==sPRI)
    stgwrite("\tstor.pri ");
  else
    stgwrite("\tstor.alt ");
  outval(address,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/* source must in PRI, destination address in ALT. The "size"
 * parameter is in bytes, not cells.
 */
void memcopy(cell size)
{
  stgwrite("\tmovs ");
  outval(size,TRUE);

  code_idx+=opcodes(1)+opargs(1);
}

/* Address of the source must already have been loaded in PRI
 * "size" is the size in bytes (not cells).
 */
void copyarray(symbol *sym,cell size)
{
  assert(sym!=NULL);
  /* the symbol can be a local array, a global array, or an array
   * that is passed by reference.
   */
  if (sym->ident==iREFARRAY) {
    /* reference to an array; currently this is always a local variable */
    assert(sym->vclass==sLOCAL);        /* symbol must be stack relative */
    stgwrite("\tload.s.alt ");
  } else {
    /* a local or global array */
    if (sym->vclass==sLOCAL)
      stgwrite("\taddr.alt ");
    else
      stgwrite("\tconst.alt ");
  } /* if */
  outval(sym->addr,TRUE);
  markusage(sym,uWRITTEN);

  code_idx+=opcodes(1)+opargs(1);
  memcopy(size);
}

void fillarray(symbol *sym,cell size,cell value)
{
  ldconst(value,sPRI);  /* load value in PRI */

  assert(sym!=NULL);
  /* the symbol can be a local array, a global array, or an array
   * that is passed by reference.
   */
  if (sym->ident==iREFARRAY) {
    /* reference to an array; currently this is always a local variable */
    assert(sym->vclass==sLOCAL);        /* symbol must be stack relative */
    stgwrite("\tload.s.alt ");
  } else {
    /* a local or global array */
    if (sym->vclass==sLOCAL)
      stgwrite("\taddr.alt ");
    else
      stgwrite("\tconst.alt ");
  } /* if */
  outval(sym->addr,TRUE);
  markusage(sym,uWRITTEN);

  assert(size>0);
  stgwrite("\tfill ");
  outval(size,TRUE);

  code_idx+=opcodes(2)+opargs(2);
}

void stradjust(regid reg)
{
  assert(reg==sPRI);
  stgwrite("\tstradjust.pri\n");
  code_idx+=opcodes(1);
}

/* Instruction to get an immediate value into the primary or the alternate
 * register
 */
void ldconst(cell val,regid reg)
{
  assert(reg==sPRI || reg==sALT);
  switch (reg) {
  case sPRI:
    if (val==0) {
      stgwrite("\tzero.pri\n");
      code_idx+=opcodes(1);
    } else {
      stgwrite("\tconst.pri ");
      outval(val, TRUE);
      code_idx+=opcodes(1)+opargs(1);
    } /* if */
    break;
  case sALT:
    if (val==0) {
      stgwrite("\tzero.alt\n");
      code_idx+=opcodes(1);
    } else {
      stgwrite("\tconst.alt ");
      outval(val, TRUE);
      code_idx+=opcodes(1)+opargs(1);
    } /* if */
    break;
  } /* switch */
}

/* Copy value in alternate register to the primary register */
void moveto1(void)
{
  stgwrite("\tmove.pri\n");
  code_idx+=opcodes(1)+opargs(0);
}

void move_alt(void)
{
  stgwrite("\tmove.alt\n");
  code_idx+=opcodes(1)+opargs(0);
}

/* Push primary or the alternate register onto the stack
 */
void pushreg(regid reg)
{
  assert(reg==sPRI || reg==sALT);
  switch (reg) {
  case sPRI:
    stgwrite("\tpush.pri\n");
    break;
  case sALT:
    stgwrite("\tpush.alt\n");
    break;
  } /* switch */
  code_idx+=opcodes(1);
}

/*
 *  Push a constant value onto the stack
 */
void pushval(cell val)
{
  stgwrite("\tpush.c ");
  outval(val, TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/* Pop stack into the primary or the alternate register
 */
void popreg(regid reg)
{
  assert(reg==sPRI || reg==sALT);
  switch (reg) {
  case sPRI:
    stgwrite("\tpop.pri\n");
    break;
  case sALT:
    stgwrite("\tpop.alt\n");
    break;
  } /* switch */
  code_idx+=opcodes(1);
}

/*
 * Generate an array
 *   stk -= dims
 *   [stk] = hea
 *   stk += 1
 *   hea += 1 + (# cells in array)
 */
void genarray(int dims, int _autozero)
{
  if (_autozero) {
    stgwrite("\tgenarray.z ");
  } else {
    stgwrite("\tgenarray ");
  }
  outval(dims, TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/*
 *  swap the top-of-stack with the value in primary register
 */
void swap1(void)
{
  stgwrite("\tswap.pri\n");
  code_idx+=opcodes(1);
}

/* Switch statements
 * The "switch" statement generates a "case" table using the "CASE" opcode.
 * The case table contains a list of records, each record holds a comparison
 * value and a label to branch to on a match. The very first record is an
 * exception: it holds the size of the table (excluding the first record) and
 * the label to branch to when none of the values in the case table match.
 * The case table is sorted on the comparison value. This allows more advanced
 * abstract machines to sift the case table with a binary search.
 */
void ffswitch(int label)
{
  stgwrite("\tswitch ");
  outval(label,TRUE);           /* the label is the address of the case table */
  code_idx+=opcodes(1)+opargs(1);
}

void ffcase(cell value,char *labelname,int newtable)
{
  if (newtable) {
    stgwrite("\tcasetbl\n");
    code_idx+=opcodes(1);
  } /* if */
  stgwrite("\tcase ");
  outval(value,FALSE);
  stgwrite(" ");
  stgwrite(labelname);
  stgwrite("\n");
  code_idx+=opcodes(0)+opargs(2);
}

/*
 *  Call specified function
 */
void ffcall(symbol *sym,const char *label,int numargs)
{
  char symname[2*sNAMEMAX+16];
  char aliasname[sNAMEMAX+1];
  int wasAlias = 0;

  assert(sym!=NULL);
  assert(sym->ident==iFUNCTN);
  if (sc_asmfile)
    funcdisplayname(symname,sym->name);
  if ((sym->usage & uNATIVE)!=0) {
    /* reserve a SYSREQ id if called for the first time */
    assert(label==NULL);
    stgwrite("\tsysreq.c ");
    if (sc_status==statWRITE && (sym->usage & uREAD)==0 && sym->addr>=0)
      sym->addr=ntv_funcid++;
    /* Look for an alias */
    if (lookup_alias(aliasname, sym->name)) {
      symbol *asym = findglb(aliasname, sGLOBAL);
      if (asym && asym->ident==iFUNCTN && ((sym->usage & uNATIVE) != 0)) {
        sym = asym;
        if (sc_status==statWRITE && (sym->usage & uREAD)==0 && sym->addr>=0) {
          sym->addr=ntv_funcid++;
          markusage(sym, uREAD);
        }
      }
    }
    outval(sym->addr,FALSE);
    if (sc_asmfile) {
      stgwrite("\t; ");
      stgwrite(symname);
    } /* if */
    stgwrite("\n"); /* write on a separate line, to mark a sequence point for the peephole optimizer */
    stgwrite("\tstack ");
    outval((numargs+1)*sizeof(cell), TRUE);
    code_idx+=opcodes(2)+opargs(2);
  } else {
    /* normal function */
    stgwrite("\tcall ");
    if (label!=NULL) {
      stgwrite("l.");
      stgwrite(label);
    } else {
      stgwrite(sym->name);
    } /* if */
    if (sc_asmfile
        && (label!=NULL || (!isalpha(sym->name[0]) && sym->name[0]!='_'  && sym->name[0]!=sc_ctrlchar)))
    {
      stgwrite("\t; ");
      stgwrite(symname);
    } /* if */
    stgwrite("\n");
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/*  Return from function
 *
 *  Global references: funcstatus  (referred to only)
 */
void ffret(int remparams)
{
  if (remparams)
    stgwrite("\tretn\n");
  else
    stgwrite("\tret\n");
  code_idx+=opcodes(1);
}

void ffabort(int reason)
{
  stgwrite("\thalt ");
  outval(reason,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

void ffbounds(cell size)
{
  if ((sc_debug & sCHKBOUNDS)!=0) {
    stgwrite("\tbounds ");
    outval(size,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/*
 *  Jump to local label number (the number is converted to a name)
 */
void jumplabel(int number)
{
  stgwrite("\tjump ");
  outval(number,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/*
 *   Define storage (global and static variables)
 */
void defstorage(void)
{
  stgwrite("dump ");
}

/*
 *  Inclrement/decrement stack pointer. Note that this routine does
 *  nothing if the delta is zero.
 */
void modstk(int delta)
{
  if (delta) {
    stgwrite("\tstack ");
    outval(delta, TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/* set the stack to a hard offset from the frame */
void setstk(cell value)
{
  stgwrite("\tstackadjust ");
  assert(value<=0);             /* STK should always become <= FRM */
  outval(value, TRUE);        /* add (negative) offset */
  code_idx+=opcodes(1)+opargs(1);
}

void modheap(int delta)
{
  if (delta) {
    stgwrite("\theap ");
    outval(delta, TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

void modheap_i()
{
  stgwrite("\ttracker.pop.setheap\n");
  code_idx+=opcodes(1);
}

void setheap_save(cell value)
{
  assert(value);
  stgwrite("\ttracker.push.c ");
  outval(value, TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

void setheap_pri(void)
{
  stgwrite("\theap ");          /* ALT = HEA++ */
  outval(sizeof(cell), TRUE);
  stgwrite("\tstor.i\n");       /* store PRI (default value) at address ALT */
  stgwrite("\tmove.pri\n");     /* move ALT to PRI: PRI contains the address */
  code_idx+=opcodes(3)+opargs(1);
}

void setheap(cell value)
{
  stgwrite("\tconst.pri ");     /* load default value in PRI */
  outval(value, TRUE);
  code_idx+=opcodes(1)+opargs(1);
  setheap_pri();
}

/*
 *  Convert a cell number to a "byte" address; i.e. double or quadruple
 *  the primary register.
 */
void cell2addr(void)
{
  #if PAWN_CELL_SIZE==16
    stgwrite("\tshl.c.pri 1\n");
  #elif PAWN_CELL_SIZE==32
    stgwrite("\tshl.c.pri 2\n");
  #elif PAWN_CELL_SIZE==64
    stgwrite("\tshl.c.pri 3\n");
  #else
    #error Unsupported cell size
  #endif
  code_idx+=opcodes(1)+opargs(1);
}

/*
 *  Double or quadruple the alternate register.
 */
void cell2addr_alt(void)
{
  #if PAWN_CELL_SIZE==16
    stgwrite("\tshl.c.alt 1\n");
  #elif PAWN_CELL_SIZE==32
    stgwrite("\tshl.c.alt 2\n");
  #elif PAWN_CELL_SIZE==64
    stgwrite("\tshl.c.alt 3\n");
  #else
    #error Unsupported cell size
  #endif
  code_idx+=opcodes(1)+opargs(1);
}

/*
 *  Convert "distance of addresses" to "number of cells" in between.
 *  Or convert a number of packed characters to the number of cells (with
 *  truncation).
 */
void addr2cell(void)
{
  #if PAWN_CELL_SIZE==16
    stgwrite("\tshr.c.pri 1\n");
  #elif PAWN_CELL_SIZE==32
    stgwrite("\tshr.c.pri 2\n");
  #elif PAWN_CELL_SIZE==64
    stgwrite("\tshr.c.pri 3\n");
  #else
    #error Unsupported cell size
  #endif
  code_idx+=opcodes(1)+opargs(1);
}

/* Convert from character index to byte address. This routine does
 * nothing if a character has the size of a byte.
 */
void char2addr(void)
{
  #if sCHARBITS==16
    stgwrite("\tshl.c.pri 1\n");
    code_idx+=opcodes(1)+opargs(1);
  #endif
}

/* Align PRI (which should hold a character index) to an address.
 * The first character in a "pack" occupies the highest bits of
 * the cell. This is at the lower memory address on Big Endian
 * computers and on the higher address on Little Endian computers.
 * The ALIGN.pri/alt instructions must solve this machine dependence;
 * that is, on Big Endian computers, ALIGN.pri/alt shuold do nothing
 * and on Little Endian computers they should toggle the address.
 *
 * NOTE: For Source Pawn, this is fliped.  It will do nothing on Little-Endian.
 */
void charalign(void)
{
#if 0	/* TEMPORARILY DISABLED BECAUSE WE DON'T USE BIG ENDIAN */
  stgwrite("\talign.pri ");
  outval(sCHARBITS/8,TRUE);
  code_idx+=opcodes(1)+opargs(1);
#endif
}

/*
 *  Add a constant to the primary register.
 */
void addconst(cell value)
{
  if (value!=0) {
    stgwrite("\tadd.c ");
    outval(value,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/*
 *  signed multiply of primary and secundairy registers (result in primary)
 */
void os_mult(void)
{
  stgwrite("\tsmul\n");
  code_idx+=opcodes(1);
}

/*
 *  signed divide of alternate register by primary register (quotient in
 *  primary; remainder in alternate)
 */
void os_div(void)
{
  stgwrite("\tsdiv.alt\n");
  code_idx+=opcodes(1);
}

/*
 *  modulus of (alternate % primary), result in primary (signed)
 */
void os_mod(void)
{
  stgwrite("\tsdiv.alt\n");
  stgwrite("\tmove.pri\n");     /* move ALT to PRI */
  code_idx+=opcodes(2);
}

/*
 *  Add primary and alternate registers (result in primary).
 */
void ob_add(void)
{
  stgwrite("\tadd\n");
  code_idx+=opcodes(1);
}

/*
 *  subtract primary register from alternate register (result in primary)
 */
void ob_sub(void)
{
  stgwrite("\tsub.alt\n");
  code_idx+=opcodes(1);
}

/*
 *  arithmic shift left alternate register the number of bits
 *  given in the primary register (result in primary).
 *  There is no need for a "logical shift left" routine, since
 *  logical shift left is identical to arithmic shift left.
 */
void ob_sal(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tshl\n");
  code_idx+=opcodes(2);
}

/*
 *  arithmic shift right alternate register the number of bits
 *  given in the primary register (result in primary).
 */
void os_sar(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tsshr\n");
  code_idx+=opcodes(2);
}

/*
 *  logical (unsigned) shift right of the alternate register by the
 *  number of bits given in the primary register (result in primary).
 */
void ou_sar(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tshr\n");
  code_idx+=opcodes(2);
}

/*
 *  inclusive "or" of primary and alternate registers (result in primary)
 */
void ob_or(void)
{
  stgwrite("\tor\n");
  code_idx+=opcodes(1);
}

/*
 *  "exclusive or" of primary and alternate registers (result in primary)
 */
void ob_xor(void)
{
  stgwrite("\txor\n");
  code_idx+=opcodes(1);
}

/*
 *  "and" of primary and secundairy registers (result in primary)
 */
void ob_and(void)
{
  stgwrite("\tand\n");
  code_idx+=opcodes(1);
}

/*
 *  test ALT==PRI; result in primary register (1 or 0).
 */
void ob_eq(void)
{
  stgwrite("\teq\n");
  code_idx+=opcodes(1);
}

/*
 *  test ALT!=PRI
 */
void ob_ne(void)
{
  stgwrite("\tneq\n");
  code_idx+=opcodes(1);
}

/* The abstract machine defines the relational instructions so that PRI is
 * on the left side and ALT on the right side of the operator. For example,
 * SLESS sets PRI to either 1 or 0 depending on whether the expression
 * "PRI < ALT" is true.
 *
 * The compiler generates comparisons with ALT on the left side of the
 * relational operator and PRI on the right side. The XCHG instruction
 * prefixing the relational operators resets this. We leave it to the
 * peephole optimizer to choose more compact instructions where possible.
 */

/* Relational operator prefix for chained relational expressions. The
 * "suffix" code restores the stack.
 * For chained relational operators, the goal is to keep the comparison
 * result "so far" in PRI and the value of the most recent operand in
 * ALT, ready for a next comparison.
 * The "prefix" instruction pushed the comparison result (PRI) onto the
 * stack and moves the value of ALT into PRI. If there is a next comparison,
 * PRI can now serve as the "left" operand of the relational operator.
 */
void relop_prefix(void)
{
  stgwrite("\tpush.pri\n");
  stgwrite("\tmove.pri\n");
  code_idx+=opcodes(2);
}

void relop_suffix(void)
{
  stgwrite("\tswap.alt\n");
  stgwrite("\tand\n");
  stgwrite("\tpop.alt\n");
  code_idx+=opcodes(3);
}

/*
 *  test ALT<PRI (signed)
 */
void os_lt(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tsless\n");
  code_idx+=opcodes(2);
}

/*
 *  test ALT<=PRI (signed)
 */
void os_le(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tsleq\n");
  code_idx+=opcodes(2);
}

/*
 *  test ALT>PRI (signed)
 */
void os_gt(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tsgrtr\n");
  code_idx+=opcodes(2);
}

/*
 *  test ALT>=PRI (signed)
 */
void os_ge(void)
{
  stgwrite("\txchg\n");
  stgwrite("\tsgeq\n");
  code_idx+=opcodes(2);
}

/*
 *  logical negation of primary register
 */
void lneg(void)
{
  stgwrite("\tnot\n");
  code_idx+=opcodes(1);
}

/*
 *  two's complement primary register
 */
void neg(void)
{
  stgwrite("\tneg\n");
  code_idx+=opcodes(1);
}

/*
 *  one's complement of primary register
 */
void invert(void)
{
  stgwrite("\tinvert\n");
  code_idx+=opcodes(1);
}

/*
 *  nop
 */
void nooperation(void)
{
  stgwrite("\tnop\n");
  code_idx+=opcodes(1);
}

void inc_pri()
{
  stgwrite("\tinc.pri\n");
  code_idx+=opcodes(1);
}

void dec_pri()
{
  stgwrite("\tdec.pri\n");
  code_idx+=opcodes(1);
}

/*  increment symbol
 */
void inc(value *lval)
{
  symbol *sym;

  sym=lval->sym;
  if (lval->ident==iARRAYCELL) {
    /* indirect increment, address already in PRI */
    stgwrite("\tinc.i\n");
    code_idx+=opcodes(1);
  } else if (lval->ident==iARRAYCHAR) {
    /* indirect increment of single character, address already in PRI */
    stgwrite("\tpush.pri\n");
    stgwrite("\tpush.alt\n");
    stgwrite("\tmove.alt\n");   /* copy address */
    stgwrite("\tlodb.i ");      /* read from PRI into PRI */
    outval(sCHARBITS/8,TRUE);   /* read one or two bytes */
    stgwrite("\tinc.pri\n");
    stgwrite("\tstrb.i ");      /* write PRI to ALT */
    outval(sCHARBITS/8,TRUE);   /* write one or two bytes */
    stgwrite("\tpop.alt\n");
    stgwrite("\tpop.pri\n");
    code_idx+=opcodes(8)+opargs(2);
  } else if (lval->ident==iREFERENCE) {
    assert(sym!=NULL);
    stgwrite("\tpush.pri\n");
    /* load dereferenced value */
    assert(sym->vclass==sLOCAL);    /* global references don't exist in Pawn */
    if (sym->vclass==sLOCAL)
      stgwrite("\tlref.s.pri ");
    else
      stgwrite("\tlref.pri ");
    outval(sym->addr,TRUE);
    /* increment */
    stgwrite("\tinc.pri\n");
    /* store dereferenced value */
    if (sym->vclass==sLOCAL)
      stgwrite("\tsref.s.pri ");
    else
      stgwrite("\tsref.pri ");
    outval(sym->addr,TRUE);
    stgwrite("\tpop.pri\n");
    code_idx+=opcodes(5)+opargs(2);
  } else {
    /* local or global variable */
    assert(sym!=NULL);
    if (sym->vclass==sLOCAL)
      stgwrite("\tinc.s ");
    else
      stgwrite("\tinc ");
    outval(sym->addr,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/*  decrement symbol
 *
 *  in case of an integer pointer, the symbol must be incremented by 2.
 */
void dec(value *lval)
{
  symbol *sym;

  sym=lval->sym;
  if (lval->ident==iARRAYCELL) {
    /* indirect decrement, address already in PRI */
    stgwrite("\tdec.i\n");
    code_idx+=opcodes(1);
  } else if (lval->ident==iARRAYCHAR) {
    /* indirect decrement of single character, address already in PRI */
    stgwrite("\tpush.pri\n");
    stgwrite("\tpush.alt\n");
    stgwrite("\tmove.alt\n");   /* copy address */
    stgwrite("\tlodb.i ");      /* read from PRI into PRI */
    outval(sCHARBITS/8,TRUE);   /* read one or two bytes */
    stgwrite("\tdec.pri\n");
    stgwrite("\tstrb.i ");      /* write PRI to ALT */
    outval(sCHARBITS/8,TRUE);   /* write one or two bytes */
    stgwrite("\tpop.alt\n");
    stgwrite("\tpop.pri\n");
    code_idx+=opcodes(8)+opargs(2);
  } else if (lval->ident==iREFERENCE) {
    assert(sym!=NULL);
    stgwrite("\tpush.pri\n");
    /* load dereferenced value */
    assert(sym->vclass==sLOCAL);    /* global references don't exist in Pawn */
    if (sym->vclass==sLOCAL)
      stgwrite("\tlref.s.pri ");
    else
      stgwrite("\tlref.pri ");
    outval(sym->addr,TRUE);
    /* decrement */
    stgwrite("\tdec.pri\n");
    /* store dereferenced value */
    if (sym->vclass==sLOCAL)
      stgwrite("\tsref.s.pri ");
    else
      stgwrite("\tsref.pri ");
    outval(sym->addr,TRUE);
    stgwrite("\tpop.pri\n");
    code_idx+=opcodes(5)+opargs(2);
  } else {
    /* local or global variable */
    assert(sym!=NULL);
    if (sym->vclass==sLOCAL)
      stgwrite("\tdec.s ");
    else
      stgwrite("\tdec ");
    outval(sym->addr,TRUE);
    code_idx+=opcodes(1)+opargs(1);
  } /* if */
}

/*
 *  Jumps to "label" if PRI != 0
 */
void jmp_ne0(int number)
{
  stgwrite("\tjnz ");
  outval(number,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/*
 *  Jumps to "label" if PRI == 0
 */
void jmp_eq0(int number)
{
  stgwrite("\tjzer ");
  outval(number,TRUE);
  code_idx+=opcodes(1)+opargs(1);
}

/* write a value in hexadecimal; optionally adds a newline */
void outval(cell val,int newline)
{
  stgwrite(itoh(val));
  if (newline)
    stgwrite("\n");
}

void invoke_getter(methodmap_method_t *method)
{
  if (!method->getter) {
    error(149, method->name);
    return;
  }

  // push.c 1
  // sysreq.c N 1
  // stack 8
  pushreg(sPRI);
  pushval(1);
  ffcall(method->getter, NULL, 1);

  if (sc_status != statSKIP)
    markusage(method->getter, uREAD);
}

void invoke_setter(methodmap_method_t *method, int save)
{
  if (!method->setter) {
    error(152, method->name);
    return;
  }

  if (save)
    pushreg(sPRI);
  pushreg(sPRI);
  pushreg(sALT);
  pushval(2);
  ffcall(method->setter, NULL, 2);
  if (save)
    popreg(sPRI);

  if (sc_status != statSKIP)
    markusage(method->setter, uREAD);
}
