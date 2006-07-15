/*  Pawn compiler - Staging buffer and optimizer
 *
 *  The staging buffer
 *  ------------------
 *  The staging buffer allows buffered output of generated code, deletion
 *  of redundant code, optimization by a tinkering process and reversing
 *  the ouput of evaluated expressions (which is used for the reversed
 *  evaluation of arguments in functions).
 *  Initially, stgwrite() writes to the file directly, but after a call to
 *  stgset(TRUE), output is redirected to the buffer. After a call to
 *  stgset(FALSE), stgwrite()'s output is directed to the file again. Thus
 *  only one routine is used for writing to the output, which can be
 *  buffered output or direct output.
 *
 *  staging buffer variables:   stgbuf  - the buffer
 *                              stgidx  - current index in the staging buffer
 *                              staging - if true, write to the staging buffer;
 *                                        if false, write to file directly.
 *
 * The peephole optimizer uses a dual "pipeline". The staging buffer (described
 * above) gets optimized for each expression or sub-expression in a function
 * call. The peephole optimizer is recursive, but it does not span multiple
 * sub-expressions. However, the data gets written to a second buffer that
 * behaves much like the staging buffer. This second buffer gathers all
 * optimized strings from the staging buffer for a complete expression. The
 * peephole optmizer then runs over this second buffer to find optimzations
 * across function parameter boundaries.
 *
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
 *  Version: $Id: sc7.c 3579 2006-06-06 13:35:29Z thiadmer $
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>     /* for atoi() */
#include <string.h>
#include <ctype.h>
#if defined FORTIFY
  #include <alloc/fortify.h>
#endif
#include "sc.h"

#if defined _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4125)  /* decimal digit terminates octal escape sequence */
#endif

#include "sc7.scp"

#if defined _MSC_VER
  #pragma warning(pop)
#endif

static int stgstring(char *start,char *end);
static void stgopt(char *start,char *end,int (*outputfunc)(char *str));


#define sSTG_GROW   512
#define sSTG_MAX    20480

static char *stgbuf=NULL;
static int stgmax=0;    /* current size of the staging buffer */

static char *stgpipe=NULL;
static int pipemax=0;   /* current size of the stage pipe, a second staging buffer */
static int pipeidx=0;

#define CHECK_STGBUFFER(index) if ((int)(index)>=stgmax)  grow_stgbuffer(&stgbuf, stgmax, (index)+1)
#define CHECK_STGPIPE(index)   if ((int)(index)>=pipemax) grow_stgbuffer(&stgpipe, pipemax, (index)+1)

static void grow_stgbuffer(char **buffer, int curmax, int requiredsize)
{
  char *p;
  int clear= (*buffer==NULL); /* if previously none, empty buffer explicitly */

  assert(curmax<requiredsize);
  /* if the staging buffer (holding intermediate code for one line) grows
   * over a few kBytes, there is probably a run-away expression
   */
  if (requiredsize>sSTG_MAX)
    error(102,"staging buffer");    /* staging buffer overflow (fatal error) */
  curmax=requiredsize+sSTG_GROW;
  if (*buffer!=NULL)
    p=(char *)realloc(*buffer,curmax*sizeof(char));
  else
    p=(char *)malloc(curmax*sizeof(char));
  if (p==NULL)
    error(102,"staging buffer");    /* staging buffer overflow (fatal error) */
  *buffer=p;
  if (clear)
    **buffer='\0';
}

SC_FUNC void stgbuffer_cleanup(void)
{
  if (stgbuf!=NULL) {
    free(stgbuf);
    stgbuf=NULL;
    stgmax=0;
  } /* if */
  if (stgpipe!=NULL) {
    free(stgpipe);
    stgpipe=NULL;
    pipemax=0;
    pipeidx=0;
  } /* if */
}

/* the variables "stgidx" and "staging" are declared in "scvars.c" */

/*  stgmark
 *
 *  Copies a mark into the staging buffer. At this moment there are three
 *  possible marks:
 *     sSTARTREORDER    identifies the beginning of a series of expression
 *                      strings that must be written to the output file in
 *                      reordered order
 *    sENDREORDER       identifies the end of 'reverse evaluation'
 *    sEXPRSTART + idx  only valid within a block that is evaluated in
 *                      reordered order, it identifies the start of an
 *                      expression; the "idx" value is the argument position
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (altered)
 *                     staging (referred to only)
 */
SC_FUNC void stgmark(char mark)
{
  if (staging) {
    CHECK_STGBUFFER(stgidx);
    stgbuf[stgidx++]=mark;
  } /* if */
}

