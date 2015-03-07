// vim: set ts=8 sts=2 sw=2 tw=99 et:
/*  Pawn compiler - Error message system
 *  In fact a very simple system, using only 'panic mode'.
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
#if defined	__WIN32__ || defined _WIN32 || defined __MSDOS__
  #include <io.h>
#endif
#if defined LINUX || defined __GNUC__
  #include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>     /* ANSI standardized variable argument list functions */
#include <string.h>
#if defined FORTIFY
  #include <alloc/fortify.h>
#endif
#include "sc.h"

#if defined _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4125)  /* decimal digit terminates octal escape sequence */
#endif

#include "sc5-in.scp"

#if defined _MSC_VER
  #pragma warning(pop)
#endif

#define NUM_WARNINGS    (int)(sizeof warnmsg / sizeof warnmsg[0])
static unsigned char warndisable[(NUM_WARNINGS + 7) / 8]; /* 8 flags in a char */

static int errflag;
static int errstart;     /* line number at which the instruction started */
static int sErrLine;     /* forced line number for the error message */

/*  error
 *
 *  Outputs an error message (note: msg is passed optionally).
 *  If an error is found, the variable "errflag" is set and subsequent
 *  errors are ignored until lex() finds a semicolumn or a keyword
 *  (lex() resets "errflag" in that case).
 *
 *  Global references: inpfname   (reffered to only)
 *                     fline      (reffered to only)
 *                     fcurrent   (reffered to only)
 *                     errflag    (altered)
 */
int error(int number,...)
{
static const char *prefix[3]={ "error", "fatal error", "warning" };
static int lastline,errorcount;
static short lastfile;
  const char *msg,*pre;
  va_list argptr;

  // sErrLine is used to temporarily change the line number of reported errors.
  // Pawn has an upstream bug where this is not reset on early-return, which
  // can lead to broken line numbers in error messages.
  int errline = sErrLine;
  sErrLine = -1;

  bool is_warning = (number >= 200 && !sc_warnings_are_errors);

  /* errflag is reset on each semicolon.
   * In a two-pass compiler, an error should not be reported twice. Therefore
   * the error reporting is enabled only in the second pass (and only when
   * actually producing output). Fatal errors may never be ignored.
   */
  int not_fatal = (number < FIRST_FATAL_ERROR || number >= 200);
  if (errflag && not_fatal)
    return 0;
  if (sc_status != statWRITE && not_fatal) {
    if (!sc_err_status)
      return 0;
  }

  /* also check for disabled warnings */
  if (number>=200) {
    int index=(number-200)/8;
    int mask=1 << ((number-200)%8);
    if ((warndisable[index] & mask)!=0)
      return 0;
  } /* if */

  if (number<FIRST_FATAL_ERROR) {
    msg=errmsg[number-1];
    pre=prefix[0];
    errflag=TRUE;       /* set errflag (skip rest of erroneous expression) */
    errnum++;
  } else if (number<200){
    msg=fatalmsg[number-FIRST_FATAL_ERROR];
    pre=prefix[1];
    errnum++;           /* a fatal error also counts as an error */
  } else {
    msg=warnmsg[number-200];
    if (sc_warnings_are_errors) {
      pre=prefix[0];
      errnum++;
    } else {
      pre=prefix[2];
      warnnum++;
    }
  } /* if */

  assert(errstart<=fline);
  if (errline>0)
    errstart=errline;
  else
    errline=fline;
  assert(errstart<=errline);
  va_start(argptr,number);
  if (strlen(errfname)==0) {
    int start= (errstart==errline) ? -1 : errstart;
    if (pc_error(number,msg,inpfname,start,errline,argptr)) {
      if (outf!=NULL) {
        pc_closeasm(outf,TRUE);
        outf=NULL;
      } /* if */
      longjmp(errbuf,3);        /* user abort */
    } /* if */
  } else {
    FILE *fp=fopen(errfname,"a");
    if (fp!=NULL) {
      if (errstart>=0 && errstart!=errline)
        fprintf(fp,"%s(%d -- %d) : %s %03d: ",inpfname,errstart,errline,pre,number);
      else
        fprintf(fp,"%s(%d) : %s %03d: ",inpfname,errline,pre,number);
      vfprintf(fp,msg,argptr);
      fclose(fp);
    } /* if */
  } /* if */
  va_end(argptr);

  if ((number>=FIRST_FATAL_ERROR && number<200) || errnum>25){
    if (strlen(errfname)==0) {
      va_start(argptr,number);
      pc_error(0,"\nCompilation aborted.",NULL,0,0,argptr);
      va_end(argptr);
    } /* if */
    if (outf!=NULL) {
      pc_closeasm(outf,TRUE);
      outf=NULL;
    } /* if */
    longjmp(errbuf,2);          /* fatal error, quit */
  } /* if */

  /* check whether we are seeing many errors on the same line */
  if ((errstart<0 && lastline!=fline) || lastline<errstart || lastline>fline || fcurrent!=lastfile)
    errorcount=0;
  lastline=fline;
  lastfile=fcurrent;
  if (!is_warning)
    errorcount++;
  if (errorcount>=3)
    error(FATAL_ERROR_OVERWHELMED_BY_BAD);

  return 0;
}

void errorset(int code,int line)
{
  switch (code) {
  case sRESET:
    errflag=FALSE;      /* start reporting errors */
    break;
  case sFORCESET:
    errflag=TRUE;       /* stop reporting errors */
    break;
  case sEXPRMARK:
    errstart=fline;     /* save start line number */
    break;
  case sEXPRRELEASE:
    errstart=-1;        /* forget start line number */
    sErrLine=-1;
    break;
  case sSETPOS:
    sErrLine=line;
    break;
  } /* switch */
}

/* sc_enablewarning()
 * Enables or disables a warning (errors cannot be disabled).
 * Initially all warnings are enabled. The compiler does this by setting bits
 * for the *disabled* warnings and relying on the array to be zero-initialized.
 *
 * Parameter enable can be:
 *  o  0 for disable
 *  o  1 for enable
 *  o  2 for toggle
 */
int pc_enablewarning(int number,int enable)
{
  int index;
  unsigned char mask;

  if (number<200)
    return FALSE;       /* errors and fatal errors cannot be disabled */
  number -= 200;
  if (number>=NUM_WARNINGS)
    return FALSE;

  index=number/8;
  mask=(unsigned char)(1 << (number%8));
  switch (enable) {
  case 0:
    warndisable[index] |= mask;
    break;
  case 1:
    warndisable[index] &= (unsigned char)~mask;
    break;
  case 2:
    warndisable[index] ^= mask;
    break;
  } /* switch */

  return TRUE;
}

#undef SCPACK_TABLE
