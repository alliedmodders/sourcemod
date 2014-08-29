/* vim: set ts=8 sts=2 sw=2 tw=99 et: */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "sc.h"
#include "sctracker.h"

memuse_list_t *heapusage = NULL;
memuse_list_t *stackusage = NULL;
funcenum_t *firstenum = NULL;
funcenum_t *lastenum = NULL;
pstruct_t *firststruct = NULL;
pstruct_t *laststruct = NULL;
methodmap_t *methodmap_first = NULL;
methodmap_t *methodmap_last = NULL;

structarg_t *pstructs_getarg(pstruct_t *pstruct, const char *member)
{
  int i;

  for (i=0; i<pstruct->argcount; i++) {
    if (strcmp(pstruct->args[i]->name, member) == 0)
      return pstruct->args[i];
  }

  return NULL;
}

pstruct_t *pstructs_add(const char *name)
{
  pstruct_t *p = (pstruct_t *)malloc(sizeof(pstruct_t));
  
  memset(p, 0, sizeof(pstruct_t));
  strcpy(p->name, name);
  
  if (!firststruct) {
    firststruct = p;
    laststruct = p;
  } else {
    laststruct->next = p;
    laststruct = p;
  }

  return p;
}

void pstructs_free()
{
  pstruct_t *p, *next;

  p = firststruct;
  while (p) {
    while (p->argcount--)
      free(p->args[p->argcount]);
    free(p->args);
    next = p->next;
    free(p);
    p = next;
  }
  firststruct = NULL;
  laststruct = NULL;
}

pstruct_t *pstructs_find(const char *name)
{
  pstruct_t *p = firststruct;

  while (p) {
    if (strcmp(p->name, name) == 0)
      return p;
    p = p->next;
  }

  return NULL;
}

structarg_t *pstructs_addarg(pstruct_t *pstruct, const structarg_t *arg)
{
  structarg_t *newarg;
  int i;

  for (i=0; i<pstruct->argcount; i++) {
    if (strcmp(pstruct->args[i]->name, arg->name) == 0) {
      /* Don't allow dup names */
      return NULL;
    }
  }
  
  newarg = (structarg_t *)malloc(sizeof(structarg_t));

  memcpy(newarg, arg, sizeof(structarg_t));

  if (pstruct->argcount == 0) {
    pstruct->args = (structarg_t **)malloc(sizeof(structarg_t *) * 1);
  } else {
    pstruct->args = (structarg_t **)realloc(
              pstruct->args,
              sizeof(structarg_t *) * (pstruct->argcount + 1));
  }

  newarg->offs = pstruct->argcount * sizeof(cell);
  newarg->index = pstruct->argcount;
  pstruct->args[pstruct->argcount++] = newarg;

  return newarg;
}

void funcenums_free()
{
  funcenum_t *e, *next;

  e = firstenum;
  while (e) {
    functag_t *tag, *nexttag;
    tag = e->first;
    while (tag) {
      nexttag = tag->next;
      free(tag);
      tag = nexttag;
    }
    next = e->next;
    free(e);
    e = next;
  }

  firstenum = NULL;
  lastenum = NULL;
}

funcenum_t *funcenums_find_by_tag(int tag)
{
  funcenum_t *e = firstenum;

  while (e) {
    if (e->tag == tag)
      return e;
    e = e->next;
  }

  return NULL;
}

funcenum_t *funcenums_add(const char *name)
{
  funcenum_t *e = (funcenum_t *)malloc(sizeof(funcenum_t));

  memset(e, 0, sizeof(funcenum_t));

  if (!firstenum) {
    firstenum = e;
    lastenum = e;
  } else {
    lastenum->next = e;
    lastenum = e;
  }

  strcpy(e->name, name);
  e->tag = pc_addtag_flags((char *)name, FIXEDTAG|FUNCTAG);

  return e;
}

funcenum_t *funcenum_for_symbol(symbol *sym)
{
  functag_t ft;
  memset(&ft, 0, sizeof(ft));

  ft.ret_tag = sym->tag;
  ft.usage = uPUBLIC & (sym->usage & uRETVALUE);
  ft.argcount = 0;
  ft.ommittable = FALSE;
  for (arginfo *arg = sym->dim.arglist; arg->ident; arg++) {
    funcarg_t *dest = &ft.args[ft.argcount++];

    dest->tagcount = arg->numtags;
    memcpy(dest->tags, arg->tags, arg->numtags * sizeof(int));

    dest->dimcount = arg->numdim;
    memcpy(dest->dims, arg->dim, arg->numdim * sizeof(int));

    dest->ident = arg->ident;
    dest->fconst = !!(arg->usage & uCONST);
    dest->ommittable = FALSE;
  }

  char name[METHOD_NAMEMAX+1];
  UTIL_Format(name, sizeof(name), "::ft:%s:%d:%d", sym->name, sym->addr, sym->codeaddr);

  funcenum_t *fe = funcenums_add(name);
  functags_add(fe, &ft);

  return fe;
}

