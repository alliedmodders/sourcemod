/*  Pawn compiler - Recursive descend expresion parser
 *
 *  Copyright (c) ITB CompuPhase, 1997-2005
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
 *      appreciated but is not reeq;quired.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id: sc3.c 3635 2006-08-13 12:19:41Z thiadmer $
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>     /* for _MAX_PATH */
#include <string.h>
#if defined FORTIFY
  #include <alloc/fortify.h>
#endif
#include "sc.h"
#include "sctracker.h"

static int skim(int *opstr,void (*testfunc)(int),int dropval,int endval,
                int (*hier)(value*),value *lval);
static void dropout(int lvalue,void (*testfunc)(int val),int exit1,value *lval);
static int plnge(int *opstr,int opoff,int (*hier)(value *lval),value *lval,
                 char *forcetag,int chkbitwise);
static int plnge1(int (*hier)(value *lval),value *lval);
static void plnge2(void (*oper)(void),
                   int (*hier)(value *lval),
                   value *lval1,value *lval2);
static cell calc(cell left,void (*oper)(),cell right,char *boolresult);
static int hier14(value *lval);
static int hier13(value *lval);
static int hier12(value *lval);
static int hier11(value *lval);
static int hier10(value *lval);
static int hier9(value *lval);
static int hier8(value *lval);
static int hier7(value *lval);
static int hier6(value *lval);
static int hier5(value *lval);
static int hier4(value *lval);
static int hier3(value *lval);
static int hier2(value *lval);
static int hier1(value *lval1);
static int primary(value *lval);
static void clear_value(value *lval);
static void callfunction(symbol *sym,value *lval_result,int matchparanthesis);
static int dbltest(void (*oper)(),value *lval1,value *lval2);
static int commutative(void (*oper)());
static int constant(value *lval);

static char lastsymbol[sNAMEMAX+1]; /* name of last function/variable */
static int bitwise_opercount;   /* count of bitwise operators in an expression */

/* Function addresses of binary operators for signed operations */
static void (*op1[17])(void) = {
  os_mult,os_div,os_mod,        /* hier3, index 0 */
  ob_add,ob_sub,                /* hier4, index 3 */
  ob_sal,os_sar,ou_sar,         /* hier5, index 5 */
  ob_and,                       /* hier6, index 8 */
  ob_xor,                       /* hier7, index 9 */
  ob_or,                        /* hier8, index 10 */
  os_le,os_ge,os_lt,os_gt,      /* hier9, index 11 */
  ob_eq,ob_ne,                  /* hier10, index 15 */
};
/* These two functions are defined because the functions inc() and dec() in
 * SC4.C have a different prototype than the other code generation functions.
 * The arrays for user-defined functions use the function pointers for
 * identifying what kind of operation is requested; these functions must all
 * have the same prototype. As inc() and dec() are special cases already, it
 * is simplest to add two "do-nothing" functions.
 */
static void user_inc(void) {}
static void user_dec(void) {}

/*
 *  Searches for a binary operator a list of operators. The list is stored in
 *  the array "list". The last entry in the list should be set to 0.
 *
 *  The index of an operator in "list" (if found) is returned in "opidx". If
 *  no operator is found, nextop() returns 0.
 *
 *  If an operator is found in the expression, it cannot be used in a function
 *  call with omitted parantheses. Mark this...
 *
 *  Global references: sc_allowproccall   (modified)
 */
static int nextop(int *opidx,int *list)
{
  *opidx=0;
  while (*list){
    if (matchtoken(*list)){
      sc_allowproccall=FALSE;
      return TRUE;      /* found! */
    } else {
      list+=1;
      *opidx+=1;
    } /* if */
  } /* while */
  return FALSE;         /* entire list scanned, nothing found */
}

SC_FUNC int check_userop(void (*oper)(void),int tag1,int tag2,int numparam,
                         value *lval,int *resulttag)
{
static char *binoperstr[] = { "*", "/", "%", "+", "-", "", "", "",
                              "", "", "", "<=", ">=", "<", ">", "==", "!=" };
static int binoper_savepri[] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                                 FALSE, FALSE, FALSE, FALSE, FALSE,
                                 TRUE, TRUE, TRUE, TRUE, FALSE, FALSE };
static char *unoperstr[] = { "!", "-", "++", "--" };
static void (*unopers[])(void) = { lneg, neg, user_inc, user_dec };
  char opername[4] = "", symbolname[sNAMEMAX+1];
  int i,swapparams,savepri,savealt;
  int paramspassed;
  symbol *sym;

  /* since user-defined operators on untagged operands are forbidden, we have
   * a quick exit.
   */
  assert(numparam==1 || numparam==2);
  if (tag1==0 && (numparam==1 || tag2==0))
    return FALSE;

  savepri=savealt=FALSE;
  /* find the name with the operator */
  if (numparam==2) {
    if (oper==NULL) {
      /* assignment operator: a special case */
      strcpy(opername,"=");
      if (lval!=NULL && (lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR))
        savealt=TRUE;
    } else {
      assert( (sizeof binoperstr / sizeof binoperstr[0]) == (sizeof op1 / sizeof op1[0]) );
      for (i=0; i<sizeof op1 / sizeof op1[0]; i++) {
        if (oper==op1[i]) {
          strcpy(opername,binoperstr[i]);
          savepri=binoper_savepri[i];
          break;
        } /* if */
      } /* for */
    } /* if */
  } else {
    assert(oper!=NULL);
    assert(numparam==1);
    /* try a select group of unary operators */
    assert( (sizeof unoperstr / sizeof unoperstr[0]) == (sizeof unopers / sizeof unopers[0]) );
    if (opername[0]=='\0') {
      for (i=0; i<sizeof unopers / sizeof unopers[0]; i++) {
        if (oper==unopers[i]) {
          strcpy(opername,unoperstr[i]);
          break;
        } /* if */
      } /* for */
    } /* if */
  } /* if */
  /* if not found, quit */
  if (opername[0]=='\0')
    return FALSE;

  /* create a symbol name from the tags and the operator name */
  assert(numparam==1 || numparam==2);
  operator_symname(symbolname,opername,tag1,tag2,numparam,tag2);
  swapparams=FALSE;
  sym=findglb(symbolname,sGLOBAL);
  if (sym==NULL /*|| (sym->usage & uDEFINE)==0*/) {  /* ??? should not check uDEFINE; first pass clears these bits */
    /* check for commutative operators */
    if (tag1==tag2 || oper==NULL || !commutative(oper))
      return FALSE;             /* not commutative, cannot swap operands */
    /* if arrived here, the operator is commutative and the tags are different,
     * swap tags and try again
     */
    assert(numparam==2);        /* commutative operator must be a binary operator */
    operator_symname(symbolname,opername,tag2,tag1,numparam,tag1);
    swapparams=TRUE;
    sym=findglb(symbolname,sGLOBAL);
    if (sym==NULL /*|| (sym->usage & uDEFINE)==0*/)
      return FALSE;
  } /* if */

  /* check existance and the proper declaration of this function */
  if ((sym->usage & uMISSING)!=0 || (sym->usage & uPROTOTYPED)==0) {
    char symname[2*sNAMEMAX+16];  /* allow space for user defined operators */
    funcdisplayname(symname,sym->name);
    if ((sym->usage & uMISSING)!=0)
      error(4,symname);           /* function not defined */
    if ((sym->usage & uPROTOTYPED)==0)
      error(71,symname);          /* operator must be declared before use */
  } /* if */

  /* we don't want to use the redefined operator in the function that
   * redefines the operator itself, otherwise the snippet below gives
   * an unexpected recursion:
   *    fixed:operator+(fixed:a, fixed:b)
   *        return a + b
   */
  if (sym==curfunc)
    return FALSE;

  /* for increment and decrement operators, the symbol must first be loaded
   * (and stored back afterwards)
   */
  if (oper==user_inc || oper==user_dec) {
    assert(!savepri);
    assert(lval!=NULL);
    if (lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR)
      pushreg(sPRI);            /* save current address in PRI */
    rvalue(lval);               /* get the symbol's value in PRI */
  } /* if */

  assert(!savepri || !savealt); /* either one MAY be set, but not both */
  if (savepri) {
    /* the chained comparison operators require that the ALT register is
     * unmodified, so we save it here; actually, we save PRI because the normal
     * instruction sequence (without user operator) swaps PRI and ALT
     */
    pushreg(sPRI);              /* right-hand operand is in PRI */
  } else if (savealt) {
    /* for the assignment operator, ALT may contain an address at which the
     * result must be stored; this address must be preserved accross the
     * call
     */
    assert(lval!=NULL);         /* this was checked earlier */
    assert(lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR); /* checked earlier */
    pushreg(sALT);
  } /* if */

  /* push parameters, call the function */
  paramspassed= (oper==NULL) ? 1 : numparam;
  switch (paramspassed) {
  case 1:
    pushreg(sPRI);
    break;
  case 2:
    /* note that 1) a function expects that the parameters are pushed
     * in reversed order, and 2) the left operand is in the secondary register
     * and the right operand is in the primary register */
    if (swapparams) {
      pushreg(sALT);
      pushreg(sPRI);
    } else {
      pushreg(sPRI);
      pushreg(sALT);
    } /* if */
    break;
  default:
    assert(0);
  } /* switch */
  markexpr(sPARM,NULL,0);       /* mark the end of a sub-expression */
  pushval((cell)paramspassed /* *sizeof(cell)*/ );
  assert(sym->ident==iFUNCTN);
  ffcall(sym,NULL,paramspassed);
  if (sc_status!=statSKIP)
    markusage(sym,uREAD);       /* do not mark as "used" when this call itself is skipped */
  if ((sym->usage & uNATIVE)!=0 && sym->x.lib!=NULL)
    sym->x.lib->value += 1;     /* increment "usage count" of the library */
  sideeffect=TRUE;              /* assume functions carry out a side-effect */
  assert(resulttag!=NULL);
  *resulttag=sym->tag;          /* save tag of the called function */

  if (savepri || savealt)
    popreg(sALT);               /* restore the saved PRI/ALT that into ALT */
  if (oper==user_inc || oper==user_dec) {
    assert(lval!=NULL);
    if (lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR)
      popreg(sALT);             /* restore address (in ALT) */
    store(lval);                /* store PRI in the symbol */
    moveto1();                  /* make sure PRI is restored on exit */
  } /* if */
  return TRUE;
}

SC_FUNC int checktags_string(int tags[], int numtags, value *sym1)
{
  int i;
  if (sym1->ident == iARRAY || sym1->ident == iREFARRAY)
  {
    return FALSE;
  }
  for (i=0; i<numtags; i++) {
    if ((sym1->tag == pc_tag_string && tags[i] == 0) ||
		(sym1->tag == 0 && tags[i] == pc_tag_string))
      return TRUE;
  }
  return FALSE;
}

SC_FUNC int checktag_string(value *sym1, value *sym2)
{
  if (sym1->ident == iARRAY || sym2->ident == iARRAY
	  || sym1->ident == iREFARRAY || sym2->ident == iREFARRAY)
  {
    return FALSE;
  }
  if ((sym1->tag == pc_tag_string && sym2->tag == 0)
	  || (sym1->tag == 0 && sym2->tag == pc_tag_string))
  {
    return TRUE;
  }

  return FALSE;
}

SC_FUNC int matchtag_string(int ident, int tag)
{
  if (ident == iARRAY || ident == iREFARRAY)
    return FALSE;
  return (tag == pc_tag_string) ? TRUE : FALSE;
}

