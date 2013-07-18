/*  Pawn compiler  - maintenance of various lists
 *
 *  o  Name list (aliases)
 *  o  Include path list
 *  o  Macro definitions (text substitutions)
 *  o  Documentation tags and automatic listings
 *  o  Debug strings
 *
 *  Copyright (c) ITB CompuPhase, 2001-2006
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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "sc.h"
#include "lstring.h"

#if defined FORTIFY
  #include <alloc/fortify.h>
#endif

/* a "private" implementation of strdup(), so that porting
 * to other memory allocators becomes easier.
 * By Søren Hannibal.
 */
SC_FUNC char* duplicatestring(const char* sourcestring)
{
  char* result=(char*)malloc(strlen(sourcestring)+1);
  strcpy(result,sourcestring);
  return result;
}


static stringpair *insert_stringpair(stringpair *root,char *first,char *second,int matchlength)
{
  stringpair *cur,*pred;

  assert(root!=NULL);
  assert(first!=NULL);
  assert(second!=NULL);
  /* create a new node, and check whether all is okay */
  if ((cur=(stringpair*)malloc(sizeof(stringpair)))==NULL)
    return NULL;
  cur->first=duplicatestring(first);
  cur->second=duplicatestring(second);
  cur->matchlength=matchlength;
  cur->documentation=NULL;
  if (cur->first==NULL || cur->second==NULL) {
    if (cur->first!=NULL)
      free(cur->first);
    if (cur->second!=NULL)
      free(cur->second);
    free(cur);
    return NULL;
  } /* if */
  /* link the node to the tree, find the position */
  for (pred=root; pred->next!=NULL && strcmp(pred->next->first,first)<0; pred=pred->next)
    /* nothing */;
  cur->next=pred->next;
  pred->next=cur;
  return cur;
}

static void delete_stringpairtable(stringpair *root)
{
  stringpair *cur, *next;

  assert(root!=NULL);
  cur=root->next;
  while (cur!=NULL) {
    next=cur->next;
    assert(cur->first!=NULL);
    assert(cur->second!=NULL);
    free(cur->first);
    free(cur->second);
    free(cur);
    cur=next;
  } /* while */
  memset(root,0,sizeof(stringpair));
}

static stringpair *find_stringpair(stringpair *cur,char *first,int matchlength)
{
  int result=0;

  assert(matchlength>0);  /* the function cannot handle zero-length comparison */
  assert(first!=NULL);
  while (cur!=NULL && result<=0) {
    result=(int)*cur->first - (int)*first;
    if (result==0 && matchlength==cur->matchlength) {
      result=strncmp(cur->first,first,matchlength);
      if (result==0)
        return cur;
    } /* if */
    cur=cur->next;
  } /* while */
  return NULL;
}

static int delete_stringpair(stringpair *root,stringpair *item)
{
  stringpair *cur;

  assert(root!=NULL);
  cur=root;
  while (cur->next!=NULL) {
    if (cur->next==item) {
      cur->next=item->next;     /* unlink from list */
      assert(item->first!=NULL);
      assert(item->second!=NULL);
      free(item->first);
      free(item->second);
      free(item);
      return TRUE;
    } /* if */
    cur=cur->next;
  } /* while */
  return FALSE;
}

/* ----- string list functions ----------------------------------- */
static stringlist *insert_string(stringlist *root,char *string)
{
  stringlist *cur;

  assert(string!=NULL);
  if ((cur=(stringlist*)malloc(sizeof(stringlist)))==NULL)
    error(103);       /* insufficient memory (fatal error) */
  if ((cur->line=duplicatestring(string))==NULL)
    error(103);       /* insufficient memory (fatal error) */
  cur->next=NULL;
  if (root->tail)
    root->tail->next=cur;
  else
    root->next=cur;
  root->tail=cur;
  return cur;
}

static char *get_string(stringlist *root,int index)
{
  stringlist *cur;

  assert(root!=NULL);
  cur=root->next;
  while (cur!=NULL && index-->0)
    cur=cur->next;
  if (cur!=NULL) {
    assert(cur->line!=NULL);
    return cur->line;
  } /* if */
  return NULL;
}

static int delete_string(stringlist *root,int index)
{
  stringlist *cur,*item;

  assert(root!=NULL);
  for (cur=root; cur->next!=NULL && index>0; cur=cur->next,index--)
    /* nothing */;
  if (cur->next!=NULL) {
    item=cur->next;
    if (root->tail == cur->next)
      root->tail = cur;
    cur->next=item->next;       /* unlink from list */
    assert(item->line!=NULL);
    free(item->line);
    free(item);
    return TRUE;
  } /* if */
  return FALSE;
}

SC_FUNC void delete_stringtable(stringlist *root)
{
  stringlist *cur,*next;

  assert(root!=NULL);
  cur=root->next;
  while (cur!=NULL) {
    next=cur->next;
    assert(cur->line!=NULL);
    free(cur->line);
    free(cur);
    cur=next;
  } /* while */
  memset(root,0,sizeof(stringlist));
}


/* ----- alias table --------------------------------------------- */
static stringpair alias_tab = {NULL, NULL, NULL};   /* alias table */