// Finds a functag that was created intrinsically.
functag_t *functag_find_intrinsic(int tag)
{
  funcenum_t *fe = funcenums_find_by_tag(tag);
  if (!fe)
    return NULL;

  if (strncmp(fe->name, "::ft:", 5) != 0)
    return NULL;

  assert(fe->first && fe->first == fe->last);
  return fe->first;
}

functag_t *functags_add(funcenum_t *en, functag_t *src)
{
  functag_t *t = (functag_t *)malloc(sizeof(functag_t));
  
  memcpy(t, src, sizeof(functag_t));

  t->next = NULL;

  if (en->first == NULL) {
    en->first = t;
    en->last = t;
  } else {
    en->last->next = t;
    en->last = t;
  }

  return t;
}

/**
 * Creates a new mem usage tracker entry
 */
void _push_memlist(memuse_list_t **head)
{
  memuse_list_t *newlist = (memuse_list_t *)malloc(sizeof(memuse_list_t));
  if (*head != NULL)
  {
    newlist->list_id = (*head)->list_id + 1;
  } else {
    newlist->list_id = 0;
  }
  newlist->prev = *head;
  newlist->head = NULL;
  *head = newlist;
}

/**
 * Pops a heap list but does not free it.
 */
memuse_list_t *_pop_save_memlist(memuse_list_t **head)
{
  memuse_list_t *oldlist = *head;
  *head = (*head)->prev;
  return oldlist;
}

/**
 * Marks a memory usage on a memory list
 */
int _mark_memlist(memuse_list_t *head, int type, int size)
{
  memuse_t *use;
  if (type==MEMUSE_STATIC && size==0)
  {
    return 0;
  }
  use=head->head;
  if (use && (type==MEMUSE_STATIC) 
      && (use->type == type))
  {
    use->size += size;
  } else {
    use=(memuse_t *)malloc(sizeof(memuse_t));
    use->type=type;
    use->size=size;
    use->prev=head->head;
    head->head=use;
  }
  return size;
}

void _reset_memlist(memuse_list_t **head)
{
  memuse_list_t *curlist = *head;
  memuse_list_t *tmplist;
  while (curlist) {
    memuse_t *curuse = curlist->head;
    memuse_t *tmpuse;
    while (curuse) {
      tmpuse = curuse->prev;
      free(curuse);
      curuse = tmpuse;
    }
    tmplist = curlist->prev;
    free(curlist);
    curlist = tmplist;
  }
  *head = NULL;
}


/**
 * Wrapper for pushing the heap list
 */
void pushheaplist()
{
  _push_memlist(&heapusage);
}

/**
 * Wrapper for popping and saving the heap list
 */
memuse_list_t *popsaveheaplist()
{
  return _pop_save_memlist(&heapusage);
}

/**
 * Wrapper for marking the heap
 */
int markheap(int type, int size)
{
  return _mark_memlist(heapusage, type, size);
}

/**
 * Wrapper for pushing the stack list
 */
void pushstacklist()
{
  _push_memlist(&stackusage);
}

/**
 * Wrapper for marking the stack
 */
int markstack(int type, int size)
{
  return _mark_memlist(stackusage, type, size);
}

/**
 * Generates code to free all heap allocations on a tracker
 */
void _heap_freeusage(memuse_list_t *heap, int dofree)
{
  memuse_t *cur=heap->head;
  memuse_t *tmp;
  while (cur) {
    if (cur->type == MEMUSE_STATIC) {
      modheap((-1)*cur->size*sizeof(cell));
    } else {
      modheap_i();
    }
    if (dofree) {
      tmp=cur->prev;
      free(cur);
      cur=tmp;
    } else {
      cur=cur->prev;
    }
  }
  if (dofree)
    heap->head=NULL;
}

void _stack_genusage(memuse_list_t *stack, int dofree)
{
  memuse_t *cur = stack->head;
  memuse_t *tmp;
  while (cur)
  {
    if (cur->type == MEMUSE_DYNAMIC)
    {
      /* no idea yet */
      assert(0);
    } else {
      modstk(cur->size * sizeof(cell));
    }
    if (dofree)
    {
      tmp = cur->prev;
      free(cur);
      cur = tmp;
    } else {
      cur = cur->prev;
    }
  }
  if (dofree)
  {
    stack->head = NULL;
  }
}

/**
 * Pops a heap list and frees it.
 */
void popheaplist()
{
  memuse_list_t *oldlist;
  assert(heapusage!=NULL);

  _heap_freeusage(heapusage, 1);
  assert(heapusage->head==NULL);

  oldlist=heapusage->prev;
  free(heapusage);
  heapusage=oldlist;
}