SC_FUNC int matchtag(int formaltag,int actualtag,int allowcoerce)
{
  if (formaltag!=actualtag) {
    /* if the formal tag is zero and the actual tag is not "fixed", the actual
     * tag is "coerced" to zero
     */
    if (!allowcoerce || formaltag!=0 || (actualtag & FIXEDTAG)!=0) {
		if (formaltag & FUNCTAG)
		{
			if (actualtag == pc_functag || (formaltag == pc_functag && actualtag & FUNCTAG))
			{
				return TRUE;
			} else if (actualtag & FUNCTAG) {
				constvalue *v = find_tag_byval(actualtag);
				int index;
				short usage = uPUBLIC;
				symbol *sym, *found = NULL;
				funcenum_t *e;
				functag_t *t;

				if (strncmp(v->name, "$Func", 5) != 0)
				{
					return FALSE;
				}

				/* Now we have to go about looking up each function in this enum.  WHICH IS IT. */
				e = funcenums_find_byval(formaltag);
				if (!e)
				{
					return FALSE;
				}

				assert(v->name[5] == '@' || v->name[5] == '!');

				/* Deduce which function type this is */
				if (v->name[5] == '@')
				{
					usage = uPUBLIC;
				} else if (v->name[5] = '!') {
					usage = uSTOCK;
				}

				index = atoi(&v->name[6]);

				assert(index >= 0);

				/* Find the function, either by public idx or code addr */
				if (usage == uPUBLIC)
				{
					for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
						if (sym->ident==iFUNCTN && (sym->usage & uPUBLIC)!=0 && (sym->vclass == sGLOBAL))
						{
							if (index-- == 0)
							{
								found = sym;
								break;
							}
						}
					}
				} else if (usage == uSTOCK) {
					for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
						if (sym->ident==iFUNCTN && (sym->vclass == sGLOBAL))
						{
							if (sym->codeaddr == index)
							{
								found = sym;
								break;
							}
						}
					}
				}

				if (!found)
				{
					assert(found);
					return FALSE;
				}

				/* Wow, we now have:
				 * 1) The functional enum deduced from formaltag
				 * 2) The function trying to be shoved in deduced from actualtag
				 * Now we have to check if it matches any one of the functags inside the enum.
				 */
				t = e->first;
				while (t)
				{
					int curarg,skip=0,i;
					arginfo *func_arg;
					funcarg_t *enum_arg;
					/* Check return type first. */
					if (t->ret_tag != sym->tag)
					{
						t = t->next;
						continue;
					}
					/* Check usage */
					if (t->type != usage)
					{
						t = t->next;
						continue;
					}
					/* Begin iterating arguments */
					for (curarg=0; curarg<t->argcount; curarg++)
					{
						enum_arg = &t->args[curarg];
						/* Check whether we've exhausted our arguments */
						if (sym->dim.arglist[curarg].ident == 0)
						{
							/* Can we bail out early? */
							if (!enum_arg->ommittable)
							{
								/* No! */
								skip = 1;
							}
							break;
						}
						func_arg = &sym->dim.arglist[curarg];
						/* First check the ident type */
						if (enum_arg->ident != func_arg->ident)
						{
							skip = 1;
							break;
						}
						/* Next check arrayness */
						if (enum_arg->dimcount != func_arg->numdim)
						{
							skip = 1;
							break;
						}
						if (enum_arg->dimcount > 0)
						{
							for (i=0; i<enum_arg->dimcount; i++)
							{
								if (enum_arg->dims[i] != func_arg->dim[i])
								{
									skip = 1;
									break;
								}
							}
							if (skip)
							{
								break;
							}
						}
						/* Lastly, check the tags */
						if (enum_arg->tagcount != func_arg->numtags)
						{
							skip = 1;
							break;
						}
						/* They should all be in the same order just for clarity... */
						for (i=0; i<enum_arg->tagcount; i++)
						{
							if (enum_arg->tags[i] != func_arg->tags[i])
							{
								skip = 1;
								break;
							}
						}
						if (skip)
						{
							break;
						}
					}
					if (!skip)
					{
						/* Make sure there are no trailing arguments */
						if (sym->dim.arglist[curarg].ident == 0)
						{
							return TRUE;
						}
					}
					t = t->next;
				}
			}
		}
		return FALSE;
	}
  } /* if */
  return TRUE;
}

/*
 *  The AMX pseudo-processor has no direct support for logical (boolean)
 *  operations. These have to be done via comparing and jumping. Since we are
 *  already jumping through the code, we might as well implement an "early
 *  drop-out" evaluation (also called "short-circuit"). This conforms to
 *  standard C:
 *
 *  expr1 || expr2           expr2 will only be evaluated if expr1 is false.
 *  expr1 && expr2           expr2 will only be evaluated if expr1 is true.
 *
 *  expr1 || expr2 && expr3  expr2 will only be evaluated if expr1 is false
 *                           and expr3 will only be evaluated if expr1 is
 *                           false and expr2 is true.
 *
 *  Code generation for the last example proceeds thus:
 *
 *      evaluate expr1
 *      operator || found
 *      jump to "l1" if result of expr1 not equal to 0
 *      evaluate expr2
 *      ->  operator && found; skip to higher level in hierarchy diagram
 *          jump to "l2" if result of expr2 equal to 0
 *          evaluate expr3
 *          jump to "l2" if result of expr3 equal to 0
 *          set expression result to 1 (true)
 *          jump to "l3"
 *      l2: set expression result to 0 (false)
 *      l3:
 *      <-  drop back to previous hierarchy level
 *      jump to "l1" if result of expr2 && expr3 not equal to 0
 *      set expression result to 0 (false)
 *      jump to "l4"
 *  l1: set expression result to 1 (true)
 *  l4:
 *
 */

/*  Skim over terms adjoining || and && operators
 *  dropval   The value of the expression after "dropping out". An "or" drops
 *            out when the left hand is TRUE, so dropval must be 1 on "or"
 *            expressions.
 *  endval    The value of the expression when no expression drops out. In an
 *            "or" expression, this happens when both the left hand and the
 *            right hand are FALSE, so endval must be 0 for "or" expressions.
 */
static int skim(int *opstr,void (*testfunc)(int),int dropval,int endval,
                int (*hier)(value*),value *lval)
{
  int lvalue,hits,droplab,endlab,opidx;
  int allconst,foundop;
  cell constval;
  int index;
  cell cidx;

  stgget(&index,&cidx);         /* mark position in code generator */
  hits=FALSE;                   /* no logical operators "hit" yet */
  allconst=TRUE;                /* assume all values "const" */
  constval=0;
  droplab=0;                    /* to avoid a compiler warning */
  for ( ;; ) {
    lvalue=plnge1(hier,lval);   /* evaluate left expression */

    allconst= allconst && (lval->ident==iCONSTEXPR);
    if (allconst) {
      if (hits) {
        /* one operator was already found */
        if (testfunc==jmp_ne0)
          lval->constval= lval->constval || constval;
        else
          lval->constval= lval->constval && constval;
      } /* if */
      constval=lval->constval;  /* save result accumulated so far */
    } /* if */

    foundop=nextop(&opidx,opstr);
    if ((foundop || hits) && (lval->ident==iARRAY || lval->ident==iREFARRAY))
      error(33, lval->sym ? (lval->sym->name ? lval->sym->name : "-unknown") : "-unknown-");  /* array was not indexed in an expression */
    if (foundop) {
      if (!hits) {
        /* this is the first operator in the list */
        hits=TRUE;
        droplab=getlabel();
      } /* if */
      dropout(lvalue,testfunc,droplab,lval);
    } else if (hits) {                       /* no (more) identical operators */
      dropout(lvalue,testfunc,droplab,lval); /* found at least one operator! */
      ldconst(endval,sPRI);
      jumplabel(endlab=getlabel());
      setlabel(droplab);
      ldconst(dropval,sPRI);
      setlabel(endlab);
      lval->sym=NULL;
      lval->tag=pc_addtag("bool");  /* force tag to be "bool" */
      if (allconst) {
        lval->ident=iCONSTEXPR;
        lval->constval=constval;
        stgdel(index,cidx);         /* scratch generated code and calculate */
      } else {
        lval->ident=iEXPRESSION;
        lval->constval=0;
      } /* if */
      return FALSE;
    } else {
      return lvalue;            /* none of the operators in "opstr" were found */
    } /* if */

  } /* while */
}

/*
 *  Reads into the primary register the variable pointed to by lval if
 *  plunging through the hierarchy levels detected an lvalue. Otherwise
 *  if a constant was detected, it is loaded. If there is no constant and
 *  no lvalue, the primary register must already contain the expression
 *  result.
 *
 *  After that, the compare routines "jmp_ne0" or "jmp_eq0" are called, which
 *  compare the primary register against 0, and jump to the "early drop-out"
 *  label "exit1" if the condition is true.
 */
static void dropout(int lvalue,void (*testfunc)(int val),int exit1,value *lval)
{
  if (lvalue)
    rvalue(lval);
  else if (lval->ident==iCONSTEXPR)
    ldconst(lval->constval,sPRI);
  (*testfunc)(exit1);
}

static void checkfunction(value *lval)
{
  symbol *sym=lval->sym;

  if (sym==NULL || (sym->ident!=iFUNCTN && sym->ident!=iREFFUNC))
    return;             /* no known symbol, or not a function result */

  if ((sym->usage & uDEFINE)!=0) {
    /* function is defined, can now check the return value (but make an
     * exception for directly recursive functions)
     */
    if (sym!=curfunc && (sym->usage & uRETVALUE)==0) {
      char symname[2*sNAMEMAX+16];  /* allow space for user defined operators */
      funcdisplayname(symname,sym->name);
      error(209,symname);       /* function should return a value */
    } /* if */
  } else {
    /* function not yet defined, set */
    sym->usage|=uRETVALUE;      /* make sure that a future implementation of
                                 * the function uses "return <value>" */
  } /* if */
}

/*
 *  Plunge to a lower level
 */
static int plnge(int *opstr,int opoff,int (*hier)(value *lval),value *lval,
                 char *forcetag,int chkbitwise)
{
  int lvalue,opidx;
  int count;
  value lval2 = {0};

  lvalue=plnge1(hier,lval);
  if (nextop(&opidx,opstr)==0)
    return lvalue;              /* no operator in "opstr" found */
  if (lvalue)
    rvalue(lval);
  count=0;
  do {
    if (chkbitwise && count++>0 && bitwise_opercount!=0)
      error(212);
    opidx+=opoff;               /* add offset to index returned by nextop() */
    plnge2(op1[opidx],hier,lval,&lval2);
    if (op1[opidx]==ob_and || op1[opidx]==ob_or)
      bitwise_opercount++;
    if (forcetag!=NULL)
      lval->tag=pc_addtag(forcetag);
  } while (nextop(&opidx,opstr)); /* do */
  return FALSE;         /* result of expression is not an lvalue */
}

/*  plnge_rel
 *
 *  Binary plunge to lower level; this is very simular to plnge, but
 *  it has special code generation sequences for chained operations.
 */
static int plnge_rel(int *opstr,int opoff,int (*hier)(value *lval),value *lval)
{
  int lvalue,opidx;
  value lval2={0};
  int count;

  /* this function should only be called for relational operators */
  assert(op1[opoff]==os_le);
  lvalue=plnge1(hier,lval);
  if (nextop(&opidx,opstr)==0)
    return lvalue;              /* no operator in "opstr" found */
  if (lvalue)
    rvalue(lval);
  count=0;
  lval->boolresult=TRUE;
  do {
    /* same check as in plnge(), but "chkbitwise" is always TRUE */
    if (count>0 && bitwise_opercount!=0)
      error(212);
    if (count>0) {
      relop_prefix();
      *lval=lval2;      /* copy right hand expression of the previous iteration */
    } /* if */
    opidx+=opoff;
    plnge2(op1[opidx],hier,lval,&lval2);
    if (count++>0)
      relop_suffix();
  } while (nextop(&opidx,opstr)); /* enddo */
  lval->constval=lval->boolresult;
  lval->tag=pc_addtag("bool");    /* force tag to be "bool" */
  return FALSE;         /* result of expression is not an lvalue */
}

/*  plnge1
 *
 *  Unary plunge to lower level
 *  Called by: skim(), plnge(), plnge2(), plnge_rel(), hier14() and hier13()
 */
static int plnge1(int (*hier)(value *lval),value *lval)
{
  int lvalue,index;
  cell cidx;

  stgget(&index,&cidx); /* mark position in code generator */
  lvalue=(*hier)(lval);
  if (lval->ident==iCONSTEXPR)
    stgdel(index,cidx); /* load constant later */
  return lvalue;
}

/*  plnge2
 *
 *  Binary plunge to lower level
 *  Called by: plnge(), plnge_rel(), hier14() and hier1()
 */
static void plnge2(void (*oper)(void),
                   int (*hier)(value *lval),
                   value *lval1,value *lval2)
{
  int index;
  cell cidx;

  stgget(&index,&cidx);             /* mark position in code generator */
  if (lval1->ident==iCONSTEXPR) {   /* constant on left side; it is not yet loaded */
    if (plnge1(hier,lval2))
      rvalue(lval2);                /* load lvalue now */
    else if (lval2->ident==iCONSTEXPR)
      ldconst(lval2->constval<<dbltest(oper,lval2,lval1),sPRI);
    ldconst(lval1->constval<<dbltest(oper,lval2,lval1),sALT);
                   /* ^ doubling of constants operating on integer addresses */
                   /*   is restricted to "add" and "subtract" operators */
  } else {                          /* non-constant on left side */
    pushreg(sPRI);
    if (plnge1(hier,lval2))
      rvalue(lval2);
    if (lval2->ident==iCONSTEXPR) { /* constant on right side */
      if (commutative(oper)) {      /* test for commutative operators */
        value lvaltmp = {0};
        stgdel(index,cidx);         /* scratch pushreg() and constant fetch (then
                                     * fetch the constant again */
        ldconst(lval2->constval<<dbltest(oper,lval1,lval2),sALT);
        /* now, the primary register has the left operand and the secondary
         * register the right operand; swap the "lval" variables so that lval1
         * is associated with the secondary register and lval2 with the
         * primary register, as is the "normal" case.
         */
        lvaltmp=*lval1;
        *lval1=*lval2;
        *lval2=lvaltmp;
      } else {
        ldconst(lval2->constval<<dbltest(oper,lval1,lval2),sPRI);
        popreg(sALT);   /* pop result of left operand into secondary register */
      } /* if */
    } else {            /* non-constants on both sides */
      popreg(sALT);
      if (dbltest(oper,lval1,lval2))
        cell2addr();                    /* double primary register */
      if (dbltest(oper,lval2,lval1))
        cell2addr_alt();                /* double secondary register */
    } /* if */
  } /* if */
  if (oper) {
    /* If used in an expression, a function should return a value.
     * If the function has been defined, we can check this. If the
     * function was not defined, we can set this requirement (so that
     * a future function definition can check this bit.
     */
    checkfunction(lval1);
    checkfunction(lval2);
    if (lval1->ident==iARRAY || lval1->ident==iREFARRAY) {
      char *ptr=(lval1->sym!=NULL) ? lval1->sym->name : "-unknown-";
      error(33,ptr);                    /* array must be indexed */
    } else if (lval2->ident==iARRAY || lval2->ident==iREFARRAY) {
      char *ptr=(lval2->sym!=NULL) ? lval2->sym->name : "-unknown-";
      error(33,ptr);                    /* array must be indexed */
    } /* if */
    /* ??? ^^^ should do same kind of error checking with functions */

    /* check whether an "operator" function is defined for the tag names
     * (a constant expression cannot be optimized in that case)
     */
    if (check_userop(oper,lval1->tag,lval2->tag,2,NULL,&lval1->tag)) {
      lval1->ident=iEXPRESSION;
      lval1->constval=0;
    } else if (lval1->ident==iCONSTEXPR && lval2->ident==iCONSTEXPR) {
      /* only constant expression if both constant */
      stgdel(index,cidx);       /* scratch generated code and calculate */
      if (!matchtag(lval1->tag,lval2->tag,FALSE))
        error(213);             /* tagname mismatch */
      lval1->constval=calc(lval1->constval,oper,lval2->constval,&lval1->boolresult);
    } else {
      if (!checktag_string(lval1, lval2) && !matchtag(lval1->tag,lval2->tag,FALSE))
        error(213);             /* tagname mismatch */
      (*oper)();                /* do the (signed) operation */
      lval1->ident=iEXPRESSION;
    } /* if */
  } /* if */
}