SC_FUNC stringpair *insert_alias(char *name,char *alias)
{
  stringpair *cur;

  assert(name!=NULL);
  assert(strlen(name)<=sNAMEMAX);
  assert(alias!=NULL);
  assert(strlen(alias)<=sNAMEMAX);
  if ((cur=insert_stringpair(&alias_tab,name,alias,strlen(name)))==NULL)
    error(103);       /* insufficient memory (fatal error) */
  return cur;
}

SC_FUNC int lookup_alias(char *target,char *name)
{
  stringpair *cur=find_stringpair(alias_tab.next,name,strlen(name));
  if (cur!=NULL) {
    assert(strlen(cur->second)<=sNAMEMAX);
    strcpy(target,cur->second);
  } /* if */
  return cur!=NULL;
}

SC_FUNC void delete_aliastable(void)
{
  delete_stringpairtable(&alias_tab);
}

/* ----- include paths list -------------------------------------- */
static stringlist includepaths;  /* directory list for include files */

SC_FUNC stringlist *insert_path(char *path)
{
  return insert_string(&includepaths,path);
}

SC_FUNC char *get_path(int index)
{
  return get_string(&includepaths,index);
}

SC_FUNC void delete_pathtable(void)
{
  delete_stringtable(&includepaths);
  assert(includepaths.next==NULL);
}


/* ----- text substitution patterns ------------------------------ */
#if !defined NO_DEFINE

static stringpair substpair = { NULL, NULL, NULL};  /* list of substitution pairs */

static stringpair *substindex['z'-PUBLIC_CHAR+1]; /* quick index to first character */
static void adjustindex(char c)
{
  stringpair *cur;
  assert((c>='A' && c<='Z') || (c>='a' && c<='z') || c=='_' || c==PUBLIC_CHAR);
  assert(PUBLIC_CHAR<'A' && 'A'<'_' && '_'<'z');

  for (cur=substpair.next; cur!=NULL && cur->first[0]!=c; cur=cur->next)
    /* nothing */;
  substindex[(int)c-PUBLIC_CHAR]=cur;
}

SC_FUNC stringpair *insert_subst(char *pattern,char *substitution,int prefixlen)
{
  stringpair *cur;

  assert(pattern!=NULL);
  assert(substitution!=NULL);
  if ((cur=insert_stringpair(&substpair,pattern,substitution,prefixlen))==NULL)
    error(103);       /* insufficient memory (fatal error) */
  adjustindex(*pattern);

  if (pc_deprecate!=NULL) {
	  assert(cur!=NULL);
	  cur->flags|=flgDEPRECATED;
	  if (sc_status==statWRITE) {
		  if (cur->documentation!=NULL) {
			  free(cur->documentation);
			  cur->documentation=NULL;
		  } /* if */
		  cur->documentation=pc_deprecate;
	  } else {
		  free(pc_deprecate);
	  } /* if */
	  pc_deprecate=NULL;
  } else {
	  cur->flags = 0;
	  cur->documentation = NULL;
  } /* if */

  return cur;
}

SC_FUNC stringpair *find_subst(char *name,int length)
{
  stringpair *item;
  assert(name!=NULL);
  assert(length>0);
  assert((*name>='A' && *name<='Z') || (*name>='a' && *name<='z') || *name=='_' || *name==PUBLIC_CHAR);
  item=substindex[(int)*name-PUBLIC_CHAR];
  if (item!=NULL)
    item=find_stringpair(item,name,length);

  if (item && (item->flags & flgDEPRECATED) != 0)
  {
    static char macro[128];
    char *rem, *msg = (item->documentation != NULL) ? item->documentation : "";
    strlcpy(macro, item->first, sizeof(macro));

    /* If macro contains an opening parentheses and a percent sign, then assume that
     * it takes arguments and remove them from the warning message.
     */
    if ((rem = strchr(macro, '(')) != NULL && strchr(macro, '%') > rem)
    {
      *rem = '\0';
    }

    error(234, macro, msg);  /* deprecated (macro/constant) */
  }
  return item;
}

SC_FUNC int delete_subst(char *name,int length)
{
  stringpair *item;
  assert(name!=NULL);
  assert(length>0);
  assert((*name>='A' && *name<='Z') || (*name>='a' && *name<='z') || *name=='_' || *name==PUBLIC_CHAR);
  item=substindex[(int)*name-PUBLIC_CHAR];
  if (item!=NULL)
    item=find_stringpair(item,name,length);
  if (item==NULL)
    return FALSE;
  if (item->documentation)
  {
    free(item->documentation);
    item->documentation=NULL;
  }
  delete_stringpair(&substpair,item);
  adjustindex(*name);
  return TRUE;
}

SC_FUNC void delete_substtable(void)
{
  int i;
  delete_stringpairtable(&substpair);
  for (i=0; i<sizeof substindex/sizeof substindex[0]; i++)
    substindex[i]=NULL;
}

#endif /* !defined NO_SUBST */


/* ----- input file list ----------------------------------------- */
static stringlist sourcefiles;

