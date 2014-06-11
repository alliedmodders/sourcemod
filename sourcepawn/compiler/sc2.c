/*  Pawn compiler - File input, preprocessing and lexical analysis functions
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "lstring.h"
#include "sc.h"
#if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
  #include <sclinux.h>
#endif
#include "sp_symhash.h"

#if defined FORTIFY
  #include <alloc/fortify.h>
#endif

/* flags for litchar() */
#define RAWMODE         0x1
#define UTF8MODE        0x2
#define ISPACKED        0x4
static cell litchar(const unsigned char **lptr,int flags);
static symbol *find_symbol(const symbol *root,const char *name,int fnumber,int automaton,int *cmptag);

static void substallpatterns(unsigned char *line,int buffersize);
static int match(char *st,int end);
static int alpha(char c);

#define SKIPMODE      1 /* bit field in "#if" stack */
#define PARSEMODE     2 /* bit field in "#if" stack */
#define HANDLED_ELSE  4 /* bit field in "#if" stack */
#define SKIPPING      (skiplevel>0 && (ifstack[skiplevel-1] & SKIPMODE)==SKIPMODE)

static short icomment;  /* currently in multiline comment? */
static char ifstack[sCOMP_STACK]; /* "#if" stack */
static short iflevel;   /* nesting level if #if/#else/#endif */
static short skiplevel; /* level at which we started skipping (including nested #if .. #endif) */
static unsigned char term_expr[] = "";
static int listline=-1; /* "current line" for the list file */

#if defined DARWIN
static double pow10(double d)
{
  return pow(10, d);
}
#endif


/*  pushstk & popstk
 *
 *  Uses a LIFO stack to store information. The stack is used by doinclude(),
 *  doswitch() (to hold the state of "swactive") and some other routines.
 *
 *  Porting note: I made the bold assumption that an integer will not be
 *  larger than a pointer (it may be smaller). That is, the stack element
 *  is typedef'ed as a pointer type, but I also store integers on it. See
 *  SC.H for "stkitem"
 *
 *  Global references: stack,stkidx,stktop (private to pushstk(), popstk()
 *                     and clearstk())
 */
static stkitem *stack=NULL;
static int stkidx=0,stktop=0;

SC_FUNC void pushstk(stkitem val)
{
  assert(stkidx<=stktop);
  if (stkidx==stktop) {
    stkitem *newstack;
    int newsize= (stktop==0) ? 16 : 2*stktop;
    /* try to resize the stack */
    assert(newsize>stktop);
    newstack=(stkitem*)malloc(newsize*sizeof(stkitem));
    if (newstack==NULL)
      error(122,"parser stack");  /* stack overflow (recursive include?) */
    /* swap the stacks */
    memcpy(newstack,stack,stkidx*sizeof(stkitem));
    if (stack!=NULL)
      free(stack);
    stack=newstack;
    stktop=newsize;
  } /* if */
  assert(stkidx<stktop);
  stack[stkidx]=val;
  stkidx+=1;
}

SC_FUNC stkitem popstk(void)
{
  if (stkidx==0) {
    stkitem s;
    s.i=-1;             /* stack is empty */
    return s;
  } /* if */
  stkidx--;
  assert(stack!=NULL);
  return stack[stkidx];
}

SC_FUNC void clearstk(void)
{
  assert(stack!=NULL || stktop==0);
  if (stack!=NULL) {
    free(stack);
    stack=NULL;
    stktop=0;
  } /* if */
  assert(stktop==0);
}

SC_FUNC int plungequalifiedfile(char *name)
{
static char *extensions[] = { ".inc", ".p", ".pawn" };
  FILE *fp;
  char *ext;
  int ext_idx;

  ext_idx=0;
  do {
    fp=(FILE*)pc_opensrc(name);
    ext=strchr(name,'\0');      /* save position */
    if (fp==NULL) {
      /* try to append an extension */
      strcpy(ext,extensions[ext_idx]);
      fp=(FILE*)pc_opensrc(name);
      if (fp==NULL)
        *ext='\0';              /* on failure, restore filename */
    } /* if */
    ext_idx++;
  } while (fp==NULL && ext_idx<(sizeof extensions / sizeof extensions[0]));
  if (fp==NULL) {
    *ext='\0';                  /* restore filename */
    return FALSE;
  } /* if */
  if (sc_showincludes && sc_status==statFIRST) {
    fprintf(stdout, "Note: including file: %s\n", name);
  }
  PUSHSTK_P(inpf);
  PUSHSTK_P(inpfname);          /* pointer to current file name */
  PUSHSTK_P(curlibrary);
  PUSHSTK_I(iflevel);
  assert(!SKIPPING);
  assert(skiplevel==iflevel);   /* these two are always the same when "parsing" */
  PUSHSTK_I(sc_is_utf8);
  PUSHSTK_I(icomment);
  PUSHSTK_I(fcurrent);
  PUSHSTK_I(fline);
  inpfname=duplicatestring(name);/* set name of include file */
  if (inpfname==NULL)
    error(123);             /* insufficient memory */
  inpf=fp;                  /* set input file pointer to include file */
  fnumber++;
  fline=0;                  /* set current line number to 0 */
  fcurrent=fnumber;
  icomment=0;               /* not in a comment */
  insert_dbgfile(inpfname);
  setfiledirect(inpfname);
  listline=-1;              /* force a #line directive when changing the file */
  sc_is_utf8=(short)scan_utf8(inpf,name);
  return TRUE;
}

SC_FUNC int plungefile(char *name,int try_currentpath,int try_includepaths)
{
  int result=FALSE;

  if (try_currentpath) {
    result=plungequalifiedfile(name);
    if (!result) {
      /* failed to open the file in the active directory, try to open the file
       * in the same directory as the current file --but first check whether
       * there is a (relative) path for the current file
       */
      char *ptr;
      if ((ptr=strrchr(inpfname,DIRSEP_CHAR))!=0) {
        int len=(int)(ptr-inpfname)+1;
        if (len+strlen(name)<_MAX_PATH) {
          char path[_MAX_PATH];
          strlcpy(path,inpfname,len+1);
          strlcat(path,name,sizeof path);
          result=plungequalifiedfile(path);
        } /* if */
      } /* if */
    } /* if */
  } /* if */

  if (try_includepaths && name[0]!=DIRSEP_CHAR) {
    int i;
    char *ptr;
    for (i=0; !result && (ptr=get_path(i))!=NULL; i++) {
      char path[_MAX_PATH];
      strlcpy(path,ptr,sizeof path);
      strlcat(path,name,sizeof path);
      result=plungequalifiedfile(path);
    } /* while */
  } /* if */
  return result;
}

static void check_empty(const unsigned char *lptr)
{
  /* verifies that the string contains only whitespace */
  while (*lptr<=' ' && *lptr!='\0')
    lptr++;
  if (*lptr!='\0')
    error(38);          /* extra characters on line */
}

/*  doinclude
 *
 *  Gets the name of an include file, pushes the old file on the stack and
 *  sets some options. This routine doesn't use lex(), since lex() doesn't
 *  recognize file names (and directories).
 *
 *  Global references: inpf     (altered)
 *                     inpfname (altered)
 *                     fline    (altered)
 *                     lptr     (altered)
 */
static void doinclude(int silent)
{
  char name[_MAX_PATH];
  char symname[sNAMEMAX+1];
  char *ptr;
  char c;
  int i, result;

  while (*lptr<=' ' && *lptr!='\0')         /* skip leading whitespace */
    lptr++;
  if (*lptr=='<' || *lptr=='\"'){
    c=(char)((*lptr=='\"') ? '\"' : '>');   /* termination character */
    lptr++;
    while (*lptr<=' ' && *lptr!='\0')       /* skip whitespace after quote */
      lptr++;
  } else {
    c='\0';
  } /* if */

  i=0;
  while (*lptr!=c && *lptr!='\0' && i<sizeof name - 1)  /* find the end of the string */
    name[i++]=*lptr++;
  while (i>0 && name[i-1]<=' ')
    i--;                        /* strip trailing whitespace */
  assert(i>=0 && i<sizeof name);
  name[i]='\0';                 /* zero-terminate the string */

  if (*lptr!=c) {               /* verify correct string termination */
    error(37);                  /* invalid string */
    return;
  } /* if */
  if (c!='\0')
    check_empty(lptr+1);        /* verify that the rest of the line is whitespace */

  result=plungefile(name,(c!='>'),TRUE);
  if (!result && !silent)
    error(120,name);            /* cannot read from ... (fatal error) */
}

/*  readline
 *
 *  Reads in a new line from the input file pointed to by "inpf". readline()
 *  concatenates lines that end with a \ with the next line. If no more data
 *  can be read from the file, readline() attempts to pop off the previous file
 *  from the stack. If that fails too, it sets "freading" to 0.
 *
 *  Global references: inpf,fline,inpfname,freading,icomment (altered)
 */
static void readline(unsigned char *line)
{
  int i,num,cont;
  unsigned char *ptr;

  if (lptr==term_expr)
    return;
  num=sLINEMAX;
  cont=FALSE;
  do {
    if (inpf==NULL || pc_eofsrc(inpf)) {
      if (cont)
        error(49);        /* invalid line continuation */
      if (inpf!=NULL && inpf!=inpf_org)
        pc_closesrc(inpf);
      i=POPSTK_I();
      if (i==-1) {        /* All's done; popstk() returns "stack is empty" */
        freading=FALSE;
        *line='\0';
        /* when there is nothing more to read, the #if/#else stack should
         * be empty and we should not be in a comment
         */
        assert(iflevel>=0);
        if (iflevel>0)
          error(1,"#endif","-end of file-");
        else if (icomment!=0)
          error(1,"*/","-end of file-");
        return;
      } /* if */
      fline=i;
      fcurrent=(short)POPSTK_I();
      icomment=(short)POPSTK_I();
      sc_is_utf8=(short)POPSTK_I();
      iflevel=(short)POPSTK_I();
      skiplevel=iflevel;        /* this condition held before including the file */
      assert(!SKIPPING);        /* idem ditto */
      curlibrary=(constvalue *)POPSTK_P();
      free(inpfname);           /* return memory allocated for the include file name */
      inpfname=(char *)POPSTK_P();
      inpf=(FILE *)POPSTK_P();
      insert_dbgfile(inpfname);
      setfiledirect(inpfname);
      listline=-1;              /* force a #line directive when changing the file */
    } /* if */

    if (pc_readsrc(inpf,line,num)==NULL) {
      *line='\0';     /* delete line */
      cont=FALSE;
    } else {
      /* check whether to erase leading spaces */
      if (cont) {
        unsigned char *ptr=line;
        while (*ptr<=' ' && *ptr!='\0')
          ptr++;
        if (ptr!=line)
          memmove(line,ptr,strlen((char*)ptr)+1);
      } /* if */
      cont=FALSE;
      /* check whether a full line was read */
      if (strchr((char*)line,'\n')==NULL && !pc_eofsrc(inpf))
        error(75);      /* line too long */
      /* check if the next line must be concatenated to this line */
      if ((ptr=(unsigned char*)strchr((char*)line,'\n'))==NULL)
        ptr=(unsigned char*)strchr((char*)line,'\r');
      if (ptr!=NULL && ptr>line) {
        assert(*(ptr+1)=='\0'); /* '\n' or '\r' should be last in the string */
        while (ptr>line && *ptr<=' ')
          ptr--;        /* skip trailing whitespace */
        if (*ptr=='\\') {
          cont=TRUE;
          /* set '\a' at the position of '\\' to make it possible to check
           * for a line continuation in a single line comment (error 49)
           */
          *ptr++='\a';
          *ptr='\0';    /* erase '\n' (and any trailing whitespace) */
        } /* if */
      } /* if */
      num-=strlen((char*)line);
      line+=strlen((char*)line);
    } /* if */
    fline+=1;
  } while (num>=0 && cont);
}

/*  stripcom
 *
 *  Replaces all comments from the line by space characters. It updates
 *  a global variable ("icomment") for multiline comments.
 *
 *  This routine also supports the C++ extension for single line comments.
 *  These comments are started with "//" and end at the end of the line.
 *
 *  The function also detects (and manages) "documentation comments". The
 *  global variable "icomment" is set to 2 for documentation comments.
 *
 *  Global references: icomment  (private to "stripcom")
 */