static cell flooreddiv(cell a,cell b,int return_remainder)
{
  cell q,r;

  if (b==0) {
    error(29);
    return 0;
  } /* if */
  /* first implement truncated division in a portable way */
  #define IABS(a)       ((a)>=0 ? (a) : (-a))
  q=IABS(a)/IABS(b);
  if ((cell)(a ^ b)<0)
    q=-q;               /* swap sign if either "a" or "b" is negative (but not both) */
  r=a-q*b;              /* calculate the matching remainder */
  /* now "fiddle" with the values to get floored division */
  if (r!=0 && (cell)(r ^ b)<0) {
    q--;
    r+=b;
  } /* if */
  return return_remainder ? r : q;
}

static cell calc(cell left,void (*oper)(),cell right,char *boolresult)
{
  if (oper==ob_or)
    return (left | right);
  else if (oper==ob_xor)
    return (left ^ right);
  else if (oper==ob_and)
    return (left & right);
  else if (oper==ob_eq)
    return (left == right);
  else if (oper==ob_ne)
    return (left != right);
  else if (oper==os_le)
    return *boolresult &= (char)(left <= right), right;
  else if (oper==os_ge)
    return *boolresult &= (char)(left >= right), right;
  else if (oper==os_lt)
    return *boolresult &= (char)(left < right), right;
  else if (oper==os_gt)
    return *boolresult &= (char)(left > right), right;
  else if (oper==os_sar)
    return (left >> (int)right);
  else if (oper==ou_sar)
    return ((ucell)left >> (ucell)right);
  else if (oper==ob_sal)
    return ((ucell)left << (int)right);
  else if (oper==ob_add)
    return (left + right);
  else if (oper==ob_sub)
    return (left - right);
  else if (oper==os_mult)
    return (left * right);
  else if (oper==os_div)
    return flooreddiv(left,right,0);
  else if (oper==os_mod)
    return flooreddiv(left,right,1);
  else
    error(29);  /* invalid expression, assumed 0 (this should never occur) */
  return 0;
}

SC_FUNC int expression(cell *val,int *tag,symbol **symptr,int chkfuncresult,value *_lval)
{
  value lval={0};
  pushheaplist();

  if (hier14(&lval))
    rvalue(&lval);
  /* scrap any arrays left on the heap */
  popheaplist();

  if (lval.ident==iCONSTEXPR && val!=NULL)    /* constant expression */
    *val=lval.constval;
  if (tag!=NULL)
    *tag=lval.tag;
  if (symptr!=NULL)
    *symptr=lval.sym;
  if (chkfuncresult)
    checkfunction(&lval);
  if (_lval)
    *_lval=lval;
  return lval.ident;
}

SC_FUNC int sc_getstateid(constvalue **automaton,constvalue **state)
{
  char name[sNAMEMAX+1];
  cell val;
  char *str;
  int fsa,islabel;

  assert(automaton!=NULL);
  assert(state!=NULL);
  if (!(islabel=matchtoken(tLABEL)) && !needtoken(tSYMBOL))
    return 0;

  tokeninfo(&val,&str);
  assert(strlen(str)<sizeof name);
  strcpy(name,str);
  if (islabel || matchtoken(':')) {
    /* token is an automaton name, add the name and get a new token */
    *automaton=automaton_find(name);
    /* read in the state name before checking the automaton, to keep the parser
     * going (an "unknown automaton" error may occur when the "state" instruction
     * precedes any state definition)
     */
    if (!needtoken(tSYMBOL))
      return 0;
    tokeninfo(&val,&str);        /* do not copy the name yet, must check automaton first */
    if (*automaton==NULL) {
      error(86,name);            /* unknown automaton */
      return 0;
    } /* if */
    assert((*automaton)->index>0);
    assert(strlen(str)<sizeof name);
    strcpy(name,str);
  } else {
    *automaton=automaton_find("");
    assert(*automaton!=NULL);
    assert((*automaton)->index==0);
  } /* if */
  assert(*automaton!=NULL);
  fsa=(*automaton)->index;

  assert(*automaton!=NULL);
  *state=state_find(name,fsa);
  if (*state==NULL) {
    char *fsaname=(*automaton)->name;
    if (*fsaname=='\0')
      fsaname="<main>";
    error(87,name,fsaname);   /* unknown state for automaton */
    return 0;
  } /* if */

  return 1;
}

SC_FUNC cell array_totalsize(symbol *sym)
{
  cell length;

  assert(sym!=NULL);
  assert(sym->ident==iARRAY || sym->ident==iREFARRAY);
  length=sym->dim.array.length;
  if (sym->dim.array.level > 0) {
    cell sublength=array_totalsize(finddepend(sym));
    if (sublength>0)
      length=length+length*sublength;
    else
      length=0;
  } /* if */
  return length;
}

static cell array_levelsize(symbol *sym,int level)
{
  assert(sym!=NULL);
  assert(sym->ident==iARRAY || sym->ident==iREFARRAY);
  assert(level <= sym->dim.array.level);
  while (level-- > 0) {
    sym=finddepend(sym);
    assert(sym!=NULL);
  } /* if */
  return (sym->dim.array.slength ? sym->dim.array.slength : sym->dim.array.length);
}

/*  hier14
 *
 *  Lowest hierarchy level (except for the , operator).
 *
 *  Global references: sc_intest        (reffered to only)
 *                     sc_allowproccall (modified)
 */
static int hier14(value *lval1)
{
  int lvalue;
  value lval2={0},lval3={0};
  void (*oper)(void);
  int tok,level,i;
  cell val;
  char *st;
  int bwcount,leftarray;
  cell arrayidx1[sDIMEN_MAX],arrayidx2[sDIMEN_MAX];  /* last used array indices */
  cell *org_arrayidx;

  bwcount=bitwise_opercount;
  bitwise_opercount=0;
  /* initialize the index arrays with unlikely constant indices; note that
   * these indices will only be changed when the array is indexed with a
   * constant, and that negative array indices are invalid (so actually, any
   * negative value would do).
   */
  for (i=0; i<sDIMEN_MAX; i++)
    arrayidx1[i]=arrayidx2[i]=(cell)(-1L << (sizeof(cell)*8-1));
  org_arrayidx=lval1->arrayidx; /* save current pointer, to reset later */
  if (lval1->arrayidx==NULL)
    lval1->arrayidx=arrayidx1;
  lvalue=plnge1(hier13,lval1);
  if (lval1->ident!=iARRAYCELL && lval1->ident!=iARRAYCHAR)
    lval1->arrayidx=NULL;
  if (lval1->ident==iCONSTEXPR) /* load constant here */
    ldconst(lval1->constval,sPRI);
  tok=lex(&val,&st);
  switch (tok) {
    case taOR:
      oper=ob_or;
      break;
    case taXOR:
      oper=ob_xor;
      break;
    case taAND:
      oper=ob_and;
      break;
    case taADD:
      oper=ob_add;
      break;
    case taSUB:
      oper=ob_sub;
      break;
    case taMULT:
      oper=os_mult;
      break;
    case taDIV:
      oper=os_div;
      break;
    case taMOD:
      oper=os_mod;
      break;
    case taSHRU:
      oper=ou_sar;
      break;
    case taSHR:
      oper=os_sar;
      break;
    case taSHL:
      oper=ob_sal;
      break;
    case '=':           /* simple assignment */
      oper=NULL;
      if (sc_intest)
        error(211);     /* possibly unintended assignment */
      break;
    default:
      lexpush();
      bitwise_opercount=bwcount;
      lval1->arrayidx=org_arrayidx; /* restore array index pointer */
      return lvalue;
  } /* switch */

  /* if we get here, it was an assignment; first check a few special cases
   * and then the general */
  if (lval1->ident==iARRAYCHAR) {
    /* special case, assignment to packed character in a cell is permitted */
    lvalue=TRUE;
  } else if (lval1->ident==iARRAY || lval1->ident==iREFARRAY) {
    /* array assignment is permitted too (with restrictions) */
    if (oper)
      return error(23); /* array assignment must be simple assigment */
    assert(lval1->sym!=NULL);
    if (array_totalsize(lval1->sym)==0)
      return error(46,lval1->sym->name);        /* unknown array size */
    lvalue=TRUE;
  } /* if */

  /* operand on left side of assignment must be lvalue */
  if (!lvalue)
    return error(22);                   /* must be lvalue */
  /* may not change "constant" parameters */
  assert(lval1->sym!=NULL);
  if ((lval1->sym->usage & uCONST)!=0)
    return error(22);           /* assignment to const argument */
  sc_allowproccall=FALSE;       /* may no longer use "procedure call" syntax */

  lval3=*lval1;         /* save symbol to enable storage of expresion result */
  lval1->arrayidx=org_arrayidx; /* restore array index pointer */
  if (lval1->ident==iARRAYCELL || lval1->ident==iARRAYCHAR
      || lval1->ident==iARRAY || lval1->ident==iREFARRAY)
  {
    /* if indirect fetch: save PRI (cell address) */
    if (oper) {
      pushreg(sPRI);
      rvalue(lval1);
    } /* if */
    lval2.arrayidx=arrayidx2;
    plnge2(oper,hier14,lval1,&lval2);
    if (lval2.ident!=iARRAYCELL && lval2.ident!=iARRAYCHAR)
      lval2.arrayidx=NULL;
    if (oper)
      popreg(sALT);
    if (!oper && lval3.arrayidx!=NULL && lval2.arrayidx!=NULL
        && lval3.ident==lval2.ident && lval3.sym==lval2.sym)
    {
      int same=TRUE;
      assert(lval2.arrayidx==arrayidx2);
      for (i=0; i<sDIMEN_MAX; i++)
        same=same && (lval3.arrayidx[i]==lval2.arrayidx[i]);
        if (same)
          error(226,lval3.sym->name);   /* self-assignment */
    } /* if */
  } else {
    if (oper){
      rvalue(lval1);
      plnge2(oper,hier14,lval1,&lval2);
    } else {
      /* if direct fetch and simple assignment: no "push"
       * and "pop" needed -> call hier14() directly, */
      if (hier14(&lval2))
        rvalue(&lval2);         /* instead of plnge2(). */
      else if (lval2.ident==iVARIABLE)
        lval2.ident=iEXPRESSION;/* mark as "rvalue" if it is not an "lvalue" */
      checkfunction(&lval2);
      /* check whether lval2 and lval3 (old lval1) refer to the same variable */
      if (lval2.ident==iVARIABLE && lval3.ident==lval2.ident && lval3.sym==lval2.sym) {
        assert(lval3.sym!=NULL);
        error(226,lval3.sym->name);     /* self-assignment */
      } /* if */
    } /* if */
  } /* if */
  /* Array elements are sometimes considered as sub-arrays --when the
   * array index is an enumeration field and the enumeration size is greater
   * than 1. If the expression on the right side of the assignment is a cell,
   * or if an operation is in effect, this does not apply.
   */
  leftarray= lval3.ident==iARRAY || lval3.ident==iREFARRAY
             || ((lval3.ident==iARRAYCELL || lval3.ident==iARRAYCHAR)
                 && lval3.constval>1 && lval3.sym->dim.array.level==0
                 && !oper && (lval2.ident==iARRAY || lval2.ident==iREFARRAY));
  if (leftarray) {
    /* Left operand is an array, right operand should be an array variable
     * of the same size and the same dimension, an array literal (of the
     * same size) or a literal string. For single-dimensional arrays without
     * tag for the index, it is permitted to assign a smaller array into a
     * larger one (without warning). This is to make it easier to work with
     * strings.
     */
    int exactmatch=TRUE;
    int idxtag=0;
    int ltlength=(int)lval3.sym->dim.array.length;
    if ((lval3.ident==iARRAYCELL || lval3.ident==iARRAYCHAR)
        && lval3.constval>0 && lval3.sym->dim.array.level==0)
    {
      ltlength=(int)lval3.constval;
    } /* if */
    if (lval2.ident!=iARRAY && lval2.ident!=iREFARRAY
        && (lval2.sym==NULL || lval2.constval<=0))
      error(33,lval3.sym->name);        /* array must be indexed */
    if (lval2.sym!=NULL) {
      if (lval2.constval==0) {
        val=lval2.sym->dim.array.length;/* array variable */
      } else {
        val=lval2.constval;
        if (lval2.sym->dim.array.level!=0)
          error(28,lval2.sym->name);
      } /* if */
      level=lval2.sym->dim.array.level;
      idxtag=lval2.sym->x.tags.index;
      if (level==0 && idxtag==0 && lval3.sym->x.tags.index==0)
        exactmatch=FALSE;
    } else {
      val=lval2.constval;               /* literal array */
      level=0;
      /* If val is negative, it means that lval2 is a literal string.
       * The string array size may be smaller than the destination
       * array, provided that the destination array does not have an
       * index tag.
       */
      if (val<0) {
        val=-val;
        if (lval3.sym->x.tags.index==0)
          exactmatch=FALSE;
      } /* if */
    } /* if */
    if (lval3.sym->dim.array.level!=level)
      return error(48); /* array dimensions must match */
    else if (ltlength<val || exactmatch && ltlength>val || val==0)
      return error(47); /* array sizes must match */
    else if (lval3.ident!=iARRAYCELL && !matchtag(lval3.sym->x.tags.index,idxtag,TRUE))
      error(229,(lval2.sym!=NULL) ? lval2.sym->name : lval3.sym->name); /* index tag mismatch */
    if (level>0) {
      /* check the sizes of all sublevels too */
      symbol *sym1 = lval3.sym;
      symbol *sym2 = lval2.sym;
      int i;
      assert(sym1!=NULL && sym2!=NULL);
      /* ^^^ sym2 must be valid, because only variables can be
       *     multi-dimensional (there are no multi-dimensional literals),
       *     sym1 must be valid because it must be an lvalue
       */
      assert(exactmatch);
      for (i=0; i<level; i++) {
        sym1=finddepend(sym1);
        sym2=finddepend(sym2);
        assert(sym1!=NULL && sym2!=NULL);
        /* ^^^ both arrays have the same dimensions (this was checked
         *     earlier) so the dependend should always be found
         */
        if (sym1->dim.array.length!=sym2->dim.array.length)
          error(47);    /* array sizes must match */
        else if (!matchtag(sym1->x.tags.index,sym2->x.tags.index,TRUE))
          error(229,sym2->name);  /* index tag mismatch */
      } /* for */
      /* get the total size in cells of the multi-dimensional array */
      val=array_totalsize(lval3.sym);
      assert(val>0);    /* already checked */
    } /* if */
  } else {
    /* left operand is not an array, right operand should then not be either */
    if (lval2.ident==iARRAY || lval2.ident==iREFARRAY)
      error(6);         /* must be assigned to an array */
  } /* if */
  if (leftarray) {
    memcopy(val*sizeof(cell));
  } else {
    check_userop(NULL,lval2.tag,lval3.tag,2,&lval3,&lval2.tag);
    store(&lval3);      /* now, store the expression result */
  } /* if */
  if (!oper && !checktag_string(&lval3, &lval2) && !matchtag(lval3.tag,lval2.tag,TRUE))
    error(213);         /* tagname mismatch (if "oper", warning already given in plunge2()) */
  if (lval3.sym)
    markusage(lval3.sym,uWRITTEN);
  sideeffect=TRUE;
  bitwise_opercount=bwcount;
  lval1->ident=iEXPRESSION;
  return FALSE;         /* expression result is never an lvalue */
}