SC_FUNC stringlist *insert_sourcefile(char *string)
{
  return insert_string(&sourcefiles,string);
}

SC_FUNC char *get_sourcefile(int index)
{
  return get_string(&sourcefiles,index);
}

SC_FUNC void delete_sourcefiletable(void)
{
  delete_stringtable(&sourcefiles);
  assert(sourcefiles.next==NULL);
}


/* ----- documentation tags -------------------------------------- */
#if !defined SC_LIGHT
static stringlist docstrings;

SC_FUNC stringlist *insert_docstring(char *string)
{
  return insert_string(&docstrings,string);
}

SC_FUNC char *get_docstring(int index)
{
  return get_string(&docstrings,index);
}

SC_FUNC void delete_docstring(int index)
{
  delete_string(&docstrings,index);
}

SC_FUNC void delete_docstringtable(void)
{
  delete_stringtable(&docstrings);
  assert(docstrings.next==NULL);
}
#endif /* !defined SC_LIGHT */


/* ----- autolisting --------------------------------------------- */
static stringlist autolist;

SC_FUNC stringlist *insert_autolist(char *string)
{
  return insert_string(&autolist,string);
}

SC_FUNC char *get_autolist(int index)
{
  return get_string(&autolist,index);
}

SC_FUNC void delete_autolisttable(void)
{
  delete_stringtable(&autolist);
  assert(autolist.next==NULL);
}


/* ----- debug information --------------------------------------- */

/* These macros are adapted from LibDGG libdgg-int64.h, see
 * http://www.dennougedougakkai-ndd.org/pub/libdgg/
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__>=199901L
  #define __STDC_FORMAT_MACROS
  #define __STDC_CONSTANT_MACROS
  #include <inttypes.h>         /* automatically includes stdint.h */
#elif (defined _MSC_VER || defined __BORLANDC__) && (defined _I64_MAX || defined HAVE_I64)
  #define PRId64 "I64d"
  #define PRIx64 "I64x"
#else
  #define PRId64 "lld"
  #define PRIx64 "llx"
#endif
#if PAWN_CELL_SIZE==64
  #define PRIdC  PRId64
  #define PRIxC  PRIx64
#elif PAWN_CELL_SIZE==32
  #define PRIdC  "ld"
  #define PRIxC  "lx"
#else
  #define PRIdC  "d"
  #define PRIxC  "x"
#endif

static stringlist dbgstrings;

SC_FUNC stringlist *insert_dbgfile(const char *filename)
{

  if (sc_status==statWRITE && (sc_debug & sSYMBOLIC)!=0) {
    char string[_MAX_PATH+40];
    assert(filename!=NULL);
    assert(strlen(filename)+40<sizeof string);
    sprintf(string,"F:%" PRIxC " %s",code_idx,filename);
    return insert_string(&dbgstrings,string);
  } /* if */
  return NULL;
}

SC_FUNC stringlist *insert_dbgline(int linenr)
{
  if (sc_status==statWRITE && (sc_debug & sSYMBOLIC)!=0) {
    char string[40];
    if (linenr>0)
      linenr--;         /* line numbers are zero-based in the debug information */
    sprintf(string,"L:%" PRIxC " %x",code_idx,linenr);
    return insert_string(&dbgstrings,string);
  } /* if */
  return NULL;
}

SC_FUNC stringlist *insert_dbgsymbol(symbol *sym)
{
  if (sc_status==statWRITE && (sc_debug & sSYMBOLIC)!=0) {
    char string[2*sNAMEMAX+128];
    char symname[2*sNAMEMAX+16];

    funcdisplayname(symname,sym->name);
    /* address tag:name codestart codeend ident vclass [tag:dim ...] */
    if (sym->ident==iFUNCTN) {
      sprintf(string,"S:%" PRIxC " %x:%s %" PRIxC " %" PRIxC " %x %x",
              sym->addr,sym->tag,symname,sym->addr,sym->codeaddr,sym->ident,sym->vclass);
    } else {
      sprintf(string,"S:%" PRIxC " %x:%s %" PRIxC " %" PRIxC " %x %x",
              sym->addr,sym->tag,symname,sym->codeaddr,code_idx,sym->ident,sym->vclass);
    } /* if */
    if (sym->ident==iARRAY || sym->ident==iREFARRAY) {
      #if !defined NDEBUG
        int count=sym->dim.array.level;
      #endif
      symbol *sub;
      strcat(string," [ ");
      for (sub=sym; sub!=NULL; sub=finddepend(sub)) {
        assert(sub->dim.array.level==count--);
        sprintf(string+strlen(string),"%x:%x ",sub->x.tags.index,sub->dim.array.length);
      } /* for */
      strcat(string,"]");
    } /* if */

    return insert_string(&dbgstrings,string);
  } /* if */
  return NULL;
}

SC_FUNC stringlist *get_dbgstrings()
{
  return &dbgstrings;
}

SC_FUNC char *get_dbgstring(int index)
{
  return get_string(&dbgstrings,index);
}

SC_FUNC void delete_dbgstringtable(void)
{
  delete_stringtable(&dbgstrings);
  assert(dbgstrings.next==NULL);
}