static void stripcom(unsigned char *line)
{
  char c;
  #if !defined SC_LIGHT
    #define COMMENT_LIMIT 100
    #define COMMENT_MARGIN 40   /* length of the longest word */
    char comment[COMMENT_LIMIT+COMMENT_MARGIN];
    int commentidx=0;
    int skipstar=TRUE;
    static int prev_singleline=FALSE;
    int singleline=prev_singleline;

    prev_singleline=FALSE;  /* preset */
  #endif

  while (*line){
    if (icomment!=0) {
      if (*line=='*' && *(line+1)=='/') {
        #if !defined SC_LIGHT
          if (icomment==2) {
            assert(commentidx<COMMENT_LIMIT+COMMENT_MARGIN);
            comment[commentidx]='\0';
            if (strlen(comment)>0)
              insert_docstring(comment);
          } /* if */
        #endif
        icomment=0;     /* comment has ended */
        *line=' ';      /* replace '*' and '/' characters by spaces */
        *(line+1)=' ';
        line+=2;
      } else {
        if (*line=='/' && *(line+1)=='*')
          error(216);   /* nested comment */
        #if !defined SC_LIGHT
          /* collect the comment characters in a string */
          if (icomment==2) {
            if (skipstar && ((*line!='\0' && *line<=' ') || *line=='*')) {
              /* ignore leading whitespace and '*' characters */
            } else if (commentidx<COMMENT_LIMIT+COMMENT_MARGIN-1) {
              comment[commentidx++]=(char)((*line!='\n') ? *line : ' ');
              if (commentidx>COMMENT_LIMIT && *line!='\0' && *line<=' ') {
                comment[commentidx]='\0';
                insert_docstring(comment);
                commentidx=0;
              } /* if */
              skipstar=FALSE;
            } /* if */
          } /* if */
        #endif
        *line=' ';      /* replace comments by spaces */
        line+=1;
      } /* if */
    } else {
      if (*line=='/' && *(line+1)=='*'){
        icomment=1;     /* start comment */
        #if !defined SC_LIGHT
          /* there must be two "*" behind the slash and then white space */
          if (*(line+2)=='*' && *(line+3)<=' ') {
            /* if we are not in a function, we must attach the previous block
             * to the global documentation
             */
            if (curfunc==NULL && get_docstring(0)!=NULL)
              sc_attachdocumentation(NULL);
            icomment=2; /* documentation comment */
          } /* if */
          commentidx=0;
          skipstar=TRUE;
        #endif
        *line=' ';      /* replace '/' and '*' characters by spaces */
        *(line+1)=' ';
        line+=2;
        if (icomment==2)
          *line++=' ';
      } else if (*line=='/' && *(line+1)=='/'){  /* comment to end of line */
        if (strchr((char*)line,'\a')!=NULL)
          error(49);    /* invalid line continuation */
        #if !defined SC_LIGHT
          if (*(line+2)=='/' && *(line+3)<=' ') {
            /* documentation comment */
            char *str=(char*)line+3;
            char *end;
            while (*str<=' ' && *str!='\0')
              str++;    /* skip leading whitespace */
            if ((end=strrchr(str,'\n'))!=NULL)
              *end='\0';/* erase trailing '\n' */
            /* if there is a disjunct block, we may need to attach the previous
             * block to the global documentation
             */
            if (!singleline && curfunc==NULL && get_docstring(0)!=NULL)
              sc_attachdocumentation(NULL);
            insert_docstring(str);
            prev_singleline=TRUE;
          } /* if */
        #endif
        *line++='\n';   /* put "newline" at first slash */
        *line='\0';     /* put "zero-terminator" at second slash */
      } else {
        if (*line=='\"' || *line=='\''){        /* leave literals unaltered */
          c=*line;      /* ending quote, single or double */
          line+=1;
          while ((*line!=c || *(line-1)==sc_ctrlchar) && *line!='\0')
            line+=1;
          line+=1;      /* skip final quote */
        } else {
          line+=1;
        } /* if */
      } /* if */
    } /* if */
  } /* while */
  #if !defined SC_LIGHT
    if (icomment==2) {
      assert(commentidx<COMMENT_LIMIT+COMMENT_MARGIN);
      comment[commentidx]='\0';
      if (strlen(comment)>0)
        insert_docstring(comment);
    } /* if */
  #endif
}

/*  btoi
 *
 *  Attempts to interpret a numeric symbol as a boolean value. On success
 *  it returns the number of characters processed (so the line pointer can be
 *  adjusted) and the value is stored in "val". Otherwise it returns 0 and
 *  "val" is garbage.
 *
 *  A boolean value must start with "0b"
 */
static int btoi(cell *val,const unsigned char *curptr)
{
  const unsigned char *ptr;

  *val=0;
  ptr=curptr;
  if (*ptr=='0' && *(ptr+1)=='b') {
    ptr+=2;
    while (*ptr=='0' || *ptr=='1' || *ptr=='_') {
      if (*ptr!='_')
        *val=(*val<<1) | (*ptr-'0');
      ptr++;
    } /* while */
  } else {
    return 0;
  } /* if */
  if (alphanum(*ptr))   /* number must be delimited by non-alphanumeric char */
    return 0;
  else
    return (int)(ptr-curptr);
}

/*  dtoi
 *
 *  Attempts to interpret a numeric symbol as a decimal value. On success
 *  it returns the number of characters processed and the value is stored in
 *  "val". Otherwise it returns 0 and "val" is garbage.
 */
static int dtoi(cell *val,const unsigned char *curptr)
{
  const unsigned char *ptr;

  *val=0;
  ptr=curptr;
  if (!isdigit(*ptr))   /* should start with digit */
    return 0;
  while (isdigit(*ptr) || *ptr=='_') {
    if (*ptr!='_')
      *val=(*val*10)+(*ptr-'0');
    ptr++;
  } /* while */
  if (alphanum(*ptr))   /* number must be delimited by non-alphanumerical */
    return 0;
  if (*ptr=='.' && isdigit(*(ptr+1)))
    return 0;           /* but a fractional part must not be present */
  return (int)(ptr-curptr);
}

/*  htoi
 *
 *  Attempts to interpret a numeric symbol as a hexadecimal value. On
 *  success it returns the number of characters processed and the value is
 *  stored in "val". Otherwise it return 0 and "val" is garbage.
 */
static int htoi(cell *val,const unsigned char *curptr)
{
  const unsigned char *ptr;

  *val=0;
  ptr=curptr;
  if (!isdigit(*ptr))   /* should start with digit */
    return 0;
  if (*ptr=='0' && *(ptr+1)=='x') {     /* C style hexadecimal notation */
    ptr+=2;
    while (ishex(*ptr) || *ptr=='_') {
      if (*ptr!='_') {
        assert(ishex(*ptr));
        *val= *val<<4;
        if (isdigit(*ptr))
          *val+= (*ptr-'0');
        else
          *val+= (tolower(*ptr)-'a'+10);
      } /* if */
      ptr++;
    } /* while */
  } else {
    return 0;
  } /* if */
  if (alphanum(*ptr))
    return 0;
  else
    return (int)(ptr-curptr);
}

/*  ftoi
 *
 *  Attempts to interpret a numeric symbol as a rational number, either as
 *  IEEE 754 single/double precision floating point or as a fixed point integer.
 *  On success it returns the number of characters processed and the value is
 *  stored in "val". Otherwise it returns 0 and "val" is unchanged.
 *
 *  Pawn has stricter definition for rational numbers than most:
 *  o  the value must start with a digit; ".5" is not a valid number, you
 *     should write "0.5"
 *  o  a period must appear in the value, even if an exponent is given; "2e3"
 *     is not a valid number, you should write "2.0e3"
 *  o  at least one digit must follow the period; "6." is not a valid number,
 *     you should write "6.0"
 */
static int ftoi(cell *val,const unsigned char *curptr)
{
  const unsigned char *ptr;
  double fnum,ffrac,fmult;
  unsigned long dnum,dbase;
  int i, ignore;

  assert(rational_digits>=0 && rational_digits<9);
  for (i=0,dbase=1; i<rational_digits; i++)
    dbase*=10;
  fnum=0.0;
  dnum=0L;
  ptr=curptr;
  if (!isdigit(*ptr))   /* should start with digit */
    return 0;
  while (isdigit(*ptr) || *ptr=='_') {
    if (*ptr!='_') {
      fnum=(fnum*10.0)+(*ptr-'0');
      dnum=(dnum*10L)+(*ptr-'0')*dbase;
    } /* if */
    ptr++;
  } /* while */
  if (*ptr!='.')
    return 0;           /* there must be a period */
  ptr++;
  if (!isdigit(*ptr))   /* there must be at least one digit after the dot */
    return 0;
  ffrac=0.0;
  fmult=1.0;
  ignore=FALSE;
  while (isdigit(*ptr) || *ptr=='_') {
    if (*ptr!='_') {
      ffrac=(ffrac*10.0)+(*ptr-'0');
      fmult=fmult/10.0;
      dbase /= 10L;
      dnum += (*ptr-'0')*dbase;
      if (dbase==0L && sc_rationaltag && rational_digits>0 && !ignore) {
        error(222);     /* number of digits exceeds rational number precision */
        ignore=TRUE;
      } /* if */
    } /* if */
    ptr++;
  } /* while */
  fnum += ffrac*fmult;  /* form the number so far */
  if (*ptr=='e') {      /* optional fractional part */
    int exp,sign;
    ptr++;
    if (*ptr=='-') {
      sign=-1;
      ptr++;
    } else {
      sign=1;
    } /* if */
    if (!isdigit(*ptr)) /* 'e' should be followed by a digit */
      return 0;
    exp=0;
    while (isdigit(*ptr)) {
      exp=(exp*10)+(*ptr-'0');
      ptr++;
    } /* while */
    #if defined __GNUC__
      fmult=pow10(exp*sign);
    #else
      fmult=pow(10,exp*sign);
    #endif
    fnum *= fmult;
    dnum *= (unsigned long)(fmult+0.5);
  } /* if */

  /* decide how to store the number */
  if (sc_rationaltag==0) {
    error(70);          /* rational number support was not enabled */
    *val=0;
  } else if (rational_digits==0) {
    /* floating point */
    #if PAWN_CELL_SIZE==32
      float value=(float)fnum;
      *val=*((cell *)&value);
      #if 0 /* SourceMod - not needed */
        /* I assume that the C/C++ compiler stores "float" values in IEEE 754
         * format (as mandated in the ANSI standard). Test this assumption
         * anyway.
         * Note: problems have been reported with GCC 3.2.x, version 3.3.x works.
         */
        { float test1 = 0.0, test2 = 50.0, test3 = -50.0;
          uint32_t bit = 1;
          /* test 0.0 == all bits 0 */
          assert(*(uint32_t*)&test1==0x00000000L);
          /* test sign & magnitude format */
          assert(((*(uint32_t*)&test2) ^ (*(uint32_t*)&test3)) == (bit << (PAWN_CELL_SIZE-1)));
          /* test a known value */
          assert(*(uint32_t*)&test2==0x42480000L);
        }
      #endif
    #elif PAWN_CELL_SIZE==64
      *val=*((cell *)&fnum);
      #if 0 /* SourceMod - not needed */
        /* I assume that the C/C++ compiler stores "double" values in IEEE 754
         * format (as mandated in the ANSI standard).
         */
        { float test1 = 0.0, test2 = 50.0, test3 = -50.0;
          uint64_t bit = 1;
          /* test 0.0 == all bits 0 */
          assert(*(uint64_t*)&test1==0x00000000L);
          /* test sign & magnitude format */
          assert(((*(uint64_t*)&test2) ^ (*(uint64_t*)&test3)) == (bit << (PAWN_CELL_SIZE-1)));
        }
      #endif
    #else
      #error Unsupported cell size
    #endif
  } else {
    /* fixed point */
    *val=(cell)dnum;
  } /* if */

  return (int)(ptr-curptr);
}

/*  number
 *
 *  Reads in a number (binary, decimal or hexadecimal). It returns the number
 *  of characters processed or 0 if the symbol couldn't be interpreted as a
 *  number (in this case the argument "val" remains unchanged). This routine
 *  relies on the 'early dropout' implementation of the logical or (||)
 *  operator.
 *
 *  Note: the routine doesn't check for a sign (+ or -). The - is checked
 *        for at "hier2()" (in fact, it is viewed as an operator, not as a
 *        sign) and the + is invalid (as in K&R C, and unlike ANSI C).
 */
static int number(cell *val,const unsigned char *curptr)
{
  int i;
  cell value;

  if ((i=btoi(&value,curptr))!=0      /* binary? */
      || (i=htoi(&value,curptr))!=0   /* hexadecimal? */
      || (i=dtoi(&value,curptr))!=0)  /* decimal? */
  {
    *val=value;
    return i;
  } else {
    return 0;                      /* else not a number */
  } /* if */
}