/**
 * Sums up array usage in the current heap tracer and convert it into a dynamic array.
 * This is used for the ternary operator, which needs to convert its array usage into
 * something dynamically managed.
 * !Note:
 * This might break if expressions can ever return dynamic arrays.
 * Thus, we assert() if something is non-static here.
 * Right now, this poses no problem because this type of expression is impossible:
 *   (a() ? return_array() : return_array()) ? return_array() : return_array()
 */

void dynarray_from_heaplist(memuse_list_t *heap)
{
  memuse_t *use=heap->head;
  memuse_t *tmp;
  long total=0;
  while (use) {
    assert(use->type==MEMUSE_STATIC);
    total+=use->size;
    tmp=use->prev;
    free(use);
    use=tmp;
  }
  free(heap);
  if (total)
    setheap_save(-total*sizeof(cell));
}

static int hier13(value *lval)
{
  int lvalue=plnge1(hier12,lval);
  if (matchtoken('?')) {
    int flab1=getlabel();
    int flab2=getlabel();
    value lval2={0};
    int array1,array2;
    memuse_list_t *heap;
    
    pushheaplist();
    if (lvalue) {
      rvalue(lval);
    } else if (lval->ident==iCONSTEXPR) {
      ldconst(lval->constval,sPRI);
      error(lval->constval ? 206 : 205);        /* redundant test */
    } /* if */
    jmp_eq0(flab1);             /* go to second expression if primary register==0 */
    PUSHSTK_I(sc_allowtags);
    sc_allowtags=FALSE;         /* do not allow tagnames here (colon is a special token) */
    if (hier13(lval))
      rvalue(lval);
    if (lval->ident==iCONSTEXPR)        /* load constant here */
      ldconst(lval->constval,sPRI);
    sc_allowtags=(short)POPSTK_I();     /* restore */
    heap=popsaveheaplist();
    dynarray_from_heaplist(heap);
    pushheaplist();
    jumplabel(flab2);
    setlabel(flab1);
    needtoken(':');
    if (hier13(&lval2))
      rvalue(&lval2);
    if (lval2.ident==iCONSTEXPR)        /* load constant here */
      ldconst(lval2.constval,sPRI);
    array1= (lval->ident==iARRAY || lval->ident==iREFARRAY);
    array2= (lval2.ident==iARRAY || lval2.ident==iREFARRAY);
    if (array1 && !array2) {
      char *ptr=(lval->sym->name!=NULL) ? lval->sym->name : "-unknown-";
      error(33,ptr);            /* array must be indexed */
    } else if (!array1 && array2) {
      char *ptr=(lval2.sym->name!=NULL) ? lval2.sym->name : "-unknown-";
      error(33,ptr);            /* array must be indexed */
    } /* if */
    /* ??? if both are arrays, should check dimensions */
    if (!matchtag(lval->tag,lval2.tag,FALSE))
      error(213);               /* tagname mismatch ('true' and 'false' expressions) */
    heap=popsaveheaplist();
    dynarray_from_heaplist(heap);
    setlabel(flab2);
    if (array1 && array2) {
      markheap(MEMUSE_DYNAMIC, 0);
    }
    if (lval->ident==iARRAY)
      lval->ident=iREFARRAY;    /* iARRAY becomes iREFARRAY */
    else if (lval->ident!=iREFARRAY)
      lval->ident=iEXPRESSION;  /* iREFARRAY stays iREFARRAY, rest becomes iEXPRESSION */
    return FALSE;               /* conditional expression is no lvalue */
  } else {
    return lvalue;
  } /* if */
}

/* the order of the operators in these lists is important and must be
 * the same as the order of the operators in the array "op1"
 */
static int list3[]  = {'*','/','%',0};
static int list4[]  = {'+','-',0};
static int list5[]  = {tSHL,tSHR,tSHRU,0};
static int list6[]  = {'&',0};
static int list7[]  = {'^',0};
static int list8[]  = {'|',0};
static int list9[]  = {tlLE,tlGE,'<','>',0};
static int list10[] = {tlEQ,tlNE,0};
static int list11[] = {tlAND,0};
static int list12[] = {tlOR,0};

static int hier12(value *lval)
{
  return skim(list12,jmp_ne0,1,0,hier11,lval);
}

static int hier11(value *lval)
{
  return skim(list11,jmp_eq0,0,1,hier10,lval);
}

static int hier10(value *lval)
{ /* ==, != */
  return plnge(list10,15,hier9,lval,"bool",TRUE);
}                  /* ^ this variable is the starting index in the op1[]
                    *   array of the operators of this hierarchy level */

static int hier9(value *lval)
{ /* <=, >=, <, > */
  return plnge_rel(list9,11,hier8,lval);
}

static int hier8(value *lval)
{ /* | */
  return plnge(list8,10,hier7,lval,NULL,FALSE);
}

static int hier7(value *lval)
{ /* ^ */
  return plnge(list7,9,hier6,lval,NULL,FALSE);
}

static int hier6(value *lval)
{ /* & */
  return plnge(list6,8,hier5,lval,NULL,FALSE);
}

static int hier5(value *lval)
{ /* <<, >>, >>> */
  return plnge(list5,5,hier4,lval,NULL,FALSE);
}

static int hier4(value *lval)
{ /* +, - */
  return plnge(list4,3,hier3,lval,NULL,FALSE);
}

static int hier3(value *lval)
{ /* *, /, % */
  return plnge(list3,0,hier2,lval,NULL,FALSE);
}

