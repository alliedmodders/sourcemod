/*  Pawn compiler
 *
 *  Machine and state maintenance.
 *
 *  Three lists are maintained here:
 *  - A list of automatons (state machines): these hold a name, a unique id
 *    (in the "index" field) and the memory address of a cell that holds the
 *    current state of the automaton (in the "value" field).
 *  - A list of states for each automaton: a name, an automaton id (in the
 *    "index" field) and a unique id for the state (unique in the automaton;
 *    states belonging to different automatons may have the same id).
 *  - A list of state combinations. Each function may belong to a set of states.
 *    This list assigns a unique id to the combination of the automaton and all
 *    states.
 *
 *  For a function/variable that has states, there is a fourth list, which is
 *  attached to the "symbol" structure. This list contains the code label (in
 *  the "name" field, only for functions), the id of the state combinations (the
 *  state list id; it is stored in the "index" field) and the code address at
 *  which the function starts. The latter is currently unused.
 *
 *  At the start of the compiled code, a set of stub functions is generated.
 *  Each stub function looks up the value of the "state selector" value for the
 *  automaton, and goes with a "switch" instruction to the start address of the
 *  function. This happens in SC4.C.
 *
 *
 *  Copyright (c) ITB CompuPhase, 2005-2006
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
 *  Version: $Id: scstate.c 3579 2006-06-06 13:35:29Z thiadmer $
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sc.h"
#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
  #include <sclinux.h>
#endif

#if defined FORTIFY
  #include <alloc/fortify.h>
#endif

typedef struct s_statelist {
  struct s_statelist *next;
  int *states;          /* list of states in this combination */
  int numstates;        /* number of items in the above list */
  int fsa;              /* automaton id */
  int listid;           /* unique id for this combination list */
} statelist;

static statelist statelist_tab = { NULL, NULL, 0, 0, 0};   /* state combinations table */


static constvalue *find_automaton(const char *name,int *last)
{
  constvalue *ptr;

  assert(last!=NULL);
  *last=0;
  ptr=sc_automaton_tab.next;
  while (ptr!=NULL) {
    if (strcmp(name,ptr->name)==0)
      return ptr;
    if (ptr->index>*last)
      *last=ptr->index;
    ptr=ptr->next;
  } /* while */
  return NULL;
}

SC_FUNC constvalue *automaton_add(const char *name)
{
  constvalue *ptr;
  int last;

  assert(strlen(name)<sizeof(ptr->name));
  ptr=find_automaton(name,&last);
  if (ptr==NULL) {
    assert(last+1 <= SHRT_MAX);
    ptr=append_constval(&sc_automaton_tab,name,(cell)0,(short)(last+1));
  } /* if */
  return ptr;
}

SC_FUNC constvalue *automaton_find(const char *name)
{
  int last;
  return find_automaton(name,&last);
}

SC_FUNC constvalue *automaton_findid(int id)
{
  constvalue *ptr;
  for (ptr=sc_automaton_tab.next; ptr!=NULL && ptr->index!=id; ptr=ptr->next)
    /* nothing */;
  return ptr;
}


static constvalue *find_state(const char *name,int fsa,int *last)
{
  constvalue *ptr;

  assert(last!=NULL);
  *last=0;
  ptr=sc_state_tab.next;
  while (ptr!=NULL) {
    if (ptr->index==fsa) {
      if (strcmp(name,ptr->name)==0)
        return ptr;
      if ((int)ptr->value>*last)
        *last=(int)ptr->value;
    } /* if */
    ptr=ptr->next;
  } /* while */
  return NULL;
}

SC_FUNC constvalue *state_add(const char *name,int fsa)
{
  constvalue *ptr;
  int last;

  assert(strlen(name)<sizeof(ptr->name));
  ptr=find_state(name,fsa,&last);
  if (ptr==NULL) {
    assert(fsa <= SHRT_MAX);
    ptr=append_constval(&sc_state_tab,name,(cell)(last+1),(short)fsa);
  } /* if */
  return ptr;
}

SC_FUNC constvalue *state_find(const char *name,int fsa_id)
{
  int last;     /* dummy */
  return find_state(name,fsa_id,&last);
}

SC_FUNC constvalue *state_findid(int id)
{
  constvalue *ptr;
  for (ptr=sc_state_tab.next; ptr!=NULL && ptr->value!=id; ptr=ptr->next)
    /* nothing */;
  return ptr;
}

SC_FUNC void state_buildlist(int **list,int *listsize,int *count,int stateid)
{
  int idx;

  assert(list!=NULL);
  assert(listsize!=NULL);
  assert(*listsize>=0);
  assert(count!=NULL);
  assert(*count>=0);
  assert(*count<=*listsize);

  if (*count==*listsize) {
    /* To avoid constantly calling malloc(), the list is grown by 4 states at
     * a time.
     */
    *listsize+=4;
    *list=(int*)realloc(*list,*listsize*sizeof(int));
    if (*list==NULL)
      error(103);               /* insufficient memory */
  } /* if */

  /* find the insertion point (the list has to stay sorted) */
  for (idx=0; idx<*count && *list[idx]<stateid; idx++)
    /* nothing */;
  if (idx<*count)
    memmove(&(*list)[idx+1],&(*list)[idx],(int)((*count-idx+1)*sizeof(int)));
  (*list)[idx]=stateid;
  *count+=1;
}