static void chrcat(char *str,char chr)
{
  str=strchr(str,'\0');
  *str++=chr;
  *str='\0';
}

static int preproc_expr(cell *val,int *tag)
{
  int result;
  int index;
  cell code_index;
  char *term;

  /* Disable staging; it should be disabled already because
   * expressions may not be cut off half-way between conditional
   * compilations. Reset the staging index, but keep the code
   * index.
   */
  if (stgget(&index,&code_index)) {
    error(57);                          /* unfinished expression */
    stgdel(0,code_index);
    stgset(FALSE);
  } /* if */
  assert((lptr-pline)<(int)strlen((char*)pline));   /* lptr must point inside the string */
  #if !defined NO_DEFINE
    /* preprocess the string */
    substallpatterns(pline,sLINEMAX);
    assert((lptr-pline)<(int)strlen((char*)pline)); /* lptr must STILL point inside the string */
  #endif
  /* append a special symbol to the string, so the expression
   * analyzer won't try to read a next line when it encounters
   * an end-of-line
   */
  assert(strlen((char*)pline)<sLINEMAX);
  term=strchr((char*)pline,'\0');
  assert(term!=NULL);
  chrcat((char*)pline,PREPROC_TERM);    /* the "DEL" code (see SC.H) */
  result=constexpr(val,tag,NULL);       /* get value (or 0 on error) */
  *term='\0';                           /* erase the token (if still present) */
  lexclr(FALSE);                        /* clear any "pushed" tokens */
  return result;
}

/* getstring
 * Returns returns a pointer behind the closing quote or to the other
 * character that caused the input to be ended.
 */
static const unsigned char *getstring(unsigned char *dest,int max,const unsigned char *line)
{
  assert(dest!=NULL && line!=NULL);
  *dest='\0';
  while (*line<=' ' && *line!='\0')
    line++;             /* skip whitespace */
  if (*line=='"') {
    int len=0;
    line++;             /* skip " */
    while (*line!='"' && *line!='\0') {
      if (len<max-1)
        dest[len++]=*line;
      line++;
    } /* if */
    dest[len]='\0';
    if (*line=='"')
      line++;           /* skip closing " */
    else
      error(37);        /* invalid string */
  } else {
    error(37);          /* invalid string */
  } /* if */
  return line;
}

enum {
  CMD_NONE,
  CMD_TERM,
  CMD_EMPTYLINE,
  CMD_CONDFALSE,
  CMD_INCLUDE,
  CMD_DEFINE,
  CMD_IF,
  CMD_DIRECTIVE,
};

/*  command
 *
 *  Recognizes the compiler directives. The function returns:
 *     CMD_NONE         the line must be processed
 *     CMD_TERM         a pending expression must be completed before processing further lines
 *     Other value: the line must be skipped, because:
 *     CMD_CONDFALSE    false "#if.." code
 *     CMD_EMPTYLINE    line is empty
 *     CMD_INCLUDE      the line contains a #include directive
 *     CMD_DEFINE       the line contains a #subst directive
 *     CMD_IF           the line contains a #if/#else/#endif directive
 *     CMD_DIRECTIVE    the line contains some other compiler directive
 *
 *  Global variables: iflevel, ifstack (altered)
 *                    lptr      (altered)
 */
static int command(void)
{
  int tok,ret;
  cell val;
  char *str;
  int index;
  cell code_index;

  while (*lptr<=' ' && *lptr!='\0')
    lptr+=1;
  if (*lptr=='\0')
    return CMD_EMPTYLINE;       /* empty line */
  if (*lptr!='#')
    return SKIPPING ? CMD_CONDFALSE : CMD_NONE; /* it is not a compiler directive */
  /* compiler directive found */
  indent_nowarn=TRUE;           /* allow loose indentation" */
  lexclr(FALSE);                /* clear any "pushed" tokens */
  /* on a pending expression, force to return a silent ';' token and force to
   * re-read the line
   */
  if (!sc_needsemicolon && stgget(&index,&code_index)) {
    lptr=term_expr;
    return CMD_TERM;
  } /* if */
  tok=lex(&val,&str);
  ret=SKIPPING ? CMD_CONDFALSE : CMD_DIRECTIVE;  /* preset 'ret' to CMD_DIRECTIVE (most common case) */
  switch (tok) {
  case tpIF:                    /* conditional compilation */
    ret=CMD_IF;
    assert(iflevel>=0);
    if (iflevel>=sCOMP_STACK)
      error(122,"Conditional compilation stack"); /* table overflow */
    iflevel++;
    if (SKIPPING)
      break;                    /* break out of switch */
    skiplevel=iflevel;
    preproc_expr(&val,NULL);    /* get value (or 0 on error) */
    ifstack[iflevel-1]=(char)(val ? PARSEMODE : SKIPMODE);
    check_empty(lptr);
    break;
  case tpELSE:
  case tpELSEIF:
    ret=CMD_IF;
    assert(iflevel>=0);
    if (iflevel==0) {
      error(26);                /* no matching #if */
      errorset(sRESET,0);
    } else {
      /* check for earlier #else */
      if ((ifstack[iflevel-1] & HANDLED_ELSE)==HANDLED_ELSE) {
        if (tok==tpELSEIF)
          error(61);            /* #elseif directive may not follow an #else */
        else
          error(60);            /* multiple #else directives between #if ... #endif */
        errorset(sRESET,0);
      } else {
        assert(iflevel>0);
        /* if there has been a "parse mode" on this level, set "skip mode",
         * otherwise, clear "skip mode"
         */
        if ((ifstack[iflevel-1] & PARSEMODE)==PARSEMODE) {
          /* there has been a parse mode already on this level, so skip the rest */
          ifstack[iflevel-1] |= (char)SKIPMODE;
          /* if we were already skipping this section, allow expressions with
           * undefined symbols; otherwise check the expression to catch errors
           */
          if (tok==tpELSEIF) {
            if (skiplevel==iflevel)
              preproc_expr(&val,NULL);  /* get, but ignore the expression */
            else
              lptr=(unsigned char*)strchr((char*)lptr,'\0');
          } /* if */
        } else {
          /* previous conditions were all FALSE */
          if (tok==tpELSEIF) {
            /* if we were already skipping this section, allow expressions with
             * undefined symbols; otherwise check the expression to catch errors
             */
            if (skiplevel==iflevel) {
              preproc_expr(&val,NULL);  /* get value (or 0 on error) */
            } else {
              lptr=(unsigned char*)strchr((char*)lptr,'\0');
              val=0;
            } /* if */
            ifstack[iflevel-1]=(char)(val ? PARSEMODE : SKIPMODE);
          } else {
            /* a simple #else, clear skip mode */
            ifstack[iflevel-1] &= (char)~SKIPMODE;
          } /* if */
        } /* if */
      } /* if */
    } /* if */
    check_empty(lptr);
    break;
  case tpENDIF:
    ret=CMD_IF;
    if (iflevel==0){
      error(26);        /* no matching "#if" */
      errorset(sRESET,0);
    } else {
      iflevel--;
      if (iflevel<skiplevel)
        skiplevel=iflevel;
    } /* if */
    check_empty(lptr);
    break;
  case tINCLUDE:                /* #include directive */
  case tpTRYINCLUDE:
    ret=CMD_INCLUDE;
    if (!SKIPPING)
      doinclude(tok==tpTRYINCLUDE);
    break;
  case tpFILE:
    if (!SKIPPING) {
      char pathname[_MAX_PATH];
      lptr=getstring((unsigned char*)pathname,sizeof pathname,lptr);
      if (strlen(pathname)>0) {
        free(inpfname);
        inpfname=duplicatestring(pathname);
        if (inpfname==NULL)
          error(123);           /* insufficient memory */
        fline=0;
      } /* if */
    } /* if */
    check_empty(lptr);
    break;
  case tpLINE:
    if (!SKIPPING) {
      if (lex(&val,&str)!=tNUMBER)
        error(8);               /* invalid/non-constant expression */
      fline=(int)val;
    } /* if */
    check_empty(lptr);
    break;
  case tpASSERT:
    if (!SKIPPING && (sc_debug & sCHKBOUNDS)!=0) {
      for (str=(char*)lptr; *str<=' ' && *str!='\0'; str++)
        /* nothing */;          /* save start of expression */
      preproc_expr(&val,NULL);  /* get constant expression (or 0 on error) */
      if (!val)
        error(130,str);         /* assertion failed */
      check_empty(lptr);
    } /* if */
    break;
  case tpPRAGMA:
    if (!SKIPPING) {
      if (lex(&val,&str)==tSYMBOL) {
        if (strcmp(str,"amxlimit")==0) {
          preproc_expr(&pc_amxlimit,NULL);
        } else if (strcmp(str,"amxram")==0) {
          preproc_expr(&pc_amxram,NULL);
        } else if (strcmp(str,"codepage")==0) {
          char name[sNAMEMAX+1];
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          if (*lptr=='"') {
            lptr=getstring((unsigned char*)name,sizeof name,lptr);
          } else {
            int i;
            for (i=0; i<sizeof name && alphanum(*lptr); i++,lptr++)
              name[i]=*lptr;
            name[i]='\0';
          } /* if */
          if (!cp_set(name))
            error(128);         /* codepage mapping file not found */
        } else if (strcmp(str,"compress")==0) {
          cell val;
          preproc_expr(&val,NULL);
          sc_compress=(int)val; /* switch code packing on/off */
        } else if (strcmp(str,"ctrlchar")==0) {
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          if (*lptr=='\0') {
            sc_ctrlchar=sc_ctrlchar_org;
          } else {
            if (lex(&val,&str)!=tNUMBER)
              error(27);          /* invalid character constant */
            sc_ctrlchar=(char)val;
          } /* if */
        } else if (strcmp(str,"deprecated")==0) {
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          pc_deprecate=(char*)malloc(strlen((char*)lptr)+1);
          if (pc_deprecate!=NULL)
            strcpy(pc_deprecate,(char*)lptr);
          lptr=(unsigned char*)strchr((char*)lptr,'\0'); /* skip to end (ignore "extra characters on line") */
        } else if (strcmp(str,"dynamic")==0) {
          preproc_expr(&pc_stksize,NULL);
        } else if (!strcmp(str,"library")
          ||!strcmp(str,"reqclass")
          ||!strcmp(str,"loadlib")
          ||!strcmp(str,"explib")
          ||!strcmp(str,"expclass")
          ||!strcmp(str,"defclasslib")) {
          char name[sNAMEMAX+1],sname[sNAMEMAX+1];
           const char *prefix="";
          sname[0]='\0';
          sname[1]='\0';
          if (!strcmp(str,"library"))
            prefix="??li_";
          else if (!strcmp(str,"reqclass"))
            prefix="??rc_";
          else if (!strcmp(str,"loadlib"))
            prefix="??f_";
          else if (!strcmp(str,"explib"))
            prefix="??el_";
          else if (!strcmp(str,"expclass"))
            prefix="??ec_";
          else if (!strcmp(str,"defclasslib"))
            prefix="??d_";
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          if (*lptr=='"') {
            lptr=getstring((unsigned char*)name,sizeof name,lptr);
          } else {
            int i;
            for (i=0; i<sizeof name && (alphanum(*lptr) || *lptr=='-'); i++,lptr++)
              name[i]=*lptr;
            name[i]='\0';
            if (!strncmp(str,"exp",3) || !strncmp(str,"def",3)) {
              while (*lptr && isspace(*lptr))
                lptr++;
              for (i=1; i<sizeof sname && alphanum(*lptr); i++,lptr++)
                sname[i]=*lptr;
              sname[i]='\0';
              if (!sname[1]) {
                error(45);
              } else {
                sname[0]='_';
              }
            }
          } /* if */
          if (strlen(name)==0) {
            curlibrary=NULL;
          } else if (strcmp(name,"-")==0) {
            pc_addlibtable=FALSE;
          } else {
            /* add the name if it does not yet exist in the table */
            char newname[sNAMEMAX+1];
            if (strlen(name)+strlen(prefix)+strlen(sname)<=sNAMEMAX) {
              strcpy(newname,prefix);
              strcat(newname,name);
              strcat(newname,sname);
              if (find_constval(&libname_tab,newname,0)==NULL) {
                if (!strcmp(str,"library") || !strcmp(str,"reqclass")) {
                  curlibrary=append_constval(&libname_tab,newname,1,0);
                } else {
                  append_constval(&libname_tab,newname,1,0);
                }
              }
            }
          } /* if */
#if 0	/* more unused */
        } else if (strcmp(str,"pack")==0) {
          cell val;
          preproc_expr(&val,NULL);      /* default = packed/unpacked */
          sc_packstr=(int)val;
#endif
        } else if (strcmp(str,"rational")==0) {
          char name[sNAMEMAX+1];
          cell digits=0;
          int i;
          /* first gather all information, start with the tag name */
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          for (i=0; i<sizeof name && alphanum(*lptr); i++,lptr++)
            name[i]=*lptr;
          name[i]='\0';
          /* then the precision (for fixed point arithmetic) */
          while (*lptr<=' ' && *lptr!='\0')
            lptr++;
          if (*lptr=='(') {
            preproc_expr(&digits,NULL);
            if (digits<=0 || digits>9) {
              error(68);        /* invalid rational number precision */
              digits=0;
            } /* if */
            if (*lptr==')')
              lptr++;
          } /* if */
          /* add the tag (make it public) and check the values */
          i=pc_addtag(name);
          exporttag(i);
          if (sc_rationaltag==0 || (sc_rationaltag==i && rational_digits==(int)digits)) {
            sc_rationaltag=i;
            rational_digits=(int)digits;
          } else {
            error(69);          /* rational number format already set, can only be set once */
          } /* if */
        } else if (strcmp(str,"semicolon")==0) {
          cell val;
          preproc_expr(&val,NULL);
          sc_needsemicolon=(int)val;
        } else if (strcmp(str,"tabsize")==0) {
          cell val;
          preproc_expr(&val,NULL);
          sc_tabsize=(int)val;
        } else if (strcmp(str,"align")==0) {
          sc_alignnext=TRUE;
        } else if (strcmp(str,"unused")==0) {
          char name[sNAMEMAX+1];
          int i,comma;
          symbol *sym;
          do {
            /* get the name */
            while (*lptr<=' ' && *lptr!='\0')
              lptr++;
            for (i=0; i<sizeof name && alphanum(*lptr); i++,lptr++)
              name[i]=*lptr;
            name[i]='\0';
            /* get the symbol */
            sym=findloc(name);
            if (sym==NULL)
              sym=findglb(name,sSTATEVAR);
            if (sym!=NULL) {
              sym->usage |= uREAD;
              if (sym->ident==iVARIABLE || sym->ident==iREFERENCE
                  || sym->ident==iARRAY || sym->ident==iREFARRAY)
                sym->usage |= uWRITTEN;
            } else {
              error(17,name);     /* undefined symbol */
            } /* if */
            /* see if a comma follows the name */
            while (*lptr<=' ' && *lptr!='\0')
              lptr++;
            comma= (*lptr==',');
            if (comma)
              lptr++;
          } while (comma);
        } else {
          error(207);           /* unknown #pragma */
        } /* if */
      } else {
        error(207);             /* unknown #pragma */
      } /* if */
      check_empty(lptr);
    } /* if */
    break;
  case tpENDINPUT:
  case tpENDSCRPT:
    if (!SKIPPING) {
      check_empty(lptr);
      assert(inpf!=NULL);
      if (inpf!=inpf_org)
        pc_closesrc(inpf);
      inpf=NULL;
    } /* if */
    break;
#if !defined NOEMIT
  case tpEMIT: {
    /* write opcode to output file */
    char name[40];
    int i;
    while (*lptr<=' ' && *lptr!='\0')
      lptr++;
    for (i=0; i<40 && (isalpha(*lptr) || *lptr=='.'); i++,lptr++)
      name[i]=(char)tolower(*lptr);
    name[i]='\0';
    stgwrite("\t");
    stgwrite(name);
    stgwrite(" ");
    code_idx+=opcodes(1);
    /* write parameter (if any) */
    while (*lptr<=' ' && *lptr!='\0')
      lptr++;
    if (*lptr!='\0') {
      symbol *sym;
      tok=lex(&val,&str);
      switch (tok) {
      case tNUMBER:
      case tRATIONAL:
        outval(val,FALSE);
        code_idx+=opargs(1);
        break;
      case tSYMBOL:
        sym=findloc(str);
        if (sym==NULL)
          sym=findglb(str,sSTATEVAR);
        if (sym==NULL || (sym->ident!=iFUNCTN && sym->ident!=iREFFUNC && (sym->usage & uDEFINE)==0)) {
          error(17,str);        /* undefined symbol */
        } else {
          outval(sym->addr,FALSE);
          /* mark symbol as "used", unknown whether for read or write */
          markusage(sym,uREAD | uWRITTEN);
          code_idx+=opargs(1);
        } /* if */
        break;
      default: {
        char s2[20];
        extern char *sc_tokens[];/* forward declaration */
        if (tok<256)
          sprintf(s2,"%c",(char)tok);
        else
          strcpy(s2,sc_tokens[tok-tFIRST]);
        error(1,sc_tokens[tSYMBOL-tFIRST],s2);
        break;
      } /* case */
      } /* switch */
    } /* if */
    stgwrite("\n");
    check_empty(lptr);
    break;
  } /* case */
#endif
#if !defined NO_DEFINE
  case tpDEFINE: {
    ret=CMD_DEFINE;
    if (!SKIPPING) {
      char *pattern,*substitution;
      const unsigned char *start,*end;
      int count,prefixlen;
      stringpair *def;
      /* find the pattern to match */
      while (*lptr<=' ' && *lptr!='\0')
        lptr++;
      start=lptr;       /* save starting point of the match pattern */
      count=0;
      while (*lptr>' ' && *lptr!='\0') {
        litchar(&lptr,0); /* litchar() advances "lptr" and handles escape characters */
        count++;
      } /* while */
      end=lptr;
      /* check pattern to match */
      if (!alpha(*start)) {
        error(74);      /* pattern must start with an alphabetic character */
        break;
      } /* if */
      /* store matched pattern */
      pattern=(char*)malloc(count+1);
      if (pattern==NULL)
        error(123);     /* insufficient memory */
      lptr=start;
      count=0;
      while (lptr!=end) {
        assert(lptr<end);
        assert(*lptr!='\0');
        pattern[count++]=(char)litchar(&lptr,0);
      } /* while */
      pattern[count]='\0';
      /* special case, erase trailing variable, because it could match anything */
      if (count>=2 && isdigit(pattern[count-1]) && pattern[count-2]=='%')
        pattern[count-2]='\0';
      /* find substitution string */
      while (*lptr<=' ' && *lptr!='\0')
        lptr++;
      start=lptr;       /* save starting point of the match pattern */
      count=0;
      end=NULL;
      while (*lptr!='\0') {
        /* keep position of the start of trailing whitespace */
        if (*lptr<=' ') {
          if (end==NULL)
            end=lptr;
        } else {
          end=NULL;
        } /* if */
        count++;
        lptr++;
      } /* while */
      if (end==NULL)
        end=lptr;
      /* store matched substitution */
      substitution=(char*)malloc(count+1);  /* +1 for '\0' */
      if (substitution==NULL)
        error(123);     /* insufficient memory */
      lptr=start;
      count=0;
      while (lptr!=end) {
        assert(lptr<end);
        assert(*lptr!='\0');
        substitution[count++]=*lptr++;
      } /* while */
      substitution[count]='\0';
      /* check whether the definition already exists */
      for (prefixlen=0,start=(unsigned char*)pattern; alphanum(*start); prefixlen++,start++)
        /* nothing */;
      assert(prefixlen>0);
      if ((def=find_subst(pattern,prefixlen))!=NULL) {
        if (strcmp(def->first,pattern)!=0 || strcmp(def->second,substitution)!=0)
          error(201,pattern);   /* redefinition of macro (non-identical) */
        delete_subst(pattern,prefixlen);
      } /* if */
      /* add the pattern/substitution pair to the list */
      assert(strlen(pattern)>0);
      insert_subst(pattern,substitution,prefixlen);
      free(pattern);
      free(substitution);
    } /* if */
    break;
  } /* case */
  case tpUNDEF:
    if (!SKIPPING) {
      if (lex(&val,&str)==tSYMBOL) {
        ret=delete_subst(str,strlen(str));
        if (!ret) {
          /* also undefine normal constants */
          symbol *sym=findconst(str,NULL);
          if (sym!=NULL && (sym->usage & (uENUMROOT | uENUMFIELD))==0) {
            delete_symbol(&glbtab,sym);
            ret=TRUE;
          } /* if */
        } /* if */
        if (!ret)
          error(17,str);        /* undefined symbol */
      } else {
        error(20,str);          /* invalid symbol name */
      } /* if */
      check_empty(lptr);
    } /* if */
    break;
#endif
  case tpERROR:
    while (*lptr<=' ' && *lptr!='\0')
      lptr++;
    if (!SKIPPING)
      error(131,lptr);  /* user error */
    break;
  default:
    error(31);          /* unknown compiler directive */
    ret=SKIPPING ? CMD_CONDFALSE : CMD_NONE;  /* process as normal line */
  } /* switch */
  return ret;
}