static int hier2(value *lval)
{
  int lvalue,tok;
  int tag,paranthese;
  cell val;
  char *st;
  symbol *sym;
  int saveresult;

  tok=lex(&val,&st);
  switch (tok) {
  case tINC:                    /* ++lval */
    if (!hier2(lval))
      return error(22);         /* must be lvalue */
    assert(lval->sym!=NULL);
    if ((lval->sym->usage & uCONST)!=0)
      return error(22);         /* assignment to const argument */
    if (!check_userop(user_inc,lval->tag,0,1,lval,&lval->tag))
      inc(lval);                /* increase variable first */
    rvalue(lval);               /* and read the result into PRI */
    sideeffect=TRUE;
    return FALSE;               /* result is no longer lvalue */
  case tDEC:                    /* --lval */
    if (!hier2(lval))
      return error(22);         /* must be lvalue */
    assert(lval->sym!=NULL);
    if ((lval->sym->usage & uCONST)!=0)
      return error(22);         /* assignment to const argument */
    if (!check_userop(user_dec,lval->tag,0,1,lval,&lval->tag))
      dec(lval);                /* decrease variable first */
    rvalue(lval);               /* and read the result into PRI */
    sideeffect=TRUE;
    return FALSE;               /* result is no longer lvalue */
  case '~':                     /* ~ (one's complement) */
    if (hier2(lval))
      rvalue(lval);
    invert();                   /* bitwise NOT */
    lval->constval=~lval->constval;
    return FALSE;
  case '!':                     /* ! (logical negate) */
    if (hier2(lval))
      rvalue(lval);
    if (check_userop(lneg,lval->tag,0,1,NULL,&lval->tag)) {
      lval->ident=iEXPRESSION;
      lval->constval=0;
    } else {
      lneg();                   /* 0 -> 1,  !0 -> 0 */
      lval->constval=!lval->constval;
      lval->tag=pc_addtag("bool");
    } /* if */
    return FALSE;
  case '-':                     /* unary - (two's complement) */
    if (hier2(lval))
      rvalue(lval);
    /* make a special check for a constant expression with the tag of a
     * rational number, so that we can simple swap the sign of that constant.
     */
    if (lval->ident==iCONSTEXPR && lval->tag==sc_rationaltag && sc_rationaltag!=0) {
      if (rational_digits==0) {
        #if PAWN_CELL_SIZE==32
          float *f = (float *)&lval->constval;
        #elif PAWN_CELL_SIZE==64
          double *f = (double *)&lval->constval;
        #else
          #error Unsupported cell size
        #endif
        *f= - *f; /* this modifies lval->constval */
      } else {
        /* the negation of a fixed point number is just an integer negation */
        lval->constval=-lval->constval;
      } /* if */
    } else if (check_userop(neg,lval->tag,0,1,NULL,&lval->tag)) {
      lval->ident=iEXPRESSION;
      lval->constval=0;
    } else {
      neg();                    /* arithmic negation */
      lval->constval=-lval->constval;
    } /* if */
    return FALSE;
  case tLABEL:                  /* tagname override */
    tag=pc_addtag(st);
    lval->cmptag=tag;
    lvalue=hier2(lval);
    lval->tag=tag;
    return lvalue;
  case tDEFINED:
    paranthese=0;
    while (matchtoken('('))
      paranthese++;
    tok=lex(&val,&st);
    if (tok!=tSYMBOL)
      return error(20,st);      /* illegal symbol name */
    sym=findloc(st);
    if (sym==NULL)
      sym=findglb(st,sSTATEVAR);
    if (sym!=NULL && sym->ident!=iFUNCTN && sym->ident!=iREFFUNC && (sym->usage & uDEFINE)==0)
      sym=NULL;                 /* symbol is not a function, it is in the table, but not "defined" */
    val= (sym!=NULL);
    if (!val && find_subst(st,strlen(st))!=NULL)
      val=1;
    clear_value(lval);
    lval->ident=iCONSTEXPR;
    lval->constval= val;
    lval->tag=pc_addtag("bool");
    ldconst(lval->constval,sPRI);
    while (paranthese--)
      needtoken(')');
    return FALSE;
  case tSIZEOF:
    paranthese=0;
    while (matchtoken('('))
      paranthese++;
    tok=lex(&val,&st);
    if (tok!=tSYMBOL)
      return error(20,st);      /* illegal symbol name */
    sym=findloc(st);
    if (sym==NULL)
      sym=findglb(st,sSTATEVAR);
    if (sym==NULL)
      return error(17,st);      /* undefined symbol */
    if (sym->ident==iCONSTEXPR)
      error(39);                /* constant symbol has no size */
    else if (sym->ident==iFUNCTN || sym->ident==iREFFUNC)
      error(72);                /* "function" symbol has no size */
    else if ((sym->usage & uDEFINE)==0)
      return error(17,st);      /* undefined symbol (symbol is in the table, but it is "used" only) */
    clear_value(lval);
    lval->ident=iCONSTEXPR;
    lval->constval=1;           /* preset */
    if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
      int level;
      symbol *idxsym=NULL;
      symbol *subsym=sym;
      for (level=0; matchtoken('['); level++) {
        idxsym=NULL;
        if (subsym!=NULL && level==subsym->dim.array.level && matchtoken(tSYMBOL)) {
          char *idxname;
          int cmptag=subsym->x.tags.index;
          tokeninfo(&val,&idxname);
          if ((idxsym=findconst(idxname,&cmptag))==NULL)
            error(80,idxname);  /* unknown symbol, or non-constant */
          else if (cmptag>1)
            error(91,idxname);  /* ambiguous constant */
        } /* if */
        needtoken(']');
        if (subsym!=NULL)
          subsym=finddepend(subsym);
      } /* for */
	  if (level>sym->dim.array.level+1) {
        error(28,sym->name);  /* invalid subscript */
      } else if (level==sym->dim.array.level+1) {
        lval->constval= (idxsym!=NULL && idxsym->dim.array.length>0) ? idxsym->dim.array.length : 1;
      } else {
        lval->constval=array_levelsize(sym,level);
      }
      if (lval->constval==0 && strchr((char *)lptr,PREPROC_TERM)==NULL)
        error(224,st);          /* indeterminate array size in "sizeof" expression */
    } /* if */
    ldconst(lval->constval,sPRI);
    while (paranthese--)
      needtoken(')');
    return FALSE;
  case tCELLSOF:
    paranthese=0;
    while (matchtoken('('))
      paranthese++;
    tok=lex(&val,&st);
    if (tok!=tSYMBOL)
      return error(20,st);      /* illegal symbol name */
    sym=findloc(st);
    if (sym==NULL)
      sym=findglb(st,sSTATEVAR);
    if (sym==NULL)
      return error(17,st);      /* undefined symbol */
    if (sym->ident==iCONSTEXPR)
      error(39);                /* constant symbol has no size */
    else if (sym->ident==iFUNCTN || sym->ident==iREFFUNC)
      error(72);                /* "function" symbol has no size */
    else if ((sym->usage & uDEFINE)==0)
      return error(17,st);      /* undefined symbol (symbol is in the table, but it is "used" only) */
    clear_value(lval);
    lval->ident=iCONSTEXPR;
    lval->constval=1;           /* preset */
    if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
      int level;
      symbol *idxsym=NULL;
      symbol *subsym=sym;
      for (level=0; matchtoken('['); level++) {
        idxsym=NULL;
        if (subsym!=NULL && level==subsym->dim.array.level && matchtoken(tSYMBOL)) {
          char *idxname;
          int cmptag=subsym->x.tags.index;
          tokeninfo(&val,&idxname);
          if ((idxsym=findconst(idxname,&cmptag))==NULL)
            error(80,idxname);  /* unknown symbol, or non-constant */
          else if (cmptag>1)
            error(91,idxname);  /* ambiguous constant */
        } /* if */
        needtoken(']');
        if (subsym!=NULL)
          subsym=finddepend(subsym);
      } /* for */
	  if (level>sym->dim.array.level+1) {
        error(28,sym->name);  /* invalid subscript */
      } else if (level==sym->dim.array.level+1) {
        lval->constval= (idxsym!=NULL && idxsym->dim.array.length>0) ? idxsym->dim.array.length : 1;
      } else {
        lval->constval=array_levelsize(sym,level);
      }
      if (lval->constval==0 && strchr((char *)lptr,PREPROC_TERM)==NULL)
        error(224,st);          /* indeterminate array size in "sizeof" expression */
    } /* if */
    ldconst(lval->constval,sPRI);
    while (paranthese--)
      needtoken(')');
    return FALSE;
  case tTAGOF:
    paranthese=0;
    while (matchtoken('('))
      paranthese++;
    tok=lex(&val,&st);
    if (tok!=tSYMBOL && tok!=tLABEL)
      return error(20,st);      /* illegal symbol name */
    if (tok==tLABEL) {
      constvalue *tagsym=find_constval(&tagname_tab,st,0);
      tag=(int)((tagsym!=NULL) ? tagsym->value : 0);
    } else {
      sym=findloc(st);
      if (sym==NULL)
        sym=findglb(st,sSTATEVAR);
      if (sym==NULL)
        return error(17,st);      /* undefined symbol */
      if ((sym->usage & uDEFINE)==0)
        return error(17,st);      /* undefined symbol (symbol is in the table, but it is "used" only) */
      tag=sym->tag;
    } /* if */
    if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
      int level;
      symbol *idxsym=NULL;
      symbol *subsym=sym;
      for (level=0; matchtoken('['); level++) {
        idxsym=NULL;
        if (subsym!=NULL && level==subsym->dim.array.level && matchtoken(tSYMBOL)) {
          char *idxname;
          int cmptag=subsym->x.tags.index;
          tokeninfo(&val,&idxname);
          if ((idxsym=findconst(idxname,&cmptag))==NULL)
            error(80,idxname);  /* unknown symbol, or non-constant */
          else if (cmptag>1)
            error(91,idxname);  /* ambiguous constant */
        } /* if */
        needtoken(']');
        if (subsym!=NULL)
          subsym=finddepend(subsym);
      } /* for */
      if (level>sym->dim.array.level+1)
        error(28,sym->name);  /* invalid subscript */
      else if (level==sym->dim.array.level+1 && idxsym!=NULL)
        tag= idxsym->x.tags.index;
    } /* if */
    exporttag(tag);
    clear_value(lval);
    lval->ident=iCONSTEXPR;
    lval->constval=tag | PUBLICTAG;
    ldconst(lval->constval,sPRI);
    while (paranthese--)
      needtoken(')');
    return FALSE;
  case tSTATE: {
    constvalue *automaton;
    constvalue *state;
    if (sc_getstateid(&automaton,&state)) {
      assert(automaton!=NULL);
      assert(automaton->index==0 && automaton->name[0]=='\0' || automaton->index>0);
      loadreg(automaton->value,sALT);
      assert(state!=NULL);
      ldconst(state->value,sPRI);
      ob_eq();
      clear_value(lval);
      lval->ident=iEXPRESSION;
      lval->tag=pc_addtag("bool");
    } /* if */
    return FALSE;
  } /* case */
  default:
    lexpush();
    lvalue=hier1(lval);
    /* check for postfix operators */
    if (matchtoken(';')) {
      /* Found a ';', do not look further for postfix operators */
      lexpush();                /* push ';' back after successful match */
      return lvalue;
    } else if (matchtoken(tTERM)) {
      /* Found a newline that ends a statement (this is the case when
       * semicolons are optional). Note that an explicit semicolon was
       * handled above. This case is similar, except that the token must
       * not be pushed back.
       */
      return lvalue;
    } else {
      tok=lex(&val,&st);
      switch (tok) {
      case tINC:                /* lval++ */
        if (!lvalue)
          return error(22);     /* must be lvalue */
        assert(lval->sym!=NULL);
        if ((lval->sym->usage & uCONST)!=0)
          return error(22);     /* assignment to const argument */
        /* on incrementing array cells, the address in PRI must be saved for
         * incremening the value, whereas the current value must be in PRI
         * on exit.
         */
        saveresult= (lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR);
        if (saveresult)
          pushreg(sPRI);        /* save address in PRI */
        rvalue(lval);           /* read current value into PRI */
        if (saveresult)
          swap1();              /* save PRI on the stack, restore address in PRI */
        if (!check_userop(user_inc,lval->tag,0,1,lval,&lval->tag))
          inc(lval);            /* increase variable afterwards */
        if (saveresult)
          popreg(sPRI);         /* restore PRI (result of rvalue()) */
        sideeffect=TRUE;
        return FALSE;           /* result is no longer lvalue */
      case tDEC:                /* lval-- */
        if (!lvalue)
          return error(22);     /* must be lvalue */
        assert(lval->sym!=NULL);
        if ((lval->sym->usage & uCONST)!=0)
          return error(22);     /* assignment to const argument */
        saveresult= (lval->ident==iARRAYCELL || lval->ident==iARRAYCHAR);
        if (saveresult)
          pushreg(sPRI);        /* save address in PRI */
        rvalue(lval);           /* read current value into PRI */
        if (saveresult)
          swap1();              /* save PRI on the stack, restore address in PRI */
        if (!check_userop(user_dec,lval->tag,0,1,lval,&lval->tag))
          dec(lval);            /* decrease variable afterwards */
        if (saveresult)
          popreg(sPRI);         /* restore PRI (result of rvalue()) */
        sideeffect=TRUE;
        return FALSE;
/* This is temporarily disabled because we detect it automatically.
 * Thus, it could be weird if both were used at once
 */
#if 0
      case tCHAR:               /* char (compute required # of cells */
        if (lval->ident==iCONSTEXPR) {
          lval->constval *= sCHARBITS/8;  /* from char to bytes */
          lval->constval = (lval->constval + sizeof(cell)-1) / sizeof(cell);
        } else {
          if (lvalue)
            rvalue(lval);       /* fetch value if not already in PRI */
          char2addr();          /* from characters to bytes */
          addconst(sizeof(cell)-1);     /* make sure the value is rounded up */
          addr2cell();          /* truncate to number of cells */
        } /* if */
        return FALSE;
#endif
      default:
        lexpush();
        return lvalue;
      } /* switch */
    } /* if */
  } /* switch */
}

/*  hier1
 *
 *  The highest hierarchy level: it looks for pointer and array indices
 *  and function calls.
 *  Generates code to fetch a pointer value if it is indexed and code to
 *  add to the pointer value or the array address (the address is already
 *  read at primary()). It also generates code to fetch a function address
 *  if that hasn't already been done at primary() (check lval[4]) and calls
 *  callfunction() to call the function.
 */