static statelist *state_findlist(int *list,int count,int fsa,int *last)
{
  statelist *ptr;
  int i;

  assert(count>0);
  assert(last!=NULL);
  *last=0;
  ptr=statelist_tab.next;
  while (ptr!=NULL) {
    if (ptr->listid>*last)
      *last=ptr->listid;
    if (ptr->fsa==fsa && ptr->numstates==count) {
      /* compare all states */
      for (i=0; i<count && ptr->states[i]==list[i]; i++)
        /* nothing */;
      if (i==count)
        return ptr;
    } /* if */
    ptr=ptr->next;
  } /* while */
  return NULL;
}

static statelist *state_getlist_ptr(int listid)
{
  statelist *ptr;

  assert(listid>0);
  for (ptr=statelist_tab.next; ptr!=NULL && ptr->listid!=listid; ptr=ptr->next)
    /* nothing */;
  return ptr;
}

SC_FUNC int state_addlist(int *list,int count,int fsa)
{
  statelist *ptr;
  int last;

  assert(list!=NULL);
  assert(count>0);
  ptr=state_findlist(list,count,fsa,&last);
  if (ptr==NULL) {
    if ((ptr=(statelist*)malloc(sizeof(statelist)))==NULL)
      error(103);       /* insufficient memory */
    if ((ptr->states=(int*)malloc(count*sizeof(int)))==NULL) {
      free(ptr);
      error(103);       /* insufficient memory */
    } /* if */
    memcpy(ptr->states,list,count*sizeof(int));
    ptr->numstates=count;
    ptr->fsa=fsa;
    ptr->listid=last+1;
    ptr->next=statelist_tab.next;
    statelist_tab.next=ptr;
  } /* if */
  assert(ptr!=NULL);
  return ptr->listid;
}

SC_FUNC void state_deletetable(void)
{
  statelist *ptr;

  while (statelist_tab.next!=NULL) {
    ptr=statelist_tab.next;
    /* unlink first */
    statelist_tab.next=ptr->next;
    /* then delete */
    assert(ptr->states!=NULL);
    free(ptr->states);
    free(ptr);
  } /* while */
}

SC_FUNC int state_getfsa(int listid)
{
  statelist *ptr;

  assert(listid>=0);
  if (listid==0)
    return -1;

  ptr=state_getlist_ptr(listid);
  return (ptr!=NULL) ? ptr->fsa : -1; /* fsa 0 exists */
}

SC_FUNC int state_count(int listid)
{
  statelist *ptr=state_getlist_ptr(listid);
  if (ptr==NULL)
    return 0;           /* unknown list, no states in it */
  return ptr->numstates;
}

SC_FUNC int state_inlist(int listid,int state)
{
  statelist *ptr;
  int i;

  ptr=state_getlist_ptr(listid);
  if (ptr==NULL)
    return FALSE;       /* unknown list, state not in it */
  for (i=0; i<ptr->numstates; i++)
    if (ptr->states[i]==state)
      return TRUE;
  return FALSE;
}

SC_FUNC int state_listitem(int listid,int index)
{
  statelist *ptr;

  ptr=state_getlist_ptr(listid);
  assert(ptr!=NULL);
  assert(index>=0 && index<ptr->numstates);
  return ptr->states[index];
}

static int checkconflict(statelist *psrc,statelist *ptgt)
{
  int s,t;

  assert(psrc!=NULL);
  assert(ptgt!=NULL);
  for (s=0; s<psrc->numstates; s++)
    for (t=0; t<ptgt->numstates; t++)
      if (psrc->states[s]==ptgt->states[t])
        return 1;       /* state conflict */
  return 0;
}

/* This function searches whether one of the states in the list of statelist id's
 * of a symbol exists in any other statelist id's of the same function; it also
 * verifies that all definitions of the symbol are in the same automaton.
 */
SC_FUNC void state_conflict(symbol *root)
{
  statelist *psrc,*ptgt;
  constvalue *srcptr,*tgtptr;
  symbol *sym;

  assert(root!=NULL);
  for (sym=root->next; sym!=NULL; sym=sym->next) {
    if (sym->parent!=NULL || sym->ident!=iFUNCTN)
      continue;                 /* hierarchical data type or no function */
    if (sym->states==NULL)
      continue;                 /* this function has no states */
    for (srcptr=sym->states->next; srcptr!=NULL; srcptr=srcptr->next) {
      if (srcptr->index==-1)
        continue;               /* state list id -1 is a special case */
      psrc=state_getlist_ptr(srcptr->index);
      assert(psrc!=NULL);
      for (tgtptr=srcptr->next; tgtptr!=NULL; tgtptr=tgtptr->next) {
        if (tgtptr->index==-1)
          continue;             /* state list id -1 is a special case */
        ptgt=state_getlist_ptr(tgtptr->index);
        assert(ptgt!=NULL);
        if (psrc->fsa!=ptgt->fsa && strcmp(sym->name,uENTRYFUNC)!=0)
          error(83,sym->name);  /* this function is part of another machine */
        if (checkconflict(psrc,ptgt))
          error(84,sym->name);  /* state conflict */
      } /* for (tgtptr) */
    } /* for (srcptr) */
  } /* for (sym) */
}

/* check whether the two state lists (whose ids are passed in) share any
 * states
 */
SC_FUNC int state_conflict_id(int listid1,int listid2)
{
  statelist *psrc,*ptgt;

  psrc=state_getlist_ptr(listid1);
  assert(psrc!=NULL);
  ptgt=state_getlist_ptr(listid2);
  assert(ptgt!=NULL);
  return checkconflict(psrc,ptgt);
}