#if !defined NO_DEFINE
static int is_startstring(const unsigned char *string)
{
  if (*string=='\"' || *string=='\'')
    return TRUE;                        /* "..." */

  if (*string=='!') {
    string++;
    if (*string=='\"' || *string=='\'')
      return TRUE;                      /* !"..." */
    if (*string==sc_ctrlchar) {
      string++;
      if (*string=='\"' || *string=='\'')
        return TRUE;                    /* !\"..." */
    } /* if */
  } else if (*string==sc_ctrlchar) {
    string++;
    if (*string=='\"' || *string=='\'')
      return TRUE;                      /* \"..." */
    if (*string=='!') {
      string++;
      if (*string=='\"' || *string=='\'')
        return TRUE;                    /* \!"..." */
    } /* if */
  } /* if */

  return FALSE;
}

static const unsigned char *skipstring(const unsigned char *string)
{
  char endquote;
  int flags=0;

  while (*string=='!' || *string==sc_ctrlchar) {
    if (*string==sc_ctrlchar)
      flags=RAWMODE;
    string++;
  } /* while */

  endquote=*string;
  assert(endquote=='"' || endquote=='\'');
  string++;             /* skip open quote */
  while (*string!=endquote && *string!='\0')
    litchar(&string,flags);
  return string;
}

static const unsigned char *skippgroup(const unsigned char *string)
{
  int nest=0;
  char open=*string;
  char close;

  switch (open) {
  case '(':
    close=')';
    break;
  case '{':
    close='}';
    break;
  case '[':
    close=']';
    break;
  case '<':
    close='>';
    break;
  default:
    assert(0);
	close='\0';         /* only to avoid a compiler warning */
  }/* switch */

  string++;
  while (*string!=close || nest>0) {
    if (*string==open)
      nest++;
    else if (*string==close)
      nest--;
    else if (is_startstring(string))
      string=skipstring(string);
    if (*string=='\0')
      break;
    string++;
  } /* while */
  return string;
}

static char *strdel(char *str,size_t len)
{
  size_t length=strlen(str);
  if (len>length)
    len=length;
  memmove(str, str+len, length-len+1);  /* include EOS byte */
  return str;
}

static char *strins(char *dest,char *src,size_t srclen)
{
  size_t destlen=strlen(dest);
  assert(srclen<=strlen(src));
  memmove(dest+srclen, dest, destlen+1);/* include EOS byte */
  memcpy(dest, src, srclen);
  return dest;
}