static int hier1(value *lval1)
{
  int lvalue,index,tok,symtok;
  cell val,cidx;
  value lval2={0};
  char *st;
  char close;
  symbol *sym;
  int magic_string=0;
  symbol dummysymbol,*cursym;   /* for changing the index tags in case of enumerated pseudo-arrays */

  lvalue=primary(lval1);
  symtok=tokeninfo(&val,&st);   /* get token read by primary() */
  cursym=lval1->sym;
restart:
  sym=cursym;
  if (matchtoken('[') || matchtoken('{') || matchtoken('(')) {
    tok=tokeninfo(&val,&st);    /* get token read by matchtoken() */
    magic_string = (sym && (sym->tag == pc_tag_string && sym->dim.array.level == 0));
    if (sym==NULL && symtok!=tSYMBOL) {
      /* we do not have a valid symbol and we appear not to have read a valid
       * symbol name (so it is unlikely that we would have read a name of an
       * undefined symbol) */
      error(29);                /* expression error, assumed 0 */
      lexpush();                /* analyse '(', '{' or '[' again later */
      return FALSE;
    } /* if */
    if (tok=='[' || tok=='{') { /* subscript */
      close = (char)((tok=='[') ? ']' : '}');
      if (sym==NULL) {  /* sym==NULL if lval is a constant or a literal */
        error(28,"<no variable>");  /* cannot subscript */
        needtoken(close);
        return FALSE;
      } else if (sym->ident!=iARRAY && sym->ident!=iREFARRAY){
        error(28,sym->name);    /* cannot subscript, variable is not an array */
        needtoken(close);
        return FALSE;
      } else if (sym->dim.array.level>0 && close!=']') {
        error(51);      /* invalid subscript, must use [ ] */
        needtoken(close);
        return FALSE;
      } /* if */
      /* set the tag to match (enumeration fields as indices) */
      lval2.cmptag=sym->x.tags.index;
      stgget(&index,&cidx);     /* mark position in code generator */
      pushreg(sPRI);            /* save base address of the array */
      if (hier14(&lval2))       /* create expression for the array index */
        rvalue(&lval2);
      if (lval2.ident==iARRAY || lval2.ident==iREFARRAY)
        error(33,lval2.sym->name);      /* array must be indexed */
      needtoken(close);
      if ((sym->usage & uENUMROOT) && !matchtag(sym->x.tags.index,lval2.tag,TRUE))
        error(213);
      if (lval2.ident==iCONSTEXPR) {    /* constant expression */
        stgdel(index,cidx);             /* scratch generated code */
        if (lval1->arrayidx!=NULL) {    /* keep constant index, for checking */
          assert(sym->dim.array.level>=0 && sym->dim.array.level<sDIMEN_MAX);
          lval1->arrayidx[sym->dim.array.level]=lval2.constval;
        } /* if */
        if (close==']' && !(sym->tag == pc_tag_string && sym->dim.array.level == 0)) {
          /* normal array index */
          if (lval2.constval<0 || sym->dim.array.length!=0 && sym->dim.array.length<=lval2.constval)
            error(32,sym->name);        /* array index out of bounds */
          if (lval2.constval!=0) {
            /* don't add offsets for zero subscripts */
            #if PAWN_CELL_SIZE==16
              ldconst(lval2.constval<<1,sALT);
            #elif PAWN_CELL_SIZE==32
              ldconst(lval2.constval<<2,sALT);
            #elif PAWN_CELL_SIZE==64
              ldconst(lval2.constval<<3,sALT);
            #else
              #error Unsupported cell size
            #endif
            ob_add();
          } /* if */
        } else {
          /* character index */
          if (lval2.constval<0 || sym->dim.array.length!=0
              && sym->dim.array.length*((8*sizeof(cell))/sCHARBITS)<=(ucell)lval2.constval)
            error(32,sym->name);        /* array index out of bounds */
          if (lval2.constval!=0) {
            /* don't add offsets for zero subscripts */
            #if sCHARBITS==16
              ldconst(lval2.constval<<1,sALT);/* 16-bit character */
            #else
              ldconst(lval2.constval,sALT);   /* 8-bit character */
            #endif
            ob_add();
          } /* if */
          charalign();                  /* align character index into array */
        } /* if */
        /* if the array index is a field from an enumeration, get the tag name
         * from the field and save the size of the field too.
         */
        assert(lval2.sym==NULL || lval2.sym->dim.array.level==0);
        if (lval2.sym!=NULL && lval2.sym->dim.array.length>0 && sym->dim.array.level==0) {
          lval1->tag=lval2.sym->x.tags.index;
          lval1->constval=lval2.sym->dim.array.length;
        } /* if */
      } else {
        /* array index is not constant */
        lval1->arrayidx=NULL;           /* reset, so won't be checked */
        if (close==']' && !magic_string) {
          if (sym->dim.array.length!=0)
            ffbounds(sym->dim.array.length-1);  /* run time check for array bounds */
          cell2addr();  /* normal array index */
        } else {
          if (sym->dim.array.length!=0)
            ffbounds(sym->dim.array.length*(32/sCHARBITS)-1);
          char2addr();  /* character array index */
        } /* if */
        popreg(sALT);
        ob_add();       /* base address was popped into secondary register */
        if (close!=']' || magic_string)
          charalign();  /* align character index into array */
      } /* if */
      /* the indexed item may be another array (multi-dimensional arrays) */
      assert(cursym==sym && sym!=NULL); /* should still be set */
      if (sym->dim.array.level>0) {
        assert(close==']');     /* checked earlier */
        assert(cursym==lval1->sym);
        /* read the offset to the subarray and add it to the current address */
        lval1->ident=iARRAYCELL;
        pushreg(sPRI);          /* the optimizer makes this to a MOVE.alt */
        rvalue(lval1);
        popreg(sALT);
        ob_add();
        /* adjust the "value" structure and find the referenced array */
        lval1->ident=iREFARRAY;
        lval1->sym=finddepend(sym);
        assert(lval1->sym!=NULL);
        assert(lval1->sym->dim.array.level==sym->dim.array.level-1);
        cursym=lval1->sym;
        /* try to parse subsequent array indices */
        lvalue=FALSE;   /* for now, a iREFARRAY is no lvalue */
        goto restart;
      } /* if */
      assert(sym->dim.array.level==0);
      /* set type to fetch... INDIRECTLY */
      if (sym->tag == pc_tag_string) {
        lval1->ident = iARRAYCHAR;
      } else {
        lval1->ident= (char)((close==']') ? iARRAYCELL : iARRAYCHAR);
      }
      /* if the array index is a field from an enumeration, get the tag name
       * from the field and save the size of the field too. Otherwise, the
       * tag is the one from the array symbol.
       */
      if (lval2.ident==iCONSTEXPR && lval2.sym!=NULL
          && lval2.sym->dim.array.length>0 && sym->dim.array.level==0)
      {
        lval1->tag=lval2.sym->x.tags.index;
        lval1->constval=lval2.sym->dim.array.length;
        if (lval2.tag==sym->x.tags.index && lval1->constval>1 && matchtoken('[')) {
          /* an array indexed with an enumeration field may be considered a sub-array */
          lexpush();
          lvalue=FALSE;   /* for now, a iREFARRAY is no lvalue */
          lval1->ident=iREFARRAY;
          /* initialize a dummy symbol, which is a copy of the current symbol,
           * but with an adjusted index tag
           */
          assert(sym!=NULL);
          dummysymbol=*sym;
          /* get the tag of the root of the enumeration */
          assert(lval2.sym!=NULL);
          dummysymbol.x.tags.index=lval2.sym->x.tags.field;
          dummysymbol.dim.array.length=lval2.sym->dim.array.length;
          cursym=&dummysymbol;
          /* recurse */
          goto restart;
        } /* if */
      } else {
        assert(sym!=NULL);
        if (cursym!=&dummysymbol)
          lval1->tag=sym->tag;
        lval1->constval=0;
      } /* if */
      /* a cell in an array is an lvalue, a character in an array is not
       * always a *valid* lvalue */
      return TRUE;
    } else {            /* tok=='(' -> function(...) */
      assert(tok=='(');
      if (sym==NULL
          || (sym->ident!=iFUNCTN && sym->ident!=iREFFUNC))
      {
        if (sym==NULL && sc_status==statFIRST) {
          /* could be a "use before declaration"; in that case, create a stub
           * function so that the usage can be marked.
           */
          sym=fetchfunc(lastsymbol,0);
          if (sym==NULL)
            error(103); /* insufficient memory */
          markusage(sym,uREAD);
        } else {
          return error(12);           /* invalid function call */
        } /* if */
      } else if ((sym->usage & uMISSING)!=0) {
        char symname[2*sNAMEMAX+16];  /* allow space for user defined operators */
        funcdisplayname(symname,sym->name);
        error(4,symname);             /* function not defined */
      } /* if */
      callfunction(sym,lval1,TRUE);
      return FALSE;             /* result of function call is no lvalue */
    } /* if */
  } /* if */
  if (sym!=NULL && lval1->ident==iFUNCTN) {
    assert(sym->ident==iFUNCTN);
    if (sc_allowproccall) {
      callfunction(sym,lval1,FALSE);
    } else {
      symbol *oldsym=sym;
      int n=-1,iter=0;
      int usage = ((sym->usage & uPUBLIC) == uPUBLIC) ? uPUBLIC : 0;
      cell code_addr=0;
      for (sym=glbtab.next; sym!=NULL; sym=sym->next) {
        if (sym->ident==iFUNCTN && sym->vclass == sGLOBAL && (!usage || (sym->usage & usage)))
        {
          if (strcmp(sym->name, lval1->sym->name)==0) {
            n = iter;
            code_addr = sym->codeaddr;
            break;
          }
          iter++;
        }
      }
      if (n!=-1) {
        char faketag[sNAMEMAX+1];
        lval1->sym=NULL;
        lval1->ident=iCONSTEXPR;
        /* Generate a quick pseudo-tag! */
        if (usage == uPUBLIC) {
          lval1->constval=(n<<1)|1;
          snprintf(faketag, sizeof(faketag)-1, "$Func@%d", n);
        } else {
          lval1->constval=(code_addr<<1)|0;
          snprintf(faketag, sizeof(faketag)-1, "$Func!%d", code_addr);
        }
        lval1->tag=pc_addfunctag(faketag);
        oldsym->usage |= uREAD;
      } else {
        error(76);                /* invalid function call, or syntax error */
      } /* if */
      return FALSE;
    }
  } /* if */
  return lvalue;
}

/*  primary
 *
 *  Returns 1 if the operand is an lvalue (everything except arrays, functions
 *  constants and -of course- errors).
 *  Generates code to fetch the address of arrays. Code for constants is
 *  already generated by constant().
 *  This routine first clears the entire lval array (all fields are set to 0).
 *
 *  Global references: sc_intest  (may be altered, but restored upon termination)
 */
static int primary(value *lval)
{
  char *st;
  int lvalue,tok;
  cell val;
  symbol *sym;

  if (matchtoken('(')){         /* sub-expression - (expression,...) */
    PUSHSTK_I(sc_intest);
    PUSHSTK_I(sc_allowtags);

    sc_intest=FALSE;            /* no longer in "test" expression */
    sc_allowtags=TRUE;          /* allow tagnames to be used in parenthesized expressions */
    sc_allowproccall=FALSE;
    do
      lvalue=hier14(lval);
    while (matchtoken(','));
    needtoken(')');
    lexclr(FALSE);              /* clear lex() push-back, it should have been
                                 * cleared already by needtoken() */
    sc_allowtags=(short)POPSTK_I();
    sc_intest=(short)POPSTK_I();
    return lvalue;
  } /* if */

  clear_value(lval);    /* clear lval */
  tok=lex(&val,&st);
  if (tok==tSYMBOL) {
    /* lastsymbol is char[sNAMEMAX+1], lex() should have truncated any symbol
     * to sNAMEMAX significant characters */
    assert(strlen(st)<sizeof lastsymbol);
    strcpy(lastsymbol,st);
  } /* if */
  if (tok==tSYMBOL && !findconst(st,NULL)) {
    /* first look for a local variable */
    if ((sym=findloc(st))!=0) {
      if (sym->ident==iLABEL) {
        error(29);          /* expression error, assumed 0 */
        ldconst(0,sPRI);    /* load 0 */
        return FALSE;       /* return 0 for labels (expression error) */
      } /* if */
      lval->sym=sym;
      lval->ident=sym->ident;
      lval->tag=sym->tag;
      if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
        address(sym,sPRI);  /* get starting address in primary register */
        return FALSE;       /* return 0 for array (not lvalue) */
      } else {
        return TRUE;        /* return 1 if lvalue (not label or array) */
      } /* if */
    } /* if */
    /* now try a global variable */
    if ((sym=findglb(st,sSTATEVAR))!=0) {
      if (sym->ident==iFUNCTN || sym->ident==iREFFUNC) {
        /* if the function is only in the table because it was inserted as a
         * stub in the first pass (i.e. it was "used" but never declared or
         * implemented, issue an error
         */
        if ((sym->usage & uPROTOTYPED)==0)
          error(17,st);
      } else {
        if ((sym->usage & uDEFINE)==0)
          error(17,st);
        lval->sym=sym;
        lval->ident=sym->ident;
        lval->tag=sym->tag;
        if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
          address(sym,sPRI);    /* get starting address in primary register */
          return FALSE;         /* return 0 for array (not lvalue) */
        } else {
          return TRUE;          /* return 1 if lvalue (not function or array) */
        } /* if */
      } /* if */
    } else {
      if (!sc_allowproccall)
        return error(17,st);    /* undefined symbol */
      /* an unknown symbol, but used in a way compatible with the "procedure
       * call" syntax. So assume that the symbol refers to a function.
       */
      assert(sc_status==statFIRST);
      sym=fetchfunc(st,0);
      if (sym==NULL)
        error(103);     /* insufficient memory */
    } /* if */
    assert(sym!=NULL);
    assert(sym->ident==iFUNCTN || sym->ident==iREFFUNC);
    lval->sym=sym;
    lval->ident=sym->ident;
    lval->tag=sym->tag;
    return FALSE;       /* return 0 for function (not an lvalue) */
  } /* if */
  lexpush();            /* push the token, it is analyzed by constant() */
  if (constant(lval)==0) {
    error(29);          /* expression error, assumed 0 */
    ldconst(0,sPRI);    /* load 0 */
  } /* if */
  return FALSE;         /* return 0 for constants (or errors) */
}

static void clear_value(value *lval)
{
  lval->sym=NULL;
  lval->constval=0L;
  lval->tag=0;
  lval->ident=0;
  lval->boolresult=FALSE;
  /* do not clear lval->arrayidx, it is preset in hier14() */
  /* do not clear lval->cmptag */
}

