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
  cell dims[sDIMEN_MAX];
  int ident;
  int fconst;
  int ommittable;
} funcarg_t;

typedef struct functag_s
{
  int ret_tag;
  int type;
  int argcount;
  int ommittable;
  funcarg_t args[sARGS_MAX];
  struct functag_s *next;
} functag_t;

typedef struct funcenum_s
{
  int value;
  char name[sNAMEMAX+1];
  functag_t *first;
  functag_t *last;
  struct funcenum_s *next;
} funcenum_t;

typedef struct structarg_s
{
  int tag;
  int dimcount;
  cell dims[sDIMEN_MAX];
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
funcenum_t *funcenums_find_byval(int value);
functag_t *functags_add(funcenum_t *en, functag_t *src);

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

extern memuse_list_t *heapusage;
extern memuse_list_t *stackusage;

#endif //_INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