static int substpattern(unsigned char *line,size_t buffersize,char *pattern,char *substitution)
{
  int prefixlen;
  const unsigned char *p,*s,*e;
  unsigned char *args[10];
  int match,arg,len,argsnum=0;
  int stringize;

  memset(args,0,sizeof args);

  /* check the length of the prefix */
  for (prefixlen=0,s=(unsigned char*)pattern; alphanum(*s); prefixlen++,s++)
    /* nothing */;
  assert(prefixlen>0);
  assert(strncmp((char*)line,pattern,prefixlen)==0);

  /* pattern prefix matches; match the rest of the pattern, gather
   * the parameters
   */
  s=line+prefixlen;
  p=(unsigned char*)pattern+prefixlen;
  match=TRUE;         /* so far, pattern matches */
  while (match && *s!='\0' && *p!='\0') {
    if (*p=='%') {
      p++;            /* skip '%' */
      if (isdigit(*p)) {
        arg=*p-'0';
        assert(arg>=0 && arg<=9);
        p++;          /* skip parameter id */
        assert(*p!='\0');
        /* match the source string up to the character after the digit
         * (skipping strings in the process
         */
        e=s;
        while (*e!=*p && *e!='\0' && *e!='\n') {
          if (is_startstring(e))              /* skip strings */
            e=skipstring(e);
          else if (strchr("({[",*e)!=NULL)    /* skip parenthized groups */
            e=skippgroup(e);
          if (*e!='\0')
            e++;      /* skip non-alphapetic character (or closing quote of
                       * a string, or the closing paranthese of a group) */
        } /* while */
        /* store the parameter (overrule any earlier) */
        if (args[arg]!=NULL)
          free(args[arg]);
		else
          argsnum++;
        len=(int)(e-s);
        args[arg]=(unsigned char*)malloc(len+1);
        if (args[arg]==NULL)
          error(123); /* insufficient memory */
        strlcpy((char*)args[arg],(char*)s,len+1);
        /* character behind the pattern was matched too */
        if (*e==*p) {
          s=e+1;
        } else if (*e=='\n' && *p==';' && *(p+1)=='\0' && !sc_needsemicolon) {
          s=e;    /* allow a trailing ; in the pattern match to end of line */
        } else {
          assert(*e=='\0' || *e=='\n');
          match=FALSE;
          s=e;
        } /* if */
        p++;
      } else {
        match=FALSE;
      } /* if */
    } else if (*p==';' && *(p+1)=='\0' && !sc_needsemicolon) {
      /* source may be ';' or end of the line */
      while (*s<=' ' && *s!='\0')
        s++;          /* skip white space */
      if (*s!=';' && *s!='\0')
        match=FALSE;
      p++;            /* skip the semicolon in the pattern */
    } else {
      cell ch;
      /* skip whitespace between two non-alphanumeric characters, except
       * for two identical symbols
       */
      assert((char*)p>pattern);
      if (!alphanum(*p) && *(p-1)!=*p)
        while (*s<=' ' && *s!='\0')
          s++;                  /* skip white space */
      ch=litchar(&p,0);         /* this increments "p" */
      if (*s!=ch)
        match=FALSE;
      else
        s++;                    /* this character matches */
    } /* if */
  } /* while */

  if (match && *p=='\0') {
    /* if the last character to match is an alphanumeric character, the
     * current character in the source may not be alphanumeric
     */
    assert(p>(unsigned char*)pattern);
    if (alphanum(*(p-1)) && alphanum(*s))
      match=FALSE;
  } /* if */

  if (match) {
    /* calculate the length of the substituted string */
    for (e=(unsigned char*)substitution,len=0; *e!='\0'; e++) {
      if(*e=='#' && *(e+1)=='%' && isdigit(*(e+2)) && argsnum) {
        stringize=1;
          e++;           /* skip '#' */
      } else {
        stringize=0;
      } /* if */
      if (*e=='%' && isdigit(*(e+1)) && argsnum) {
        arg=*(e+1)-'0';
        assert(arg>=0 && arg<=9);
        assert(stringize==0 || stringize==1);
        if (args[arg]!=NULL) {
          len+=strlen((char*)args[arg])+2*stringize;
          e++;
        } else {
          len++;
        }
      } else {
        len++;
      } /* if */
    } /* for */
    /* check length of the string after substitution */
    if (strlen((char*)line) + len - (int)(s-line) > buffersize) {
      error(75);      /* line too long */
    } else {
      /* substitute pattern */
      strdel((char*)line,(int)(s-line));
      for (e=(unsigned char*)substitution,s=line; *e!='\0'; e++) {
        if (*e=='#' && *(e+1)=='%' && isdigit(*(e+2))) {
          stringize=1;
          e++;         /* skip '#' */
        } else {
          stringize=0;
        } /* if */
        if (*e=='%' && isdigit(*(e+1))) {
          arg=*(e+1)-'0';
          assert(arg>=0 && arg<=9);
          if (args[arg]!=NULL) {
            if (stringize)
              strins((char*)s++,"\"",1);
            strins((char*)s,(char*)args[arg],strlen((char*)args[arg]));
            s+=strlen((char*)args[arg]);
            if (stringize)
              strins((char*)s++,"\"",1);
          } else {
            error(236); /* parameter does not exist, incorrect #define pattern */
            strins((char*)s,(char*)e,2);
            s+=2;
          } /* if */
          e++;          /* skip %, digit is skipped later */
        } else if (*e == '"') {
          p=e;
          if (is_startstring(e)) {
            e=skipstring(e);
            strins((char*)s,(char *)p,(e-p+1));
            s+=(e-p+1);
          } else {
            strins((char*)s,(char*)e,1);
            s++;
          }
        } else {
          strins((char*)s,(char*)e,1);
          s++;
        } /* if */
      } /* for */
    } /* if */
  } /* if */

  for (arg=0; arg<10; arg++)
    if (args[arg]!=NULL)
      free(args[arg]);

  return match;
}

static void substallpatterns(unsigned char *line,int buffersize)
{
  unsigned char *start, *end;
  int prefixlen;
  stringpair *subst;

  start=line;
  while (*start!='\0') {
    /* find the start of a prefix (skip all non-alphabetic characters),
     * also skip strings
     */
    while (!alpha(*start) && *start!='\0') {
      /* skip strings */
      if (is_startstring(start)) {
        start=(unsigned char *)skipstring(start);
        if (*start=='\0')
          break;        /* abort loop on error */
      } /* if */
      start++;          /* skip non-alphapetic character (or closing quote of a string) */
    } /* while */
    if (*start=='\0')
      break;            /* abort loop on error */
    /* if matching the operator "defined", skip it plus the symbol behind it */
	if (strncmp((char*)start,"defined",7)==0 && !isalpha((char)*(start+7))) {
      start+=7;         /* skip "defined" */
      /* skip white space & parantheses */
      while ((*start<=' ' && *start!='\0') || *start=='(')
        start++;
      /* skip the symbol behind it */
      while (alphanum(*start))
        start++;
      /* drop back into the main loop */
      continue;
    } /* if */
    /* get the prefix (length), look for a matching definition */
    prefixlen=0;
    end=start;
    while (alphanum(*end)) {
      prefixlen++;
      end++;
    } /* while */
    assert(prefixlen>0);
    subst=find_subst((char*)start,prefixlen);
    if (subst!=NULL) {
      /* properly match the pattern and substitute */
      if (!substpattern(start,buffersize-(int)(start-line),subst->first,subst->second))
        start=end;      /* match failed, skip this prefix */
      /* match succeeded: do not update "start", because the substitution text
       * may be matched by other macros
       */
    } else {
      start=end;        /* no macro with this prefix, skip this prefix */
    } /* if */
  } /* while */
}
#endif

/*  scanellipsis
 *  Look for ... in the string and (if not there) in the remainder of the file,
 *  but restore (or keep intact):
 *  - the current position in the file
 *  - the comment parsing state
 *  - the line buffer used by the lexical analyser
 *  - the active line number and the active file
 *
 *  The function returns 1 if an ellipsis was found and 0 if not
 */
static int scanellipsis(const unsigned char *lptr)
{
  static void *inpfmark=NULL;
  unsigned char *localbuf;
  short localcomment,found;

  /* first look for the ellipsis in the remainder of the string */
  while (*lptr<=' ' && *lptr!='\0')
    lptr++;
  if (lptr[0]=='.' && lptr[1]=='.' && lptr[2]=='.')
    return 1;
  if (*lptr!='\0')
    return 0;           /* stumbled on something that is not an ellipsis and not white-space */

  /* the ellipsis was not on the active line, read more lines from the current
   * file (but save its position first)
   */
  if (inpf==NULL || pc_eofsrc(inpf))
    return 0;           /* quick exit: cannot read after EOF */
  if ((localbuf=(unsigned char*)malloc((sLINEMAX+1)*sizeof(unsigned char)))==NULL)
    return 0;
  inpfmark=pc_getpossrc(inpf,inpfmark);
  localcomment=icomment;

  found=0;
  /* read from the file, skip preprocessing, but strip off comments */
  while (!found && pc_readsrc(inpf,localbuf,sLINEMAX)!=NULL) {
    stripcom(localbuf);
    lptr=localbuf;
    /* skip white space */
    while (*lptr<=' ' && *lptr!='\0')
      lptr++;
    if (lptr[0]=='.' && lptr[1]=='.' && lptr[2]=='.')
      found=1;
    else if (*lptr!='\0')
      break;                       /* stumbled on something that is not an ellipsis and not white-space */
  } /* while */

  /* clean up & reset */
  free(localbuf);
  pc_resetsrc(inpf,inpfmark);
  icomment=localcomment;
  return found;
}

/*  preprocess
 *
 *  Reads a line by readline() into "pline" and performs basic preprocessing:
 *  deleting comments, skipping lines with false "#if.." code and recognizing
 *  other compiler directives. There is an indirect recursion: lex() calls
 *  preprocess() if a new line must be read, preprocess() calls command(),
 *  which at his turn calls lex() to identify the token.
 *
 *  Global references: lptr     (altered)
 *                     pline    (altered)
 *                     freading (referred to only)
 */
SC_FUNC void preprocess(void)
{
  int iscommand;

  if (!freading)
    return;
  do {
    readline(pline);
    stripcom(pline);    /* ??? no need for this when reading back from list file (in the second pass) */
    lptr=pline;         /* set "line pointer" to start of the parsing buffer */
    iscommand=command();
    if (iscommand!=CMD_NONE)
      errorset(sRESET,0); /* reset error flag ("panic mode") on empty line or directive */
    #if !defined NO_DEFINE
      if (iscommand==CMD_NONE) {
        assert(lptr!=term_expr);
        substallpatterns(pline,sLINEMAX);
        lptr=pline;       /* reset "line pointer" to start of the parsing buffer */
      } /* if */
    #endif
    if (sc_status==statFIRST && sc_listing && freading
        && (iscommand==CMD_NONE || iscommand==CMD_EMPTYLINE || iscommand==CMD_DIRECTIVE))
    {
      listline++;
      if (fline!=listline) {
        listline=fline;
        setlinedirect(fline);
      } /* if */
      if (iscommand==CMD_EMPTYLINE)
        pc_writeasm(outf,"\n");
      else
        pc_writeasm(outf,(char*)pline);
    } /* if */
  } while (iscommand!=CMD_NONE && iscommand!=CMD_TERM && freading); /* enddo */
}

static const unsigned char *unpackedstring(const unsigned char *lptr,int flags)
{
  while (*lptr!='\"' && *lptr!='\0') {
    if (*lptr=='\a') {          /* ignore '\a' (which was inserted at a line concatenation) */
      lptr++;
      continue;
    } /* if */
    litadd(litchar(&lptr,flags | UTF8MODE));  /* litchar() alters "lptr" */
  } /* while */
  litadd(0);                    /* terminate string */
  return lptr;
}

static const unsigned char *packedstring(const unsigned char *lptr,int flags)
{
  int i;
  ucell val,c;

  i=0; /* start at least significant byte */
  val=0;
  glbstringread=1;
  while (*lptr!='\"' && *lptr!='\0') {
    if (*lptr=='\a') {          /* ignore '\a' (which was inserted at a line concatenation) */
      lptr++;
      continue;
    } /* if */
    c=litchar(&lptr,flags);     /* litchar() alters "lptr" */
    if (c>=(ucell)(1 << sCHARBITS))
      error(43);                /* character constant exceeds range */
    val |= (c << 8*i);
    glbstringread++;
    if (i==sizeof(ucell)-(sCHARBITS/8)) {
      litadd(val);
      val=0;
      i=0;
    } else {
      i=i+1;
    }
  } /* if */
  /* save last code; make sure there is at least one terminating zero character */
  if (i!=0)
    litadd(val);        /* at least one zero character in "val" */
  else
    litadd(0);          /* add full cell of zeros */
  return lptr;
}

/*  lex(lexvalue,lexsym)        Lexical Analysis
 *
 *  lex() first deletes leading white space, then checks for multi-character
 *  operators, keywords (including most compiler directives), numbers,
 *  labels, symbols and literals (literal characters are converted to a number
 *  and are returned as such). If every check fails, the line must contain
 *  a single-character operator. So, lex() returns this character. In the other
 *  case (something did match), lex() returns the number of the token. All
 *  these tokens have been assigned numbers above 255.
 *
 *  Some tokens have "attributes":
 *     tNUMBER        the value of the number is return in "lexvalue".
 *     tRATIONAL      the value is in IEEE 754 encoding or in fixed point
 *                    encoding in "lexvalue".
 *     tSYMBOL        the first sNAMEMAX characters of the symbol are
 *                    stored in a buffer, a pointer to this buffer is
 *                    returned in "lexsym".
 *     tLABEL         the first sNAMEMAX characters of the label are
 *                    stored in a buffer, a pointer to this buffer is
 *                    returned in "lexsym".
 *     tSTRING        the string is stored in the literal pool, the index
 *                    in the literal pool to this string is stored in
 *                    "lexvalue".
 *
 *  lex() stores all information (the token found and possibly its attribute)
 *  in global variables. This allows a token to be examined twice. If "_pushed"
 *  is true, this information is returned.
 *
 *  Global references: lptr          (altered)
 *                     fline         (referred to only)
 *                     litidx        (referred to only)
 *                     _lextok, _lexval, _lexstr
 *                     _pushed
 */

