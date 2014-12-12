/* vim: set sts=2 ts=8 sw=2 tw=99 et: */
#ifndef _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
#define _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_

#define MEMUSE_STATIC      0
#define MEMUSE_DYNAMIC     1

typedef struct memuse_s {
  int type;   /* MEMUSE_STATIC or MEMUSE_DYNAMIC */
  int size;   /* size of array for static (0 for dynamic) */
  struct memuse_s *prev; /* previous block on the list */
} memuse_t;

typedef struct memuse_list_s {
  struct memuse_list_s *prev;   /* last used list */
  int list_id;
  memuse_t *head;               /* head of the current list */
} memuse_list_t;

typedef struct funcarg_s
{
  int tagcount;
  int tags[sTAGS_MAX];
  int dimcount;
  int dims[sDIMEN_MAX];
  int ident;
  int fconst;
  int ommittable;
} funcarg_t;

typedef struct functag_s
{
  int ret_tag;
  int usage;
  int argcount;
  int ommittable;
  funcarg_t args[sARGS_MAX];
  struct functag_s *next;
} functag_t;

typedef struct funcenum_s
{
  int tag;
  char name[METHOD_NAMEMAX+1];
  functag_t *first;
  functag_t *last;
  struct funcenum_s *next;
} funcenum_t;

typedef struct structarg_s
{
  int tag;
  int dimcount;
  int dims[sDIMEN_MAX];
  char name[sNAMEMAX+1];
  int fconst;
  int ident;
  unsigned int offs;
  int index;
} structarg_t;

typedef struct pstruct_s
{
  int argcount;
  char name[sNAMEMAX+1];
  structarg_t **args;
  struct pstruct_s *next;
} pstruct_t;

// The ordering of these definitions should be preserved for
// can_redef_layout_spec().
typedef enum LayoutSpec_t
{
  Layout_None,
  Layout_Enum,
  Layout_FuncTag,
  Layout_PawnStruct,
  Layout_MethodMap,
  Layout_Class
} LayoutSpec;

typedef struct methodmap_method_s
{
  char name[METHOD_NAMEMAX + 1];
  symbol *target;
  symbol *getter;
  symbol *setter;
  bool is_static;

  int property_tag() const {
    assert(getter || setter);
    if (getter)
      return getter->tag;
    arginfo *thisp = &setter->dim.arglist[0];
    if (thisp->ident == 0)
      return pc_tag_void;
    arginfo *valp = &setter->dim.arglist[1];
    if (valp->ident != iVARIABLE || valp->numtags != 1)
      return pc_tag_void;
    return valp->tags[0];
  }
} methodmap_method_t;

struct methodmap_t
{
  methodmap_t *next;
  methodmap_t *parent;
  int tag;
  int nullable;
  LayoutSpec spec;
  char name[sNAMEMAX+1];
  methodmap_method_t **methods;
  size_t nummethods;

  // Shortcut.
  methodmap_method_t *dtor;
  methodmap_method_t *ctor;
};

/**
 * Pawn Structs
 */
pstruct_t *pstructs_add(const char *name);
void pstructs_free();
pstruct_t *pstructs_find(const char *name);
structarg_t *pstructs_addarg(pstruct_t *pstruct, const structarg_t *arg);
structarg_t *pstructs_getarg(pstruct_t *pstruct, const char *member);

/**
 * Function enumeration tags
 */
void funcenums_free();
funcenum_t *funcenums_add(const char *name);
funcenum_t *funcenums_find_by_tag(int tag);
functag_t *functags_add(funcenum_t *en, functag_t *src);
funcenum_t *funcenum_for_symbol(symbol *sym);
functag_t *functag_find_intrinsic(int tag);

/**
 * Given a name or tag, find any extra weirdness it has associated with it.
 */
LayoutSpec deduce_layout_spec_by_tag(int tag);
LayoutSpec deduce_layout_spec_by_name(const char *name);
const char *layout_spec_name(LayoutSpec spec);
int can_redef_layout_spec(LayoutSpec olddef, LayoutSpec newdef);

/**
 * Heap functions
 */
void pushheaplist();
memuse_list_t *popsaveheaplist();
void popheaplist();
int markheap(int type, int size);

/**
 * Stack functions
 */
void pushstacklist();
void popstacklist(int codegen);
int markstack(int type, int size);
/**
 * Generates code to free mem usage, but does not pop the list.  
 *  This is used for code like dobreak()/docont()/doreturn().
 * stop_id is the list at which to stop generating.
 */
void genstackfree(int stop_id);
void genheapfree(int stop_id);

/**
 * Resets a mem list by freeing everything
 */
void resetstacklist();
void resetheaplist();

/**
 * Method maps.
 */
void methodmap_add(methodmap_t *map);
methodmap_t *methodmap_find_by_tag(int tag);
methodmap_t *methodmap_find_by_name(const char *name);
methodmap_method_t *methodmap_find_method(methodmap_t *map, const char *name);
void methodmaps_free();

extern memuse_list_t *heapusage;
extern memuse_list_t *stackusage;

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);

#endif //_INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