static int rebuffer(char *str)
{
  if (sc_status==statWRITE) {
    if (pipeidx>=2 && stgpipe[pipeidx-1]=='\0' && stgpipe[pipeidx-2]!='\n')
      pipeidx-=1;                      /* overwrite last '\0' */
    while (*str!='\0') {               /* copy to staging buffer */
      CHECK_STGPIPE(pipeidx);
      stgpipe[pipeidx++]=*str++;
    } /* while */
    CHECK_STGPIPE(pipeidx);
    stgpipe[pipeidx++]='\0';
  } /* if */
  return TRUE;
}

static int filewrite(char *str)
{
  if (sc_status==statWRITE)
    return pc_writeasm(outf,str);
  return TRUE;
}

/*  stgwrite
 *
 *  Writes the string "st" to the staging buffer or to the output file. In the
 *  case of writing to the staging buffer, the terminating byte of zero is
 *  copied too, but... the optimizer can only work on complete lines (not on
 *  fractions of it. Therefore if the string is staged, if the last character
 *  written to the buffer is a '\0' and the previous-to-last is not a '\n',
 *  the string is concatenated to the last string in the buffer (the '\0' is
 *  overwritten). This also means an '\n' used in the middle of a string isn't
 *  recognized and could give wrong results with the optimizer.
 *  Even when writing to the output file directly, all strings are buffered
 *  until a whole line is complete.
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (altered)
 *                     staging (referred to only)
 */
SC_FUNC void stgwrite(const char *st)
{
  int len;

  if (staging) {
    assert(stgidx==0 || stgbuf!=NULL);  /* staging buffer must be valid if there is (apparently) something in it */
    if (stgidx>=2 && stgbuf[stgidx-1]=='\0' && stgbuf[stgidx-2]!='\n')
      stgidx-=1;                       /* overwrite last '\0' */
    while (*st!='\0') {                /* copy to staging buffer */
      CHECK_STGBUFFER(stgidx);
      stgbuf[stgidx++]=*st++;
    } /* while */
    CHECK_STGBUFFER(stgidx);
    stgbuf[stgidx++]='\0';
  } else {
    len=(stgbuf!=NULL) ? strlen(stgbuf) : 0;
    CHECK_STGBUFFER(len+strlen(st)+1);
    strcat(stgbuf,st);
    len=strlen(stgbuf);
    if (len>0 && stgbuf[len-1]=='\n') {
      filewrite(stgbuf);
      stgbuf[0]='\0';
    } /* if */
  } /* if */
}

/*  stgout
 *
 *  Writes the staging buffer to the output file via stgstring() (for
 *  reversing expressions in the buffer) and stgopt() (for optimizing). It
 *  resets "stgidx".
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (referred to only)
 *                     staging (referred to only)
 */
SC_FUNC void stgout(int index)
{
  int reordered=0;
  int idx;

  if (!staging)
    return;
  assert(pipeidx==0);

  /* first pass: sub-expressions */
  if (sc_status==statWRITE)
    reordered=stgstring(&stgbuf[index],&stgbuf[stgidx]);
  stgidx=index;

  /* second pass: optimize the buffer created in the first pass */
  if (sc_status==statWRITE) {
    if (reordered) {
      stgopt(stgpipe,stgpipe+pipeidx,filewrite);
    } else {
      /* there is no sense in re-optimizing if the order of the sub-expressions
       * did not change; so output directly
       */
      for (idx=0; idx<pipeidx; idx+=strlen(stgpipe+idx)+1)
        filewrite(stgpipe+idx);
    } /* if */
  } /* if */
  pipeidx=0;  /* reset second pipe */
}

typedef struct {
  char *start,*end;
} argstack;

/*  stgstring
 *
 *  Analyses whether code strings should be output to the file as they appear
 *  in the staging buffer or whether portions of it should be re-ordered.
 *  Re-ordering takes place in function argument lists; Pawn passes arguments
 *  to functions from right to left. When arguments are "named" rather than
 *  positional, the order in the source stream is indeterminate.
 *  This function calls itself recursively in case it needs to re-order code
 *  strings, and it uses a private stack (or list) to mark the start and the
 *  end of expressions in their correct (reversed) order.
 *  In any case, stgstring() sends a block as large as possible to the
 *  optimizer stgopt().
 *
 *  In "reorder" mode, each set of code strings must start with the token
 *  sEXPRSTART, even the first. If the token sSTARTREORDER is represented
 *  by '[', sENDREORDER by ']' and sEXPRSTART by '|' the following applies:
 *     '[]...'     valid, but useless; no output
 *     '[|...]     valid, but useless; only one string
 *     '[|...|...] valid and usefull
 *     '[...|...]  invalid, first string doesn't start with '|'
 *     '[|...|]    invalid
 */