static int _pushed;
static int _lextok;
static cell _lexval;
static char _lexstr[sLINEMAX+1];
static int _lexnewline;

SC_FUNC void lexinit(void)
{
  stkidx=0;             /* index for pushstk() and popstk() */
  iflevel=0;            /* preprocessor: nesting of "#if" is currently 0 */
  skiplevel=0;          /* preprocessor: not currently skipping */
  icomment=0;           /* currently not in a multiline comment */
  _pushed=FALSE;        /* no token pushed back into lex */
  _lexnewline=FALSE;
}

char *sc_tokens[] = {
         "*=", "/=", "%=", "+=", "-=", "<<=", ">>>=", ">>=", "&=", "^=", "|=",
         "||", "&&", "==", "!=", "<=", ">=", "<<", ">>>", ">>", "++", "--",
         "...", "..", "::",
         "assert", "*begin", "break", "case", "cellsof", "chars", "const", "continue", "default",
         "defined", "do", "else", "*end", "enum", "exit", "for", "forward", "funcenum", "functag", "goto",
         "if", "native", "new", "decl", "operator", "public", "return", "sizeof",
         "sleep", "static", "stock", "struct", "switch", "tagof", "*then", "while",
         "#assert", "#define", "#else", "#elseif", "#emit", "#endif", "#endinput",
         "#endscript", "#error", "#file", "#if", "#include", "#line", "#pragma",
         "#tryinclude", "#undef",
         ";", ";", "-integer value-", "-rational value-", "-identifier-",
         "-label-", "-string-"
       };

SC_FUNC int lex(cell *lexvalue,char **lexsym)
{
  int i,toolong,newline;
  char **tokptr;
  const unsigned char *starttoken;

  if (_pushed) {
    _pushed=FALSE;      /* reset "_pushed" flag */
    *lexvalue=_lexval;
    *lexsym=_lexstr;
    return _lextok;
  } /* if */

  _lextok=0;            /* preset all values */
  _lexval=0;
  _lexstr[0]='\0';
  *lexvalue=_lexval;
  *lexsym=_lexstr;
  _lexnewline=FALSE;
  if (!freading)
    return 0;

  newline= (lptr==pline);       /* does lptr point to start of line buffer */
  while (*lptr<=' ') {          /* delete leading white space */
    if (*lptr=='\0') {
      preprocess();             /* preprocess resets "lptr" */
      if (!freading)
        return 0;
      if (lptr==term_expr)      /* special sequence to terminate a pending expression */
        return (_lextok=tENDEXPR);
      _lexnewline=TRUE;         /* set this after preprocess(), because
                                 * preprocess() calls lex() recursively */
      newline=TRUE;
    } else {
      lptr+=1;
    } /* if */
  } /* while */
  if (newline) {
    stmtindent=0;
    for (i=0; i<(int)(lptr-pline); i++)
      if (pline[i]=='\t' && sc_tabsize>0)
        stmtindent += (int)(sc_tabsize - (stmtindent+sc_tabsize) % sc_tabsize);
      else
        stmtindent++;
  } /* if */

  i=tFIRST;
  tokptr=sc_tokens;
  while (i<=tMIDDLE) {  /* match multi-character operators */
    if (*lptr==**tokptr && match(*tokptr,FALSE)) {
      _lextok=i;
      if (pc_docexpr)   /* optionally concatenate to documentation string */
        insert_autolist(*tokptr);
      return _lextok;
    } /* if */
    i+=1;
    tokptr+=1;
  } /* while */
  while (i<=tLAST) {    /* match reserved words and compiler directives */
    if (*lptr==**tokptr && match(*tokptr,TRUE)) {
      _lextok=i;
      errorset(sRESET,0); /* reset error flag (clear the "panic mode")*/
      if (pc_docexpr)   /* optionally concatenate to documentation string */
        insert_autolist(*tokptr);
      return _lextok;
    } /* if */
    i+=1;
    tokptr+=1;
  } /* while */

  starttoken=lptr;      /* save start pointer (for concatenating to documentation string) */
  if ((i=number(&_lexval,lptr))!=0) {   /* number */
    _lextok=tNUMBER;
    *lexvalue=_lexval;
    lptr+=i;
  } else if ((i=ftoi(&_lexval,lptr))!=0) {
    _lextok=tRATIONAL;
    *lexvalue=_lexval;
    lptr+=i;
  } else if (alpha(*lptr)) {            /* symbol or label */
    /*  Note: only sNAMEMAX characters are significant. The compiler
     *        generates a warning if a symbol exceeds this length.
     */
    _lextok=tSYMBOL;
    i=0;
    toolong=0;
    while (alphanum(*lptr)){
      _lexstr[i]=*lptr;
      lptr+=1;
      if (i<sNAMEMAX)
        i+=1;
      else
        toolong=1;
    } /* while */
    _lexstr[i]='\0';
    if (toolong)
      error(200,_lexstr,sNAMEMAX);  /* symbol too long, truncated to sNAMEMAX chars */
    if (_lexstr[0]==PUBLIC_CHAR && _lexstr[1]=='\0') {
      _lextok=PUBLIC_CHAR;  /* '@' all alone is not a symbol, it is an operator */
    } else if (_lexstr[0]=='_' && _lexstr[1]=='\0') {
      _lextok='_';      /* '_' by itself is not a symbol, it is a placeholder */
    } /* if */
    if (*lptr==':' && *(lptr+1)!=':' && _lextok!=PUBLIC_CHAR) {
      if (sc_allowtags) {
        _lextok=tLABEL; /* it wasn't a normal symbol, it was a label/tagname */
        lptr+=1;        /* skip colon */
      } else if (find_constval(&tagname_tab,_lexstr,0)!=NULL) {
        /* this looks like a tag override (because a tag with this name
         * exists), but tags are not allowed right now, so it is probably an
         * error
         */
        error(220);
      } /* if */
    } /* if */
  } else if (*lptr=='\"'                                 /* unpacked string literal */
#if 0
             || (*lptr==sc_ctrlchar && *(lptr+1)=='\"')  /* unpacked raw string */
             || (*lptr=='!' && *(lptr+1)=='\"')          /* packed string */
             || (*lptr=='!' && *(lptr+1)==sc_ctrlchar && *(lptr+2)=='\"')  /* packed raw string */
             || (*lptr==sc_ctrlchar && *(lptr+1)=='!' && *(lptr+2)=='\"') /* packed raw string */
#endif
             )
  {                                     
    int stringflags,segmentflags;
    char *cat;
    _lextok=tSTRING;
    *lexvalue=_lexval=litidx;
    _lexstr[0]='\0';
    stringflags=-1;       /* to mark the first segment */
    for ( ;; ) {
      if(*lptr=='!')
        segmentflags= (*(lptr+1)==sc_ctrlchar) ? RAWMODE | ISPACKED : ISPACKED;
      else if (*lptr==sc_ctrlchar)
        segmentflags= (*(lptr+1)=='!') ? RAWMODE | ISPACKED : RAWMODE;
      else
        segmentflags=0;
      if ((segmentflags & ISPACKED)!=0)
        lptr+=1;          /* skip '!' character */
      if ((segmentflags & RAWMODE)!=0)
        lptr+=1;          /* skip "escape" character too */
      assert(*lptr=='\"');
      lptr+=1;
      if (stringflags==-1)
        stringflags=segmentflags;
      else if (stringflags!=segmentflags)
        error(238);       /* mixing packed/unpacked/raw strings in concatenation */
      cat=strchr(_lexstr,'\0');
      assert(cat!=NULL);
      while (*lptr!='\"' && *lptr!='\0' && (cat-_lexstr)<sLINEMAX) {
        if (*lptr!='\a') {  /* ignore '\a' (which was inserted at a line concatenation) */
          *cat++=*lptr;
					if (*lptr==sc_ctrlchar && *(lptr+1)!='\0')
						*cat++=*++lptr; /* skip escape character plus the escaped character */
				} /* if */
        lptr++;
      } /* while */
      *cat='\0';          /* terminate string */
      if (*lptr=='\"')
        lptr+=1;          /* skip final quote */
      else
        error(37);        /* invalid (non-terminated) string */
      /* see whether an ellipsis is following the string */
      if (!scanellipsis(lptr))
        break;            /* no concatenation of string literals */
      /* there is an ellipses, go on parsing (this time with full preprocessing) */
      while (*lptr<=' ') {
        if (*lptr=='\0') {
          preprocess();           /* preprocess resets "lptr" */
          assert(freading && lptr!=term_expr);
        } else {
          lptr++;
        } /* if */
      } /* while */
      assert(freading && lptr[0]=='.' && lptr[1]=='.' && lptr[2]=='.');
      lptr+=3;
      while (*lptr<=' ') {
        if (*lptr=='\0') {
          preprocess();           /* preprocess resets "lptr" */
          assert(freading && lptr!=term_expr);
        } else {
          lptr++;
        } /* if */
      } /* while */
      if (!freading || !(*lptr=='\"'
#if 0
                         || *lptr==sc_ctrlchar && *(lptr+1)=='\"'
                         || *lptr=='!' && *(lptr+1)=='\"'
                         || *lptr=='!' && *(lptr+1)==sc_ctrlchar && *(lptr+2)=='\"'
                         || *lptr==sc_ctrlchar && *(lptr+1)=='!' && *(lptr+2)=='\"'
#endif
                         ))
      {
        error(37);                /* invalid string concatenation */
        break;
      } /* if */
    } /* for */
    if (sc_packstr)
      stringflags ^= ISPACKED;    /* invert packed/unpacked parameters */
    if ((stringflags & ISPACKED)!=0)
      packedstring((unsigned char *)_lexstr,stringflags);
    else
      unpackedstring((unsigned char *)_lexstr,stringflags);
  } else if (*lptr=='\'') {             /* character literal */
    lptr+=1;            /* skip quote */
    _lextok=tNUMBER;
    *lexvalue=_lexval=litchar(&lptr,UTF8MODE);
    if (*lptr=='\'')
      lptr+=1;          /* skip final quote */
    else
      error(27);        /* invalid character constant (must be one character) */
  } else if (*lptr==';') {      /* semicolumn resets "error" flag */
    _lextok=';';
    lptr+=1;
    errorset(sRESET,0); /* reset error flag (clear the "panic mode")*/
  } else {
    _lextok=*lptr;      /* if every match fails, return the character */
    lptr+=1;            /* increase the "lptr" pointer */
  } /* if */

  if (pc_docexpr) {     /* optionally concatenate to documentation string */
    char *docstr=(char*)malloc(((int)(lptr-starttoken)+1)*sizeof(char));
    if (docstr!=NULL) {
      strlcpy(docstr,(char*)starttoken,(int)(lptr-starttoken)+1);
      insert_autolist(docstr);
      free(docstr);
    } /* if */
  } /* if */
  return _lextok;
}

/*  lexpush
 *
 *  Pushes a token back, so the next call to lex() will return the token
 *  last examined, instead of a new token.
 *
 *  Only one token can be pushed back.
 *
 *  In fact, lex() already stores the information it finds into global
 *  variables, so all that is to be done is set a flag that informs lex()
 *  to read and return the information from these variables, rather than
 *  to read in a new token from the input file.
 */
SC_FUNC void lexpush(void)
{
  assert(_pushed==FALSE);
  _pushed=TRUE;
}

/*  lexclr
 *
 *  Sets the variable "_pushed" to 0 to make sure lex() will read in a new
 *  symbol (a not continue with some old one). This is required upon return
 *  from Assembler mode, and in a few cases after detecting an syntax error.
 */
SC_FUNC void lexclr(int clreol)
{
  _pushed=FALSE;
  if (clreol) {
    lptr=(unsigned char*)strchr((char*)pline,'\0');
    assert(lptr!=NULL);
  } /* if */
}

/*  matchtoken
 *
 *  This routine is useful if only a simple check is needed. If the token
 *  differs from the one expected, it is pushed back.
 *  This function returns 1 for "token found" and 2 for "implied statement
 *  termination token" found --the statement termination is an end of line in
 *  an expression where there is no pending operation. Such an implied token
 *  (i.e. not present in the source code) should not be pushed back, which is
 *  why it is sometimes important to distinguish the two.
 */