static void setdefarray(cell *string,cell size,cell array_sz,cell *dataaddr,int fconst)
{
  /* The routine must copy the default array data onto the heap, as to avoid
   * that a function can change the default value. An optimization is that
   * the default array data is "dumped" into the data segment only once (on the
   * first use).
   */
  assert(string!=NULL);
  assert(size>0);
  /* check whether to dump the default array */
  assert(dataaddr!=NULL);
  if (sc_status==statWRITE && *dataaddr<0) {
    int i;
    *dataaddr=(litidx+glb_declared)*sizeof(cell);
    for (i=0; i<size; i++)
      litadd(*string++);
  } /* if */

  /* if the function is known not to modify the array (meaning that it also
   * does not modify the default value), directly pass the address of the
   * array in the data segment.
   */
  if (fconst) {
    ldconst(*dataaddr,sPRI);
  } else {
    /* Generate the code:
     *  CONST.pri dataaddr                ;address of the default array data
     *  HEAP      array_sz*sizeof(cell)   ;heap address in ALT
     *  MOVS      size*sizeof(cell)       ;copy data from PRI to ALT
     *  MOVE.PRI                          ;PRI = address on the heap
     */
    ldconst(*dataaddr,sPRI);
    /* "array_sz" is the size of the argument (the value between the brackets
     * in the declaration), "size" is the size of the default array data.
     */
    assert(array_sz>=size);
    modheap((int)array_sz*sizeof(cell));
    markheap(MEMUSE_STATIC, array_sz);
    /* ??? should perhaps fill with zeros first */
    memcopy(size*sizeof(cell));
    moveto1();
  } /* if */
}

static int findnamedarg(arginfo *arg,char *name)
{
  int i;

  for (i=0; arg[i].ident!=0 && arg[i].ident!=iVARARGS; i++)
    if (strcmp(arg[i].name,name)==0)
      return i;
  return -1;
}

int checktag(int tags[],int numtags,int exprtag)
{
  int i;

  assert(tags!=0);
  assert(numtags>0);
  for (i=0; i<numtags; i++)
    if (matchtag(tags[i],exprtag,TRUE))
      return TRUE;    /* matching tag */
  return FALSE;       /* no tag matched */
}

enum {
  ARG_UNHANDLED,
  ARG_IGNORED,
  ARG_DONE,
};

/*  callfunction
 *
 *  Generates code to call a function. This routine handles default arguments
 *  and positional as well as named parameters.
 */
static void callfunction(symbol *sym,value *lval_result,int matchparanthesis)
{
static long nest_stkusage=0L;
static int nesting=0;
  int close,lvalue;
  int argpos;       /* index in the output stream (argpos==nargs if positional parameters) */
  int argidx=0;     /* index in "arginfo" list */
  int nargs=0;      /* number of arguments */
  int heapalloc=0;
  int namedparams=FALSE;
  value lval = {0};
  arginfo *arg;
  char arglist[sMAXARGS];
  constvalue arrayszlst = { NULL, "", 0, 0};/* array size list starts empty */
  constvalue taglst = { NULL, "", 0, 0};    /* tag list starts empty */
  symbol *symret;
  cell lexval;
  char *lexstr;

  assert(sym!=NULL);
  lval_result->ident=iEXPRESSION; /* preset, may be changed later */
  lval_result->constval=0;
  lval_result->tag=sym->tag;
  /* check whether this is a function that returns an array */
  symret=finddepend(sym);
  assert(symret==NULL || symret->ident==iREFARRAY);
  if (symret!=NULL) {
    int retsize;
    /* allocate space on the heap for the array, and pass the pointer to the
     * reserved memory block as a hidden parameter
     */
    retsize=(int)array_totalsize(symret);
    assert(retsize>0);
    modheap(retsize*sizeof(cell));/* address is in ALT */
    pushreg(sALT);                /* pass ALT as the last (hidden) parameter */
    markheap(MEMUSE_STATIC, retsize);
    /* also mark the ident of the result as "array" */
    lval_result->ident=iREFARRAY;
    lval_result->sym=symret;
  } /* if */
  pushheaplist();

  nesting++;
  assert(nest_stkusage>=0);
  #if !defined NDEBUG
    if (nesting==1)
      assert(nest_stkusage==0);
  #endif
  sc_allowproccall=FALSE;       /* parameters may not use procedure call syntax */

  if ((sym->flags & flgDEPRECATED)!=0) {
    char *ptr= (sym->documentation!=NULL) ? sym->documentation : "";
    error(234,sym->name,ptr);   /* deprecated (probably a native function) */
  } /* if */

  /* run through the arguments */
  arg=sym->dim.arglist;
  assert(arg!=NULL);
  stgmark(sSTARTREORDER);
  memset(arglist,ARG_UNHANDLED,sizeof arglist);
  if (matchparanthesis) {
    /* Opening brace was already parsed, if closing brace follows, this
     * call passes no parameters.
     */
    close=matchtoken(')');
  } else {
    /* When we find an end of line here, it may be a function call passing
     * no parameters, or it may be that the first parameter is on a line
     * below. But as a parameter can be anything, this is difficult to check.
     * The only simple check that we have is the use of "named parameters".
     */
    close=matchtoken(tTERM);
    if (close) {
      close=!matchtoken('.');
      if (!close)
        lexpush();                /* reset the '.' */
    } /* if */
  } /* if */
  if (!close) {
    do {
      if (matchtoken('.')) {
        namedparams=TRUE;
        if (needtoken(tSYMBOL))
          tokeninfo(&lexval,&lexstr);
        else
          lexstr="";
        argpos=findnamedarg(arg,lexstr);
        if (argpos<0) {
          error(17,lexstr);       /* undefined symbol */
          break;                  /* exit loop, argpos is invalid */
        } /* if */
        needtoken('=');
        argidx=argpos;
      } else {
        if (namedparams)
          error(44);   /* positional parameters must precede named parameters */
        argpos=nargs;
      } /* if */
      /* the number of arguments this was already checked at the declaration
       * of the function; check it again for functions with a variable
       * argument list
       */
      if (argpos>=sMAXARGS)
        error(45);                /* too many function arguments */
      stgmark((char)(sEXPRSTART+argpos));/* mark beginning of new expression in stage */
      if (arglist[argpos]!=ARG_UNHANDLED)
        error(58);                /* argument already set */
      if (matchtoken('_')) {
        arglist[argpos]=ARG_IGNORED;  /* flag argument as "present, but ignored" */
        if (arg[argidx].ident==0 || arg[argidx].ident==iVARARGS) {
          error(92);             /* argument count mismatch */
        } else if (!arg[argidx].hasdefault) {
          error(34,nargs+1);      /* argument has no default value */
        } /* if */
        if (arg[argidx].ident!=0 && arg[argidx].ident!=iVARARGS)
          argidx++;
        /* The rest of the code to handle default values is at the bottom
         * of this routine where default values for unspecified parameters
         * are (also) handled. Note that above, the argument is flagged as
         * ARG_IGNORED.
         */
      } else {
        arglist[argpos]=ARG_DONE; /* flag argument as "present" */
        if (arg[argidx].numtags==1)     /* set the expected tag, if any */
          lval.cmptag=arg[argidx].tags[0];
        lvalue=hier14(&lval);
        assert(sc_status==statFIRST || arg[argidx].tags!=NULL);
        switch (arg[argidx].ident) {
        case 0:
          error(92);             /* argument count mismatch */
          break;
        case iVARARGS:
          /* always pass by reference */
          if (lval.ident==iVARIABLE || lval.ident==iREFERENCE) {
            assert(lval.sym!=NULL);
            if ((lval.sym->usage & uCONST)!=0 && (arg[argidx].usage & uCONST)==0) {
              /* treat a "const" variable passed to a function with a non-const
               * "variable argument list" as a constant here */
              if (!lvalue) {
                error(22);        /* need lvalue */
              } else {
                rvalue(&lval);    /* get value in PRI */
                setheap_pri();    /* address of the value on the heap in PRI */
                heapalloc+=markheap(MEMUSE_STATIC, 1);
                nest_stkusage++;
              } /* if */
            } else if (lvalue) {
              address(lval.sym,sPRI);
            } else {
              setheap_pri();      /* address of the value on the heap in PRI */
              heapalloc+=markheap(MEMUSE_STATIC, 1);
              nest_stkusage++;
            } /* if */
          } else if (lval.ident==iCONSTEXPR || lval.ident==iEXPRESSION)
          {
            /* allocate a cell on the heap and store the
             * value (already in PRI) there */
            setheap_pri();        /* address of the value on the heap in PRI */
            heapalloc+=markheap(MEMUSE_STATIC, 1);
            nest_stkusage++;
          } /* if */
          /* ??? handle const array passed by reference */
          /* otherwise, the address is already in PRI */
          if (lval.sym!=NULL)
            markusage(lval.sym,uWRITTEN);
          if (!checktags_string(arg[argidx].tags, arg[argidx].numtags, &lval)
              && !checktag(arg[argidx].tags,arg[argidx].numtags,lval.tag))
            error(213);
          if (lval.tag!=0)
            append_constval(&taglst,arg[argidx].name,lval.tag,0);
          break;
        case iVARIABLE:
          if (lval.ident==iLABEL || lval.ident==iFUNCTN || lval.ident==iREFFUNC
              || lval.ident==iARRAY || lval.ident==iREFARRAY)
            error(35,argidx+1);   /* argument type mismatch */
          if (lvalue)
            rvalue(&lval);        /* get value (direct or indirect) */
          /* otherwise, the expression result is already in PRI */
          assert(arg[argidx].numtags>0);
          check_userop(NULL,lval.tag,arg[argidx].tags[0],2,NULL,&lval.tag);
          if (!checktags_string(arg[argidx].tags, arg[argidx].numtags, &lval)
              && !checktag(arg[argidx].tags,arg[argidx].numtags,lval.tag))
            error(213);
          if (lval.tag!=0)
            append_constval(&taglst,arg[argidx].name,lval.tag,0);
          argidx++;               /* argument done */
          break;
        case iREFERENCE:
          if (!lvalue)
            error(35,argidx+1);   /* argument type mismatch */
          if (lval.sym!=NULL && (lval.sym->usage & uCONST)!=0 && (arg[argidx].usage & uCONST)==0)
            error(35,argidx+1);   /* argument type mismatch */
          if (lval.ident==iVARIABLE || lval.ident==iREFERENCE) {
            if (lvalue) {
              assert(lval.sym!=NULL);
              address(lval.sym,sPRI);
            } else {
              setheap_pri();      /* address of the value on the heap in PRI */
              heapalloc+=markheap(MEMUSE_STATIC, 1);
              nest_stkusage++;
            } /* if */
          } /* if */
          /* otherwise, the address is already in PRI */
          if (!checktag(arg[argidx].tags,arg[argidx].numtags,lval.tag))
            error(213);
          if (lval.tag!=0)
            append_constval(&taglst,arg[argidx].name,lval.tag,0);
          argidx++;               /* argument done */
          if (lval.sym!=NULL)
            markusage(lval.sym,uWRITTEN);
          break;
        case iREFARRAY:
          if (lval.ident!=iARRAY && lval.ident!=iREFARRAY
              && lval.ident!=iARRAYCELL && lval.ident!=iARRAYCHAR)
          {
            error(35,argidx+1);   /* argument type mismatch */
            break;
          } /* if */
          if (lval.sym!=NULL && (lval.sym->usage & uCONST)!=0 && (arg[argidx].usage & uCONST)==0)
            error(35,argidx+1); /* argument type mismatch */
          /* Verify that the dimensions match with those in arg[argidx].
           * A literal array always has a single dimension.
           * An iARRAYCELL parameter is also assumed to have a single dimension.
           */
          if (lval.sym==NULL || lval.ident==iARRAYCELL || lval.ident==iARRAYCHAR) {
            if (arg[argidx].numdim!=1) {
              error(48);        /* array dimensions must match */
            } else if (arg[argidx].dim[0]!=0) {
              assert(arg[argidx].dim[0]>0);
              if (lval.ident==iARRAYCELL) {
                error(47);        /* array sizes must match */
              } else {
                assert(lval.constval!=0); /* literal array must have a size */
                /* A literal array must have exactly the same size as the
                 * function argument; a literal string may be smaller than
                 * the function argument.
                 */
                if (lval.constval>0 && arg[argidx].dim[0]!=lval.constval
                    || lval.constval<0 && arg[argidx].dim[0] < -lval.constval)
                  error(47);      /* array sizes must match */
              } /* if */
            } /* if */
            if (lval.ident!=iARRAYCELL && lval.ident!=iARRAYCHAR) {
              /* save array size, for default values with uSIZEOF flag */
              cell array_sz=lval.constval;
              assert(array_sz!=0);/* literal array must have a size */
              if (array_sz<0)
                array_sz= -array_sz;
              append_constval(&arrayszlst,arg[argidx].name,array_sz,0);
            }/* if */
          } else {
            symbol *sym=lval.sym;
            short level=0;
            assert(sym!=NULL);
            if (sym->dim.array.level+1!=arg[argidx].numdim)
              error(48);          /* array dimensions must match */
            /* the lengths for all dimensions must match, unless the dimension
             * length was defined at zero (which means "undefined")
             */
            while (sym->dim.array.level>0) {
              assert(level<sDIMEN_MAX);
              if (arg[argidx].dim[level]!=0 && sym->dim.array.length!=arg[argidx].dim[level])
                error(47);        /* array sizes must match */
              else if (!matchtag(arg[argidx].idxtag[level],sym->x.tags.index,TRUE))
                error(229,sym->name);   /* index tag mismatch */
              append_constval(&arrayszlst,arg[argidx].name,sym->dim.array.length,level);
              sym=finddepend(sym);
              assert(sym!=NULL);
              level++;
            } /* if */
            /* the last dimension is checked too, again, unless it is zero */
            assert(level<sDIMEN_MAX);
            assert(sym!=NULL);
            if (arg[argidx].dim[level]!=0 && sym->dim.array.length!=arg[argidx].dim[level])
              error(47);          /* array sizes must match */
            else if (!matchtag(arg[argidx].idxtag[level],sym->x.tags.index,TRUE))
              error(229,sym->name);   /* index tag mismatch */
            append_constval(&arrayszlst,arg[argidx].name,sym->dim.array.length,level);
          } /* if */
          /* address already in PRI */
          if (!checktag(arg[argidx].tags,arg[argidx].numtags,lval.tag))
            error(213);
          if (lval.tag!=0)
            append_constval(&taglst,arg[argidx].name,lval.tag,0);
          // ??? set uWRITTEN?
          argidx++;               /* argument done */
          break;
        } /* switch */
        pushreg(sPRI);            /* store the function argument on the stack */
        markexpr(sPARM,NULL,0);   /* mark the end of a sub-expression */
        nest_stkusage++;
      } /* if */
      assert(arglist[argpos]!=ARG_UNHANDLED);
      nargs++;
      if (matchparanthesis) {
        close=matchtoken(')');
        if (!close)               /* if not paranthese... */
          if (!needtoken(','))    /* ...should be comma... */
            break;                /* ...but abort loop if neither */
      } else {
        close=!matchtoken(',');
        if (close) {              /* if not comma... */
          if (needtoken(tTERM)==1)/* ...must be end of statement */
            lexpush();            /* push again, because end of statement is analised later */
        } /* if */
      } /* if */
    } while (!close && freading && !matchtoken(tENDEXPR)); /* do */
  } /* if */
  /* check remaining function arguments (they may have default values) */
  for (argidx=0; arg[argidx].ident!=0 && arg[argidx].ident!=iVARARGS; argidx++) {
    if (arglist[argidx]==ARG_DONE)
      continue;                 /* already seen and handled this argument */
    /* in this first stage, we also skip the arguments with uSIZEOF and uTAGOF;
     * these are handled last
     */
    if ((arg[argidx].hasdefault & uSIZEOF)!=0 || (arg[argidx].hasdefault & uTAGOF)!=0) {
      assert(arg[argidx].ident==iVARIABLE);
      continue;
    } /* if */
    stgmark((char)(sEXPRSTART+argidx));/* mark beginning of new expression in stage */
    if (arg[argidx].hasdefault) {
      if (arg[argidx].ident==iREFARRAY) {
        short level;
        setdefarray(arg[argidx].defvalue.array.data,
                    arg[argidx].defvalue.array.size,
                    arg[argidx].defvalue.array.arraysize,
                    &arg[argidx].defvalue.array.addr,
                    (arg[argidx].usage & uCONST)!=0);
        if ((arg[argidx].usage & uCONST)==0) {
          heapalloc+=arg[argidx].defvalue.array.arraysize;
          nest_stkusage+=arg[argidx].defvalue.array.arraysize;
        } /* if */
        /* keep the lengths of all dimensions of a multi-dimensional default array */
        assert(arg[argidx].numdim>0);
        if (arg[argidx].numdim==1) {
          append_constval(&arrayszlst,arg[argidx].name,arg[argidx].defvalue.array.arraysize,0);
        } else {
          for (level=0; level<arg[argidx].numdim; level++) {
            assert(level<sDIMEN_MAX);
            append_constval(&arrayszlst,arg[argidx].name,arg[argidx].dim[level],level);
          } /* for */
        } /* if */
      } else if (arg[argidx].ident==iREFERENCE) {
        setheap(arg[argidx].defvalue.val);
        /* address of the value on the heap in PRI */
        heapalloc+=markheap(MEMUSE_STATIC, 1);
        nest_stkusage++;
      } else {
        int dummytag=arg[argidx].tags[0];
        ldconst(arg[argidx].defvalue.val,sPRI);
        assert(arg[argidx].numtags>0);
        check_userop(NULL,arg[argidx].defvalue_tag,arg[argidx].tags[0],2,NULL,&dummytag);
        assert(dummytag==arg[argidx].tags[0]);
      } /* if */
      pushreg(sPRI);            /* store the function argument on the stack */
      markexpr(sPARM,NULL,0);   /* mark the end of a sub-expression */
      nest_stkusage++;
    } else {
      error(92,argidx);        /* argument count mismatch */
    } /* if */
    if (arglist[argidx]==ARG_UNHANDLED)
      nargs++;
    arglist[argidx]=ARG_DONE;
  } /* for */
  /* now a second loop to catch the arguments with default values that are
   * the "sizeof" or "tagof" of other arguments
   */
  for (argidx=0; arg[argidx].ident!=0 && arg[argidx].ident!=iVARARGS; argidx++) {
    constvalue *asz;
    cell array_sz;
    if (arglist[argidx]==ARG_DONE)
      continue;                 /* already seen and handled this argument */
    stgmark((char)(sEXPRSTART+argidx));/* mark beginning of new expression in stage */
    assert(arg[argidx].ident==iVARIABLE);           /* if "sizeof", must be single cell */
    /* if unseen, must be "sizeof" or "tagof" */
    assert((arg[argidx].hasdefault & uSIZEOF)!=0 || (arg[argidx].hasdefault & uTAGOF)!=0);
    if ((arg[argidx].hasdefault & uSIZEOF)!=0) {
      /* find the argument; if it isn't found, the argument's default value
       * was a "sizeof" of a non-array (a warning for this was already given
       * when declaring the function)
       */
      asz=find_constval(&arrayszlst,arg[argidx].defvalue.size.symname,
                        arg[argidx].defvalue.size.level);
      if (asz!=NULL) {
        array_sz=asz->value;
        if (array_sz==0)
          error(224,arg[argidx].name);    /* indeterminate array size in "sizeof" expression */
      } else {
        array_sz=1;
      } /* if */
    } else {
      asz=find_constval(&taglst,arg[argidx].defvalue.size.symname,
                        arg[argidx].defvalue.size.level);
      if (asz != NULL) {
        exporttag(asz->value);
        array_sz=asz->value | PUBLICTAG;  /* must be set, because it just was exported */
      } else {
        array_sz=0;
      } /* if */
    } /* if */
    ldconst(array_sz,sPRI);
    pushreg(sPRI);              /* store the function argument on the stack */
    markexpr(sPARM,NULL,0);
    nest_stkusage++;
    if (arglist[argidx]==ARG_UNHANDLED)
      nargs++;
    arglist[argidx]=ARG_DONE;
  } /* for */
  stgmark(sENDREORDER);         /* mark end of reversed evaluation */
  pushval((cell)nargs /* *sizeof(cell)*/ );
  nest_stkusage++;
  ffcall(sym,NULL,nargs);
  if (sc_status!=statSKIP)
    markusage(sym,uREAD);       /* do not mark as "used" when this call itself is skipped */
  if ((sym->usage & uNATIVE)!=0 &&sym->x.lib!=NULL)
    sym->x.lib->value += 1;     /* increment "usage count" of the library */
  if (symret!=NULL)
    popreg(sPRI);               /* pop hidden parameter as function result */
  sideeffect=TRUE;              /* assume functions carry out a side-effect */
  delete_consttable(&arrayszlst);     /* clear list of array sizes */
  delete_consttable(&taglst);   /* clear list of parameter tags */

  /* maintain max. amount of memory used */
  {
    long totalsize;
    totalsize=declared+heapalloc+1;   /* local variables & return value size,
                                       * +1 for PROC opcode */
    if (lval_result->ident==iREFARRAY)
      totalsize++;                    /* add hidden parameter (on the stack) */
    if ((sym->usage & uNATIVE)==0)
      totalsize++;                    /* add "call" opcode */
    totalsize+=nest_stkusage;
    assert(curfunc!=NULL);
    if (curfunc->x.stacksize<totalsize)
      curfunc->x.stacksize=totalsize;
    nest_stkusage-=nargs+heapalloc+1; /* stack/heap space, +1 for argcount param */
    /* if there is a syntax error in the script, the stack calculation is
     * probably incorrect; but we may not allow it to drop below zero
     */
    if (nest_stkusage<0)
      nest_stkusage=0;
  }

  /* scrap any arrays left on the heap, with the exception of the array that
   * this function has as a result (in other words, scrap all arrays on the
   * heap that caused by expressions in the function arguments)
   */
  popheaplist();
  nesting--;
}