static int stgstring(char *start,char *end)
{
  char *ptr;
  int nest,argc,arg;
  argstack *stack;
  int reordered=0;

  while (start<end) {
    if (*start==sSTARTREORDER) {
      start+=1;         /* skip token */
      /* allocate a argstack with sMAXARGS items */
      stack=(argstack *)malloc(sMAXARGS*sizeof(argstack));
      if (stack==NULL)
        error(103);     /* insufficient memory */
      reordered=1;      /* mark that the expression is reordered */
      nest=1;           /* nesting counter */
      argc=0;           /* argument counter */
      arg=-1;           /* argument index; no valid argument yet */
      do {
        switch (*start) {
        case sSTARTREORDER:
          nest++;
          start++;
          break;
        case sENDREORDER:
          nest--;
          start++;
          break;
        default:
          if ((*start & sEXPRSTART)==sEXPRSTART) {
            if (nest==1) {
              if (arg>=0)
                stack[arg].end=start-1; /* finish previous argument */
              arg=(unsigned char)*start - sEXPRSTART;
              stack[arg].start=start+1;
              if (arg>=argc)
                argc=arg+1;
            } /* if */
            start++;
          } else {
            start+=strlen(start)+1;
          } /* if */
        } /* switch */
      } while (nest); /* enddo */
      if (arg>=0)
        stack[arg].end=start-1;   /* finish previous argument */
      while (argc>0) {
        argc--;
        stgstring(stack[argc].start,stack[argc].end);
      } /* while */
      free(stack);
    } else {
      ptr=start;
      while (ptr<end && *ptr!=sSTARTREORDER)
        ptr+=strlen(ptr)+1;
      stgopt(start,ptr,rebuffer);
      start=ptr;
    } /* if */
  } /* while */
  return reordered;
}

/*  stgdel
 *
 *  Scraps code from the staging buffer by resetting "stgidx" to "index".
 *
 *  Global references: stgidx (altered)
 *                     staging (reffered to only)
 */
SC_FUNC void stgdel(int index,cell code_index)
{
  if (staging) {
    stgidx=index;
    code_idx=code_index;
  } /* if */
}

SC_FUNC int stgget(int *index,cell *code_index)
{
  if (staging) {
    *index=stgidx;
    *code_index=code_idx;
  } /* if */
  return staging;
}

/*  stgset
 *
 *  Sets staging on or off. If it's turned off, the staging buffer must be
 *  initialized to an empty string. If it's turned on, the routine makes sure
 *  the index ("stgidx") is set to 0 (it should already be 0).
 *
 *  Global references: staging  (altered)
 *                     stgidx   (altered)
 *                     stgbuf   (contents altered)
 */
SC_FUNC void stgset(int onoff)
{
  staging=onoff;
  if (staging){
    assert(stgidx==0);
    stgidx=0;
    CHECK_STGBUFFER(stgidx);
    /* write any contents that may be put in the buffer by stgwrite()
     * when "staging" was 0
     */
    if (strlen(stgbuf)>0)
      filewrite(stgbuf);
  } /* if */
  stgbuf[0]='\0';
}

/* phopt_init
 * Initialize all sequence strings of the peehole optimizer. The strings
 * are embedded in the .EXE file in compressed format, here we expand
 * them (and allocate memory for the sequences).
 */
static SEQUENCE *sequences;

SC_FUNC int phopt_init(void)
{
  int number, i, len;
  char str[160];

  /* count number of sequences */
  for (number=0; sequences_cmp[number].find!=NULL; number++)
    /* nothing */;
  number++;             /* include an item for the NULL terminator */

  if ((sequences=(SEQUENCE*)malloc(number * sizeof(SEQUENCE)))==NULL)
    return FALSE;

  /* pre-initialize all to NULL (in case of failure) */
  for (i=0; i<number; i++) {
    sequences[i].find=NULL;
    sequences[i].replace=NULL;
    sequences[i].savesize=0;
  } /* for */

  /* expand all strings */
  for (i=0; i<number-1; i++) {
    len = strexpand(str,(unsigned char*)sequences_cmp[i].find,sizeof str,SCPACK_TABLE);
    assert(len<=sizeof str);
    assert(len==(int)strlen(str)+1);
    sequences[i].find=(char*)malloc(len);
    if (sequences[i].find!=NULL)
      strcpy(sequences[i].find,str);
    len = strexpand(str,(unsigned char*)sequences_cmp[i].replace,sizeof str,SCPACK_TABLE);
    assert(len<=sizeof str);
    assert(len==(int)strlen(str)+1);
    sequences[i].replace=(char*)malloc(len);
    if (sequences[i].replace!=NULL)
      strcpy(sequences[i].replace,str);
    sequences[i].savesize=sequences_cmp[i].savesize;
    if (sequences[i].find==NULL || sequences[i].replace==NULL)
      return phopt_cleanup();
  } /* for */

  return TRUE;
}