void genstackfree(int stop_id)
{
  memuse_list_t *curlist = stackusage;
  while (curlist && curlist->list_id > stop_id)
  {
    _stack_genusage(curlist, 0);
    curlist = curlist->prev;
  }
}

void genheapfree(int stop_id)
{
  memuse_list_t *curlist = heapusage;
  while (curlist && curlist->list_id > stop_id)
  {
    _heap_freeusage(curlist, 0);
    curlist = curlist->prev;
  }
}

void popstacklist(int codegen)
{
  memuse_list_t *oldlist;
  assert(stackusage != NULL);

  if (codegen)
  {
    _stack_genusage(stackusage, 1);
    assert(stackusage->head==NULL);
  } else {
    memuse_t *use = stackusage->head;
    while (use) {
      memuse_t *temp = use->prev;
      free(use);
      use = temp;
    }
  }

  oldlist = stackusage->prev;
  free(stackusage);
  stackusage = oldlist;
}

void resetstacklist()
{
  _reset_memlist(&stackusage);
}

void resetheaplist()
{
  _reset_memlist(&heapusage);
}

void methodmap_add(methodmap_t *map)
{
  if (!methodmap_first) {
    methodmap_first = map;
    methodmap_last = map;
  } else {
    methodmap_last->next = map;
    methodmap_last = map;
  }
}

methodmap_t *methodmap_find_by_tag(int tag)
{
  methodmap_t *ptr = methodmap_first;
  for (; ptr; ptr = ptr->next) {
    if (ptr->tag == tag)
      return ptr;
  }
  return NULL;
}

methodmap_t *methodmap_find_by_name(const char *name)
{
  int tag = pc_findtag(name);
  if (tag == -1)
    return NULL;
  return methodmap_find_by_tag(tag);
}

methodmap_method_t *methodmap_find_method(methodmap_t *map, const char *name)
{
  size_t i;
  for (i = 0; i < map->nummethods; i++) {
    if (strcmp(map->methods[i]->name, name) == 0)
      return map->methods[i];
  }
  if (map->parent)
    return methodmap_find_method(map->parent, name);
  return NULL;
}

void methodmaps_free()
{
  methodmap_t *ptr = methodmap_first;
  while (ptr) {
    methodmap_t *next = ptr->next;
    for (size_t i = 0; i < ptr->nummethods; i++)
      free(ptr->methods[i]);
    free(ptr->methods);
    free(ptr);
    ptr = next;
  }
  methodmap_first = NULL;
  methodmap_last = NULL;
}

LayoutSpec deduce_layout_spec_by_tag(int tag)
{
  symbol *sym;
  const char *name;
  methodmap_t *map;
  if ((map = methodmap_find_by_tag(tag)) != NULL)
    return map->spec;
  if (tag & FUNCTAG)
    return Layout_FuncTag;

  name = pc_tagname(tag);
  if (pstructs_find(name))
    return Layout_PawnStruct;
  if ((sym = findglb(name, sGLOBAL)) != NULL)
    return Layout_Enum;

  return Layout_None;
}

LayoutSpec deduce_layout_spec_by_name(const char *name)
{
  symbol *sym;
  methodmap_t *map;
  int tag = pc_findtag(name);
  if (tag != -1 && (tag & FUNCTAG))
    return Layout_FuncTag;
  if (pstructs_find(name))
    return Layout_PawnStruct;
  if ((map = methodmap_find_by_name(name)) != NULL)
    return map->spec;
  if ((sym = findglb(name, sGLOBAL)) != NULL)
    return Layout_Enum;

  return Layout_None;
}

const char *layout_spec_name(LayoutSpec spec)
{
  switch (spec) {
    case Layout_None:
      return "<none>";
    case Layout_Enum:
      return "enum";
    case Layout_FuncTag:
      return "functag";
    case Layout_PawnStruct:
      return "deprecated-struct";
    case Layout_MethodMap:
      return "methodmap";
    case Layout_Class:
      return "class";
  }
  return "<unknown>";
}

int can_redef_layout_spec(LayoutSpec def1, LayoutSpec def2)
{
  // Normalize the ordering, since these checks are symmetrical.
  if (def1 > def2) {
    LayoutSpec temp = def2;
    def2 = def1;
    def1 = temp;
  }

  switch (def1) {
    case Layout_None:
      return TRUE;
    case Layout_Enum:
      if (def2 == Layout_Enum || def2 == Layout_FuncTag)
        return TRUE;
      return def2 == Layout_MethodMap;
    case Layout_FuncTag:
      return def2 == Layout_Enum || def2 == Layout_FuncTag;
    case Layout_PawnStruct:
    case Layout_MethodMap:
      return FALSE;
    case Layout_Class:
      return FALSE;
  }
  return FALSE;
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  size_t len = vsnprintf(buffer, maxlength, fmt, ap);
  va_end(ap);

  if (len >= maxlength) {
    buffer[maxlength - 1] = '\0';
    return maxlength - 1;
  }
  return len;
}