/*  dbltest
 *
 *  Returns a non-zero value if lval1 an array and lval2 is not an array and
 *  the operation is addition or subtraction.
 *
 *  Returns the "shift" count (1 for 16-bit, 2 for 32-bit) to align a cell
 *  to an array offset.
 */
static int dbltest(void (*oper)(),value *lval1,value *lval2)
{
  if ((oper!=ob_add) && (oper!=ob_sub))
    return 0;
  if (lval1->ident!=iARRAY)
    return 0;
  if (lval2->ident==iARRAY)
    return 0;
  return sizeof(cell)/2;        /* 1 for 16-bit, 2 for 32-bit */
}

/*  commutative
 *
 *  Test whether an operator is commutative, i.e. x oper y == y oper x.
 *  Commutative operators are: +  (addition)
 *                             *  (multiplication)
 *                             == (equality)
 *                             != (inequality)
 *                             &  (bitwise and)
 *                             ^  (bitwise xor)
 *                             |  (bitwise or)
 *
 *  If in an expression, code for the left operand has been generated and
 *  the right operand is a constant and the operator is commutative, the
 *  precautionary "push" of the primary register is scrapped and the constant
 *  is read into the secondary register immediately.
 */
static int commutative(void (*oper)())
{
  return oper==ob_add || oper==os_mult
         || oper==ob_eq || oper==ob_ne
         || oper==ob_and || oper==ob_xor || oper==ob_or;
}

/*  constant
 *
 *  Generates code to fetch a number, a literal character (which is returned
 *  by lex() as a number as well) or a literal string (lex() stores the
 *  strings in the literal queue). If the operand was a number, it is stored
 *  in lval->constval.
 *
 *  The function returns 1 if the token was a constant or a string, 0
 *  otherwise.
 */
static int constant(value *lval)
{
  int tok,index,ident;
  cell val,item,cidx;
  char *st;
  symbol *sym;
  int cmptag=lval->cmptag;

  tok=lex(&val,&st);
  if (tok==tSYMBOL && (sym=findconst(st,&cmptag))!=0) {
    if (cmptag>1)
      error(91,sym->name);  /* ambiguity: multiple matching constants (different tags) */
    lval->constval=sym->addr;
    ldconst(lval->constval,sPRI);
    lval->ident=iCONSTEXPR;
    lval->tag=sym->tag;
    lval->sym=sym;
    markusage(sym,uREAD);
  } else if (tok==tNUMBER) {
    lval->constval=val;
    ldconst(lval->constval,sPRI);
    lval->ident=iCONSTEXPR;
  } else if (tok==tRATIONAL) {
    lval->constval=val;
    ldconst(lval->constval,sPRI);
    lval->ident=iCONSTEXPR;
    lval->tag=sc_rationaltag;
  } else if (tok==tSTRING) {
    /* lex() stores starting index of string in the literal table in 'val' */
    ldconst((val+glb_declared)*sizeof(cell),sPRI);
    lval->ident=iARRAY;         /* pretend this is a global array */
    lval->constval=val-litidx;  /* constval == the negative value of the
                                 * size of the literal array; using a negative
                                 * value distinguishes between literal arrays
                                 * and literal strings (this was done for
                                 * array assignment). */
	lval->tag=pc_tag_string;
  } else if (tok=='{') {
    int tag,lasttag=-1;
    val=litidx;
    do {
      /* cannot call constexpr() here, because "staging" is already turned
       * on at this point */
      assert(staging);
      stgget(&index,&cidx);     /* mark position in code generator */
      ident=expression(&item,&tag,NULL,FALSE,NULL);
      stgdel(index,cidx);       /* scratch generated code */
      if (ident!=iCONSTEXPR)
        error(8);               /* must be constant expression */
      if (lasttag<0)
        lasttag=tag;
      else if (!matchtag(lasttag,tag,FALSE))
        error(213);             /* tagname mismatch */
      litadd(item);             /* store expression result in literal table */
    } while (matchtoken(','));
    if (!needtoken('}'))
      lexclr(FALSE);
    ldconst((val+glb_declared)*sizeof(cell),sPRI);
    lval->ident=iARRAY;         /* pretend this is a global array */
    lval->constval=litidx-val;  /* constval == the size of the literal array */
  } else {
    return FALSE;               /* no, it cannot be interpreted as a constant */
  } /* if */
  return TRUE;                  /* yes, it was a constant value */
}