SC_FUNC int phopt_cleanup(void)
{
  int i;
  if (sequences!=NULL) {
    i=0;
    while (sequences[i].find!=NULL || sequences[i].replace!=NULL) {
      if (sequences[i].find!=NULL)
        free(sequences[i].find);
      if (sequences[i].replace!=NULL)
        free(sequences[i].replace);
      i++;
    } /* while */
    free(sequences);
    sequences=NULL;
  } /* if */
  return FALSE;
}

#define MAX_OPT_VARS    5
#define MAX_OPT_CAT     5       /* max. values that are concatenated */
#if sNAMEMAX > (PAWN_CELL_SIZE/4) * MAX_OPT_CAT
  #define MAX_ALIAS       sNAMEMAX
#else
  #define MAX_ALIAS       (PAWN_CELL_SIZE/4) * MAX_OPT_CAT
#endif

static int matchsequence(char *start,char *end,char *pattern,
                         char symbols[MAX_OPT_VARS][MAX_ALIAS+1],
                         int *match_length)
{
  int var,i;
  char str[MAX_ALIAS+1];
  char *start_org=start;
  cell value;
  char *ptr;

  *match_length=0;
  for (var=0; var<MAX_OPT_VARS; var++)
    symbols[var][0]='\0';

  while (*start=='\t' || *start==' ')
    start++;
  while (*pattern) {
    if (start>=end)
      return FALSE;
    switch (*pattern) {
    case '%':   /* new "symbol" */
      pattern++;
      assert(isdigit(*pattern));
      var=atoi(pattern) - 1;
      assert(var>=0 && var<MAX_OPT_VARS);
      assert(*start=='-' || alphanum(*start));
      for (i=0; start<end && (*start=='-' || *start=='+' || alphanum(*start)); i++,start++) {
        assert(i<=MAX_ALIAS);
        str[i]=*start;
      } /* for */
      str[i]='\0';
      if (symbols[var][0]!='\0') {
        if (strcmp(symbols[var],str)!=0)
          return FALSE; /* symbols should be identical */
      } else {
        strcpy(symbols[var],str);
      } /* if */
      break;
    case '-':
      value=-strtol(pattern+1,&pattern,16);
      ptr=itoh((ucell)value);
      while (*ptr!='\0') {
        if (tolower(*start) != tolower(*ptr))
          return FALSE;
        start++;
        ptr++;
      } /* while */
      pattern--;  /* there is an increment following at the end of the loop */
      break;
    case ' ':
      if (*start!='\t' && *start!=' ')
        return FALSE;
      while (start<end && (*start=='\t' || *start==' '))
        start++;
      break;
    case '!':
      while (start<end && (*start=='\t' || *start==' '))
        start++;                /* skip trailing white space */
      if (*start==';')
        while (start<end && *start!='\n')
          start++;              /* skip trailing comment */
      if (*start!='\n')
        return FALSE;
      assert(*(start+1)=='\0');
      start+=2;                 /* skip '\n' and '\0' */
      if (*(pattern+1)!='\0')
        while (start<end && *start=='\t' || *start==' ')
          start++;              /* skip leading white space of next instruction */
      break;
    default:
      if (tolower(*start) != tolower(*pattern))
        return FALSE;
      start++;
    } /* switch */
    pattern++;
  } /* while */

  *match_length=(int)(start-start_org);
  return TRUE;
}