SC_FUNC int matchtoken(int token)
{
  cell val;
  char *str;
  int tok;

  tok=lex(&val,&str);
  if (tok==token || (token==tTERM && (tok==';' || tok==tENDEXPR))) {
    return 1;
  } else if (!sc_needsemicolon && token==tTERM && (_lexnewline || !freading)) {
    /* Push "tok" back, because it is the token following the implicit statement
     * termination (newline) token.
     */
    lexpush();
    return 2;
  } else {
    lexpush();
    return 0;
  } /* if */
}

/*  tokeninfo
 *
 *  Returns additional information of a token after using "matchtoken()"
 *  or needtoken(). It does no harm using this routine after a call to
 *  "lex()", but lex() already returns the same information.
 *
 *  The token itself is the return value. Normally, this one is already known.
 */
SC_FUNC int tokeninfo(cell *val,char **str)
{
  /* if the token was pushed back, tokeninfo() returns the token and
   * parameters of the *next* token, not of the *current* token.
   */
  assert(!_pushed);
  *val=_lexval;
  *str=_lexstr;
  return _lextok;
}

/*  needtoken
 *
 *  This routine checks for a required token and gives an error message if
 *  it isn't there (and returns 0/FALSE in that case). Like function matchtoken(),
 *  this function returns 1 for "token found" and 2 for "statement termination
 *  token" found; see function matchtoken() for details.
 *
 *  Global references: _lextok;
 */
SC_FUNC int needtoken(int token)
{
  char s1[20],s2[20];
  int t;

  if ((t=matchtoken(token))!=0) {
    return t;
  } else {
    /* token already pushed back */
    assert(_pushed);
    if (token<256)
      sprintf(s1,"%c",(char)token);        /* single character token */
    else
      strcpy(s1,sc_tokens[token-tFIRST]);  /* multi-character symbol */
    if (!freading)
      strcpy(s2,"-end of file-");
    else if (_lextok<256)
      sprintf(s2,"%c",(char)_lextok);
    else
      strcpy(s2,sc_tokens[_lextok-tFIRST]);
    error(1,s1,s2);     /* expected ..., but found ... */
    return FALSE;
  } /* if */
}

/*  match
 *
 *  Compares a series of characters from the input file with the characters
 *  in "st" (that contains a token). If the token on the input file matches
 *  "st", the input file pointer "lptr" is adjusted to point to the next
 *  token, otherwise "lptr" remains unaltered.
 *
 *  If the parameter "end: is true, match() requires that the first character
 *  behind the recognized token is non-alphanumeric.
 *
 *  Global references: lptr   (altered)
 */
static int match(char *st,int end)
{
  int k;
  const unsigned char *ptr;

  k=0;
  ptr=lptr;
  while (st[k]) {
    if ((unsigned char)st[k]!=*ptr)
      return 0;
    k+=1;
    ptr+=1;
  } /* while */
  if (end) {            /* symbol must terminate with non-alphanumeric char */
    if (alphanum(*ptr))
      return 0;
  } /* if */
  lptr=ptr;     /* match found, skip symbol */
  return 1;
}

static void chk_grow_litq(void)
{
  if (litidx>=litmax) {
    cell *p;

    litmax+=sDEF_LITMAX;
    p=(cell *)realloc(litq,litmax*sizeof(cell));
    if (p==NULL)
      error(122,"literal table");   /* literal table overflow (fatal error) */
    litq=p;
  } /* if */
}

/*  litadd
 *
 *  Adds a value at the end of the literal queue. The literal queue is used
 *  for literal strings used in functions and for initializing array variables.
 *
 *  Global references: litidx  (altered)
 *                     litq    (altered)
 */
SC_FUNC void litadd(cell value)
{
  chk_grow_litq();
  assert(litidx<litmax);
  litq[litidx++]=value;
}

/*  litinsert
 *
 *  Inserts a value into the literal queue. This is sometimes necessary for
 *  initializing multi-dimensional arrays.
 *
 *  Global references: litidx  (altered)
 *                     litq    (altered)
 */
SC_FUNC void litinsert(cell value,int pos)
{
  chk_grow_litq();
  assert(litidx<litmax);
  assert(pos>=0 && pos<=litidx);
  memmove(litq+(pos+1),litq+pos,(litidx-pos)*sizeof(cell));
  litidx++;
  litq[pos]=value;
}

/*  litchar
 *
 *  Return current literal character and increase the pointer to point
 *  just behind this literal character.
 *
 *  Note: standard "escape sequences" are suported, but the backslash may be
 *        replaced by another character; the syntax '\ddd' is supported,
 *        but ddd must be decimal!
 */
static cell litchar(const unsigned char **lptr,int flags)
{
  cell c=0;
  const unsigned char *cptr;

  cptr=*lptr;
  if ((flags & RAWMODE)!=0 || *cptr!=sc_ctrlchar) {  /* no escape character */
    #if !defined NO_UTF8
      if (sc_is_utf8 && (flags & UTF8MODE)!=0) {
        c=get_utf8_char(cptr,&cptr);
        assert(c>=0);   /* file was already scanned for conformance to UTF-8 */
      } else {
    #endif
      #if !defined NO_CODEPAGE
        c=cp_translate(cptr,&cptr);
      #else
        c=*cptr;
        cptr+=1;
      #endif
    #if !defined NO_UTF8
      } /* if */
    #endif
  } else {
    cptr+=1;
    if (*cptr==sc_ctrlchar) {
      c=*cptr;          /* \\ == \ (the escape character itself) */
      cptr+=1;
    } else {
      switch (*cptr) {
      case 'a':         /* \a == audible alarm */
        c=7;
        cptr+=1;
        break;
      case 'b':         /* \b == backspace */
        c=8;
        cptr+=1;
        break;
      case 'e':         /* \e == escape */
        c=27;
        cptr+=1;
        break;
      case 'f':         /* \f == form feed */
        c=12;
        cptr+=1;
        break;
      case 'n':         /* \n == NewLine character */
        c=10;
        cptr+=1;
        break;
      case 'r':         /* \r == carriage return */
        c=13;
        cptr+=1;
        break;
      case 't':         /* \t == horizontal TAB */
        c=9;
        cptr+=1;
        break;
      case 'v':         /* \v == vertical TAB */
        c=11;
        cptr+=1;
        break;
      case 'x':
      {
        int digits = 0;
        cptr+=1;
        c=0;
        while (ishex(*cptr) && digits < 2) {
          if (isdigit(*cptr))
            c=(c<<4)+(*cptr-'0');
          else
            c=(c<<4)+(tolower(*cptr)-'a'+10);
          cptr++;
          digits++;
        } /* while */
        if (*cptr==';')
          cptr++;       /* swallow a trailing ';' */
        break;
      }
      case '\'':        /* \' == ' (single quote) */
      case '"':         /* \" == " (single quote) */
      case '%':         /* \% == % (percent) */
        c=*cptr;
        cptr+=1;
        break;
      default:
        if (isdigit(*cptr)) {   /* \ddd */
          c=0;
          while (*cptr>='0' && *cptr<='9')  /* decimal! */
            c=c*10 + *cptr++ - '0';
          if (*cptr==';')
            cptr++;     /* swallow a trailing ';' */
        } else {
          error(27);    /* invalid character constant */
        } /* if */
      } /* switch */
    } /* if */
  } /* if */
  *lptr=cptr;
  assert(c>=0);
  return c;
}

/*  alpha
 *
 *  Test if character "c" is alphabetic ("a".."z"), an underscore ("_")
 *  or an "at" sign ("@"). The "@" is an extension to standard C.
 */
static int alpha(char c)
{
  return (isalpha(c) || c=='_' || c==PUBLIC_CHAR);
}

/*  alphanum
 *
 *  Test if character "c" is alphanumeric ("a".."z", "0".."9", "_" or "@")
 */
SC_FUNC int alphanum(char c)
{
  return (alpha(c) || isdigit(c));
}

/*  ishex
 *
 *  Test if character "c" is a hexadecimal digit ("0".."9" or "a".."f").
 */
SC_FUNC int ishex(char c)
{
  return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
}

/* The local variable table must be searched backwards, so that the deepest
 * nesting of local variables is searched first. The simplest way to do
 * this is to insert all new items at the head of the list.
 * In the global list, the symbols are kept in sorted order, so that the
 * public functions are written in sorted order.
 */
static symbol *add_symbol(symbol *root,symbol *entry,int sort)
{
  symbol *newsym;
  int global = root==&glbtab;

  if (sort)
    while (root->next!=NULL && strcmp(entry->name,root->next->name)>0)
      root=root->next;

  if ((newsym=(symbol *)malloc(sizeof(symbol)))==NULL) {
    error(123);
    return NULL;
  } /* if */
  memcpy(newsym,entry,sizeof(symbol));
  newsym->next=root->next;
  root->next=newsym;
  if (global)
    AddToHashTable(sp_Globals, newsym);
  return newsym;
}

static void free_symbol(symbol *sym)
{
  arginfo *arg;

  /* free all sub-symbol allocated memory blocks, depending on the
   * kind of the symbol
   */
  assert(sym!=NULL);
  if (sym->ident==iFUNCTN) {
    /* run through the argument list; "default array" arguments
     * must be freed explicitly; the tag list must also be freed */
    assert(sym->dim.arglist!=NULL);
    for (arg=sym->dim.arglist; arg->ident!=0; arg++) {
      if (arg->ident==iREFARRAY && arg->hasdefault)
        free(arg->defvalue.array.data);
      else if (arg->ident==iVARIABLE
               && ((arg->hasdefault & uSIZEOF)!=0 || (arg->hasdefault & uTAGOF)!=0))
        free(arg->defvalue.size.symname);
      assert(arg->tags!=NULL);
      free(arg->tags);
    } /* for */
    free(sym->dim.arglist);
    if (sym->states!=NULL) {
      delete_consttable(sym->states);
      free(sym->states);
    } /* if */
  } else if (sym->ident==iVARIABLE || sym->ident==iARRAY) {
    if (sym->states!=NULL) {
      delete_consttable(sym->states);
      free(sym->states);
    } /* if */
  } else if (sym->ident==iCONSTEXPR && (sym->usage & uENUMROOT)==uENUMROOT) {
    /* free the constant list of an enum root */
    assert(sym->dim.enumlist!=NULL);
    delete_consttable(sym->dim.enumlist);
    free(sym->dim.enumlist);
  } /* if */
  assert(sym->refer!=NULL);
  free(sym->refer);
  if (sym->documentation!=NULL)
    free(sym->documentation);
  free(sym);
}

SC_FUNC void delete_symbol(symbol *root,symbol *sym)
{
  symbol *origRoot=root;
  /* find the symbol and its predecessor
   * (this function assumes that you will never delete a symbol that is not
   * in the table pointed at by "root")
   */
  assert(root!=sym);
  while (root->next!=sym) {
    root=root->next;
    assert(root!=NULL);
  } /* while */

  if (origRoot==&glbtab)
    RemoveFromHashTable(sp_Globals, sym);

  /* unlink it, then free it */
  root->next=sym->next;
  free_symbol(sym);
}

SC_FUNC int get_actual_compound(symbol *sym)
{
  if (sym->ident == iARRAY || sym->ident == iREFARRAY) {
    while (sym->parent)
      sym = sym->parent;
  }

  return sym->compound;
}