static char *replacesequence(char *pattern,char symbols[MAX_OPT_VARS][MAX_ALIAS+1],int *repl_length)
{
  char *lptr;
  int var;
  char *buffer;

  /* calculate the length of the new buffer
   * this is the length of the pattern plus the length of all symbols (note
   * that the same symbol may occur multiple times in the pattern) plus
   * line endings and startings ('\t' to start a line and '\n\0' to end one)
   */
  assert(repl_length!=NULL);
  *repl_length=0;
  lptr=pattern;
  while (*lptr) {
    switch (*lptr) {
    case '%':
      lptr++;           /* skip '%' */
      assert(isdigit(*lptr));
      var=atoi(lptr) - 1;
      assert(var>=0 && var<MAX_OPT_VARS);
      assert(symbols[var][0]!='\0');    /* variable should be defined */
      *repl_length+=strlen(symbols[var]);
      break;
    case '!':
      *repl_length+=3;  /* '\t', '\n' & '\0' */
      break;
    default:
      *repl_length+=1;
    } /* switch */
    lptr++;
  } /* while */

  /* allocate a buffer to replace the sequence in */
  if ((buffer=(char*)malloc(*repl_length))==NULL)
    return (char*)error(103);

  /* replace the pattern into this temporary buffer */
  lptr=buffer;
  *lptr++='\t';         /* the "replace" patterns do not have tabs */
  while (*pattern) {
    assert((int)(lptr-buffer)<*repl_length);
    switch (*pattern) {
    case '%':
      /* write out the symbol */
      pattern++;
      assert(isdigit(*pattern));
      var=atoi(pattern) - 1;
      assert(var>=0 && var<MAX_OPT_VARS);
      assert(symbols[var][0]!='\0');    /* variable should be defined */
      strcpy(lptr,symbols[var]);
      lptr+=strlen(symbols[var]);
      break;
    case '!':
      /* finish the line, optionally start the next line with an indent */
      *lptr++='\n';
      *lptr++='\0';
      if (*(pattern+1)!='\0')
        *lptr++='\t';
      break;
    default:
      *lptr++=*pattern;
    } /* switch */
    pattern++;
  } /* while */

  assert((int)(lptr-buffer)==*repl_length);
  return buffer;
}

static void strreplace(char *dest,char *replace,int sub_length,int repl_length,int dest_length)
{
  int offset=sub_length-repl_length;
  if (offset>0) {               /* delete a section */
    memmove(dest,dest+offset,dest_length-offset);
    memset(dest+dest_length-offset,0xcc,offset); /* not needed, but for cleanlyness */
  } else if (offset<0) {        /* insert a section */
    memmove(dest-offset, dest, dest_length);
  } /* if */
  memcpy(dest, replace, repl_length);
}

/*  stgopt
 *
 *  Optimizes the staging buffer by checking for series of instructions that
 *  can be coded more compact. The routine expects the lines in the staging
 *  buffer to be separated with '\n' and '\0' characters.
 *
 *  The longest sequences should probably be checked first.
 */

static void stgopt(char *start,char *end,int (*outputfunc)(char *str))
{
  char symbols[MAX_OPT_VARS][MAX_ALIAS+1];
  int seq,match_length,repl_length;
  int matches;
  char *debut=start;  /* save original start of the buffer */

  assert(sequences!=NULL);
  /* do not match anything if debug-level is maximum */
  if (pc_optimize>sOPTIMIZE_NONE && sc_status==statWRITE) {
    do {
      matches=0;
      start=debut;
      while (start<end) {
        seq=0;
        while (sequences[seq].find!=NULL) {
          assert(seq>=0);
          if (*sequences[seq].find=='\0') {
            if (pc_optimize==sOPTIMIZE_NOMACRO) {
              break;    /* don't look further */
            } else {
              seq++;    /* continue with next string */
              continue;
            } /* if */
          } /* if */
          if (matchsequence(start,end,sequences[seq].find,symbols,&match_length)) {
            char *replace=replacesequence(sequences[seq].replace,symbols,&repl_length);
            /* If the replacement is bigger than the original section, we may need
             * to "grow" the staging buffer. This is quite complex, due to the
             * re-ordering of expressions that can also happen in the staging
             * buffer. In addition, it should not happen: the peephole optimizer
             * must replace sequences with *shorter* sequences, not longer ones.
             * So, I simply forbid sequences that are longer than the ones they
             * are meant to replace.
             */
            assert(match_length>=repl_length);
            if (match_length>=repl_length) {
              strreplace(start,replace,match_length,repl_length,(int)(end-start));
              end-=match_length-repl_length;
              free(replace);
              code_idx-=sequences[seq].savesize;
              seq=0;                      /* restart search for matches */
              matches++;
            } else {
              /* actually, we should never get here (match_length<repl_length) */
              assert(0);
              seq++;
            } /* if */
          } else {
            seq++;
          } /* if */
        } /* while */
        assert(sequences[seq].find==NULL || *sequences[seq].find=='\0' && pc_optimize==sOPTIMIZE_NOMACRO);
        start += strlen(start) + 1;       /* to next string */
      } /* while (start<end) */
    } while (matches>0);
  } /* if (pc_optimize>sOPTIMIZE_NONE && sc_status==statWRITE) */

  for (start=debut; start<end; start+=strlen(start)+1)
    outputfunc(start);
}

#undef SCPACK_TABLE