SC_FUNC void delete_symbols(symbol *root,int level,int delete_labels,int delete_functions)
{
  symbol *origRoot=root;
  symbol *sym,*parent_sym;
  constvalue *stateptr;
  int mustdelete;

  /* erase only the symbols with a deeper nesting level than the
   * specified nesting level */
  while (root->next!=NULL) {
    sym=root->next;
    if (get_actual_compound(sym)<level)
      break;
    switch (sym->ident) {
    case iLABEL:
      mustdelete=delete_labels;
      break;
    case iVARIABLE:
    case iARRAY:
      /* do not delete global variables if functions are preserved */
      mustdelete=delete_functions;
      break;
    case iREFERENCE:
      /* always delete references (only exist as function parameters) */
      mustdelete=TRUE;
      break;
    case iREFARRAY:
      /* a global iREFARRAY symbol is the return value of a function: delete
       * this only if "globals" must be deleted; other iREFARRAY instances
       * (locals) are also deleted
       */
      mustdelete=delete_functions;
      for (parent_sym=sym->parent; parent_sym!=NULL && parent_sym->ident!=iFUNCTN; parent_sym=parent_sym->parent)
        assert(parent_sym->ident==iREFARRAY);
      assert(parent_sym==NULL || (parent_sym->ident==iFUNCTN && parent_sym->parent==NULL));
      if (parent_sym==NULL || parent_sym->ident!=iFUNCTN)
        mustdelete=TRUE;
      break;
    case iCONSTEXPR:
      /* delete constants, except predefined constants */
      mustdelete=delete_functions || (sym->usage & uPREDEF)==0;
      break;
    case iFUNCTN:
      /* optionally preserve globals (variables & functions), but
       * NOT native functions
       */
      mustdelete=delete_functions || (sym->usage & uNATIVE)!=0;
      assert(sym->parent==NULL);
      break;
    case iARRAYCELL:
    case iARRAYCHAR:
    case iEXPRESSION:
    case iVARARGS:
    default:
      assert(0);
      break;
    } /* switch */
    if (mustdelete) {
      if (origRoot == &glbtab)
        RemoveFromHashTable(sp_Globals, sym);
      root->next=sym->next;
      free_symbol(sym);
    } else {
      /* if the function was prototyped, but not implemented in this source,
       * mark it as such, so that its use can be flagged
       */
      if (sym->ident==iFUNCTN && (sym->usage & uDEFINE)==0)
        sym->usage |= uMISSING;
      if (sym->ident==iFUNCTN || sym->ident==iVARIABLE || sym->ident==iARRAY)
        sym->usage &= ~uDEFINE; /* clear "defined" flag */
      /* set all states as "undefined" too */
      if (sym->states!=NULL)
        for (stateptr=sym->states->next; stateptr!=NULL; stateptr=stateptr->next)
          stateptr->value=0;
      /* for user defined operators, also remove the "prototyped" flag, as
       * user-defined operators *must* be declared before use
       */
      if (sym->ident==iFUNCTN && !alpha(*sym->name))
        sym->usage &= ~uPROTOTYPED;
      root=sym;                 /* skip the symbol */
    } /* if */
  } /* if */
}

static symbol *find_symbol(const symbol *root,const char *name,int fnumber,int automaton,int *cmptag)
{
  symbol *firstmatch=NULL;
  symbol *sym=root->next;
  int count=0;
  unsigned long hash=NameHash(name);
  while (sym!=NULL) {
    if (hash==sym->hash && strcmp(name,sym->name)==0        /* check name */
        && (sym->parent==NULL || sym->ident==iCONSTEXPR)    /* sub-types (hierarchical types) are skipped, except for enum fields */
        && (sym->fnumber<0 || sym->fnumber==fnumber))       /* check file number for scope */
    {
      assert(sym->states==NULL || sym->states->next!=NULL); /* first element of the state list is the "root" */
      if (sym->ident==iFUNCTN
          || (automaton<0 && sym->states==NULL)
          || (automaton>=0 && sym->states!=NULL && state_getfsa(sym->states->next->index)==automaton))
      {
        if (cmptag==NULL)
          return sym;   /* return first match */
        /* return closest match or first match; count number of matches */
        if (firstmatch==NULL)
          firstmatch=sym;
        assert(cmptag!=NULL);
        if (*cmptag==0)
          count++;
        if (*cmptag==sym->tag) {
          *cmptag=1;    /* good match found, set number of matches to 1 */
          return sym;
        } /* if */
      } /* if */
    } /*  */
    sym=sym->next;
  } /* while */
  if (cmptag!=NULL && firstmatch!=NULL)
    *cmptag=count;
  return firstmatch;
}

static symbol *find_symbol_child(const symbol *root,const symbol *sym)
{
  symbol *ptr=root->next;
  while (ptr!=NULL) {
    if (ptr->parent==sym)
      return ptr;
    ptr=ptr->next;
  } /* while */
  return NULL;
}

/* Adds "bywhom" to the list of referrers of "entry". Typically,
 * bywhom will be the function that uses a variable or that calls
 * the function.
 */
SC_FUNC int refer_symbol(symbol *entry,symbol *bywhom)
{
  int count;

  assert(bywhom!=NULL);         /* it makes no sense to add a "void" referrer */
  assert(entry!=NULL);
  assert(entry->refer!=NULL);

  /* see if it is already there */
  for (count=0; count<entry->numrefers && entry->refer[count]!=bywhom; count++)
    /* nothing */;
  if (count<entry->numrefers) {
    assert(entry->refer[count]==bywhom);
    return TRUE;
  } /* if */

  /* see if there is an empty spot in the referrer list */
  for (count=0; count<entry->numrefers && entry->refer[count]!=NULL; count++)
    /* nothing */;
  assert(count <= entry->numrefers);
  if (count==entry->numrefers) {
    symbol **refer;
    int newsize=2*entry->numrefers;
    assert(newsize>0);
    /* grow the referrer list */
    refer=(symbol**)realloc(entry->refer,newsize*sizeof(symbol*));
    if (refer==NULL)
      return FALSE;             /* insufficient memory */
    /* initialize the new entries */
    entry->refer=refer;
    for (count=entry->numrefers; count<newsize; count++)
      entry->refer[count]=NULL;
    count=entry->numrefers;     /* first empty spot */
    entry->numrefers=newsize;
  } /* if */

  /* add the referrer */
  assert(entry->refer[count]==NULL);
  entry->refer[count]=bywhom;
  return TRUE;
}

SC_FUNC void markusage(symbol *sym,int usage)
{
  assert(sym!=NULL);
  sym->usage |= (char)usage;
  if ((usage & uWRITTEN)!=0)
    sym->lnumber=fline;
  /* check if (global) reference must be added to the symbol */
  if ((usage & (uREAD | uWRITTEN))!=0) {
    /* only do this for global symbols */
    if (sym->vclass==sGLOBAL) {
      /* "curfunc" should always be valid, since statements may not occurs
       * outside functions; in the case of syntax errors, however, the
       * compiler may arrive through this function
       */
      if (curfunc!=NULL)
        refer_symbol(sym,curfunc);
    } /* if */
  } /* if */
}


/*  findglb
 *
 *  Returns a pointer to the global symbol (if found) or NULL (if not found)
 */
SC_FUNC symbol *findglb(const char *name,int filter)
{
  /* find a symbol with a matching automaton first */
  symbol *sym=NULL;

  if (filter>sGLOBAL && sc_curstates>0) {
    /* find a symbol whose state list matches the current fsa */
    sym=find_symbol(&glbtab,name,fcurrent,state_getfsa(sc_curstates),NULL);
    if (sym!=NULL && sym->ident!=iFUNCTN) {
      /* if sym!=NULL, we found a variable in the automaton; now we should
       * also verify whether there is an intersection between the symbol's
       * state list and the current state list
       */
      assert(sym->states!=NULL && sym->states->next!=NULL);
      if (!state_conflict_id(sc_curstates,sym->states->next->index))
        sym=NULL;
    } /* if */
  } /* if */

  /* if no symbol with a matching automaton exists, find a variable/function
   * that has no state(s) attached to it
   */
  if (sym==NULL)
    sym=FindInHashTable(sp_Globals,name,fcurrent);
  return sym;
}

/*  findloc
 *
 *  Returns a pointer to the local symbol (if found) or NULL (if not found).
 *  See add_symbol() how the deepest nesting level is searched first.
 */
SC_FUNC symbol *findloc(const char *name)
{
  return find_symbol(&loctab,name,-1,-1,NULL);
}

SC_FUNC symbol *findconst(const char *name,int *cmptag)
{
  symbol *sym;

  sym=find_symbol(&loctab,name,-1,-1,cmptag);  /* try local symbols first */
  if (sym==NULL || sym->ident!=iCONSTEXPR) {   /* not found, or not a constant */
    if (cmptag)
      sym=FindTaggedInHashTable(sp_Globals,name,fcurrent,cmptag);
    else
      sym=FindInHashTable(sp_Globals,name,fcurrent);
  }
  if (sym==NULL || sym->ident!=iCONSTEXPR)
    return NULL;
  assert(sym->parent==NULL || (sym->usage & uENUMFIELD)!=0);
  /* ^^^ constants have no hierarchy, but enumeration fields may have a parent */
  return sym;
}

SC_FUNC symbol *finddepend(const symbol *parent)
{
  symbol *sym;

  sym=find_symbol_child(&loctab,parent);    /* try local symbols first */
  if (sym==NULL)                            /* not found */
    sym=find_symbol_child(&glbtab,parent);
  return sym;
}

/*  addsym
 *
 *  Adds a symbol to the symbol table (either global or local variables,
 *  or global and local constants).
 */
SC_FUNC symbol *addsym(const char *name,cell addr,int ident,int vclass,int tag,int usage)
{
  symbol entry, **refer;

  /* labels may only be defined once */
  assert(ident!=iLABEL || findloc(name)==NULL);

  /* create an empty referrer list */
  if ((refer=(symbol**)malloc(sizeof(symbol*)))==NULL) {
    error(123);         /* insufficient memory */
    return NULL;
  } /* if */
  *refer=NULL;

  /* first fill in the entry */
  memset(&entry,0,sizeof entry);
  strcpy(entry.name,name);
  entry.hash=NameHash(name);
  entry.addr=addr;
  entry.codeaddr=code_idx;
  entry.vclass=(char)vclass;
  entry.ident=(char)ident;
  entry.tag=tag;
  entry.usage=(char)usage;
  entry.fnumber=-1;     /* assume global visibility (ignored for local symbols) */
  entry.lnumber=fline;
  entry.numrefers=1;
  entry.refer=refer;

  /* then insert it in the list */
  if (vclass==sGLOBAL)
    return add_symbol(&glbtab,&entry,TRUE);
  return add_symbol(&loctab,&entry,FALSE);
}

SC_FUNC symbol *addvariable(const char *name,cell addr,int ident,int vclass,int tag,
                            int dim[],int numdim,int idxtag[])
{
  return addvariable2(name,addr,ident,vclass,tag,dim,numdim,idxtag,0);
}

SC_FUNC symbol *addvariable2(const char *name,cell addr,int ident,int vclass,int tag,
                            int dim[],int numdim,int idxtag[],int slength)
{
  symbol *sym;

  /* global variables may only be defined once
   * One complication is that functions returning arrays declare an array
   * with the same name as the function, so the assertion must allow for
   * this special case. Another complication is that variables may be
   * "redeclared" if they are local to an automaton (and findglb() will find
   * the symbol without states if no symbol with states exists).
   */
  assert(vclass!=sGLOBAL || (sym=findglb(name,sGLOBAL))==NULL || (sym->usage & uDEFINE)==0
         || (sym->ident==iFUNCTN && sym==curfunc)
         || (sym->states==NULL && sc_curstates>0));

  if (ident==iARRAY || ident==iREFARRAY) {
    symbol *parent=NULL,*top;
    int level;
    sym=NULL;                   /* to avoid a compiler warning */
    for (level=0; level<numdim; level++) {
      top=addsym(name,addr,ident,vclass,tag,uDEFINE);
      top->dim.array.length=dim[level];
      if (tag == pc_tag_string && level == numdim - 1) {
        if (slength == 0)
          top->dim.array.length=dim[level] * sizeof(cell);
        else
          top->dim.array.slength=slength;
      } else {
        top->dim.array.slength=0;
      }
      top->dim.array.level=(short)(numdim-level-1);
      top->x.tags.index=idxtag[level];
      top->parent=parent;
      parent=top;
      if (level==0)
        sym=top;
    } /* for */
  } else {
    sym=addsym(name,addr,ident,vclass,tag,uDEFINE);
  } /* if */
  return sym;
}

/*  getlabel
 *
 *  Returns te next internal label number. The global variable sc_labnum is
 *  initialized to zero.
 */
SC_FUNC int getlabel(void)
{
  return sc_labnum++;
}

/*  itoh
 *
 *  Converts a number to a hexadecimal string and returns a pointer to that
 *  string. This function is NOT re-entrant.
 */
SC_FUNC char *itoh(ucell val)
{
static char itohstr[30];
  char *ptr;
  int i,nibble[16];             /* a 64-bit hexadecimal cell has 16 nibbles */
  int max;

  #if PAWN_CELL_SIZE==16
    max=4;
  #elif PAWN_CELL_SIZE==32
    max=8;
  #elif PAWN_CELL_SIZE==64
    max=16;
  #else
    #error Unsupported cell size
  #endif
  ptr=itohstr;
  for (i=0; i<max; i+=1){
    nibble[i]=(int)(val & 0x0f);        /* nibble 0 is lowest nibble */
    val>>=4;
  } /* endfor */
  i=max-1;
  while (nibble[i]==0 && i>0)   /* search for highest non-zero nibble */
    i-=1;
  while (i>=0){
    if (nibble[i]>=10)
      *ptr++=(char)('a'+(nibble[i]-10));
    else
      *ptr++=(char)('0'+nibble[i]);
    i-=1;
  } /* while */
  *ptr='\0';            /* and a zero-terminator */
  return itohstr;
}

