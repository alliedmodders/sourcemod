/*  Codepage translation to Unicode, and UTF-8 support
 *
 *  The translation is based on codepage mapping files that are distributed
 *  by the Unicode consortium, see ftp://ftp.unicode.org/Public/MAPPINGS/.
 *
 *  Character sets with a maximum of 256 codes are translated via a lookup
 *  table (these are Single-Byte Character Sets). Character sets like Shift-JIS
 *  with single-byte characters and multi-byte characters (introduced by a
 *  leader byte) are split into two tables: the 256-entry lookup table for
 *  the single-byte characters and an extended table for the multi-byte
 *  characters. The extended table is allocated dynamically; the lookup table
 *  is allocated statically, so loading SBCS tables cannot fail (if the tables
 *  themselves are valid, of course).
 *
 *  Copyright (c) ITB CompuPhase, 2004-2006
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
 *  Version: $Id: sci18n.c 3612 2006-07-22 09:59:46Z thiadmer $
 */
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sc.h"

#if !defined TRUE
  #define FALSE         0
  #define TRUE          1
#endif
#if !defined _MAX_PATH
  #define _MAX_PATH     250
#endif
#if !defined DIRSEP_CHAR
  #if defined LINUX || defined __FreeBSD__ || defined __OpenBSD__
    #define DIRSEP_CHAR '/'
  #elif defined macintosh
    #define DIRSEP_CHAR ':'
  #else
    #define DIRSEP_CHAR '\\'
  #endif
#endif

#if !defined ELEMENTS
  #define ELEMENTS(array)       (sizeof(array) / sizeof(array[0]))
#endif

#if !defined NO_CODEPAGE

#if !defined MAXCODEPAGE
  #define MAXCODEPAGE   12      /* typically "cp" + 4 digits + ".txt" */
#endif
#define INVALID         0xffffu /* 0xffff and 0xfffe are invalid Unicode characters */
#define LEADBYTE        0xfffeu

struct wordpair {
  unsigned short index;
  wchar_t code;
};
static char cprootpath[_MAX_PATH] = { DIRSEP_CHAR, '\0' };
static wchar_t bytetable[256];
static struct wordpair *wordtable = NULL;
static unsigned wordtablesize = 0;
static unsigned wordtabletop = 0;


/* read in a line delimited by '\r' or '\n'; do NOT store the '\r' or '\n' into
 * the string and ignore empty lines
 * returns 1 for success and 0 for failure
 */
static int cp_readline(FILE *fp,char *string,size_t size)
{
  size_t count=0;
  int c;
  assert(size>1);
  while ((c=fgetc(fp))!=EOF && count<size-1) {
    if (c=='\r' || c=='\n') {
      if (count>0)  /* '\r' or '\n' ends a string */
        break;
      /* if count==0, the line started with a '\r' or '\n', or perhaps line
       * ends in the file are '\r\n' and we read and stopped on the '\r' of
       * the preceding line
       */
    } else {
      string[count++]=(char)c;
    } /* if */
  } /* while */
  string[count]='\0';
  return count>0;
}

/* cp_path() sets the directory where all codepage files must be found (if
 * the parameter to cp_set() specifies a full path, that is used instead).
 * The path is specified into two parts: root and directory; the full path
 * for the codepage direcory is just the concatenation of the two, with a
 * directory separator in between. The directory is given in two parts,
 * because often a program already retrieves its "home" directory and the
 * codepages are most conveniently stored in a subdirectory of this home
 * directory.
 */
SC_FUNC int cp_path(const char *root, const char *directory)
{
  size_t len1,len2;
  int add_slash1,add_slash2;

  len1= (root!=NULL) ? strlen(root) : 0;
  add_slash1= (len1==0 || root[len1-1]!=DIRSEP_CHAR);
  len2= (directory!=NULL) ? strlen(directory) : 0;
  add_slash2= (len2>0 && root[len2-1]!=DIRSEP_CHAR);
  if (len1+add_slash1+len2+add_slash2>=(_MAX_PATH-MAXCODEPAGE))
    return FALSE;       /* full filename may not fit */
  if (root!=NULL)
    strcpy(cprootpath,root);
  if (add_slash1) {
    assert(len1==0 || cprootpath[len1]=='\0');
    cprootpath[len1]=DIRSEP_CHAR;
    cprootpath[len1+1]='\0';
  } /* if */
  if (directory!=NULL)
    strcat(cprootpath,directory);
  if (add_slash2) {
    assert(cprootpath[len1+add_slash1+len2]=='\0');
    cprootpath[len1+add_slash1+len2]=DIRSEP_CHAR;
    cprootpath[len1+add_slash1+len2+1]='\0';
  } /* if */
  cp_set(NULL);         /* start with a "linear" table (no translation) */
  return TRUE;
}

/* cp_set() loads a codepage from a file. The name parameter may be a
 * filename (including a full path) or it may be a partial codepage name.
 * If the name parameter is NULL, the codepage is cleared to be a "linear"
 * table (no translation).
 * The following files are attempted to open (where <name> specifies the
 * value of the parameter):
 *    <name>
 *    <cprootpath>/<name>
 *    <cprootpath>/<name>.txt
 *    <cprootpath>/cp<name>
 *    <cprootpath>/cp<name>.txt
 */
SC_FUNC int cp_set(const char *name)
{
  char filename[_MAX_PATH];
  FILE *fp=NULL;
  unsigned index;

  /* for name==NULL, set up an identity table */
  if (name==NULL || *name=='\0') {
    if (wordtable!=NULL) {
      free(wordtable);
      wordtable=NULL;
      wordtablesize=0;
      wordtabletop=0;
    } /* if */
    for (index=0; index<ELEMENTS(bytetable); index++)
      bytetable[index]=(wchar_t)index;
    return TRUE;
  } /* if */

  /* try to open the file as-is */
  if (strchr(name,DIRSEP_CHAR)!=NULL)
    fp=fopen(name,"rt");
  if (fp==NULL) {
    /* try opening the file in the "root path" for codepages */
    if (strlen(name)>MAXCODEPAGE)
      return 0;
    assert(strlen(name)+strlen(cprootpath)<_MAX_PATH);
    strcpy(filename,cprootpath);
    strcat(filename,name);
    fp=fopen(filename,"rt");
  } /* if */
  if (fp==NULL) {
    /* try opening the file in the "root path" for codepages, with a ".txt" extension */
    if (strlen(name)+4>=MAXCODEPAGE)
      return 0;
    assert(strlen(filename)+4<_MAX_PATH);
    strcat(filename,".txt");
    fp=fopen(filename,"rt");
  } /* if */
  if (fp==NULL) {
    /* try opening the file in the "root path" for codepages, with "cp" prefixed before the name */
    if (strlen(name)+2>MAXCODEPAGE)
      return 0;
    assert(2+strlen(name)+strlen(cprootpath)<_MAX_PATH);
    strcpy(filename,cprootpath);
    strcat(filename,"cp");
    strcat(filename,name);
    fp=fopen(filename,"rt");
  } /* if */
  if (fp==NULL) {
    /* try opening the file in the "root path" for codepages, with "cp" prefixed an ".txt" appended */
    if (strlen(name)+2+4>MAXCODEPAGE)
      return 0;
    assert(strlen(filename)+4<_MAX_PATH);
    strcat(filename,".txt");
    fp=fopen(filename,"rt");
  } /* if */
  if (fp==NULL)
    return FALSE;       /* all failed */

  /* clear the tables */
  for (index=0; index<ELEMENTS(bytetable); index++)
    bytetable[index]=INVALID;   /* special code meaning "not found" */
  assert(wordtablesize==0 && wordtabletop==0 && wordtable==NULL
         || wordtablesize>0 && wordtable!=NULL);
  if (wordtable!=NULL) {
    free(wordtable);
    wordtable=NULL;
    wordtablesize=0;
    wordtabletop=0;
  } /* if */

  /* read in the table */
  while (cp_readline(fp,filename,sizeof filename)) {
    char *ptr;
    if ((ptr=strchr(filename,'#'))!=NULL)
      *ptr='\0';                /* strip of comment */
    for (ptr=filename; *ptr>0 && *ptr<' '; ptr++)
      /* nothing */;            /* skip leading whitespace */
    if (*ptr!='\0') {
      /* content on line */
      unsigned code=LEADBYTE;
      int num=sscanf(ptr,"%i %i",&index,&code);
      /* if sscanf() returns 1 and the index is in range 0..255, then the
       * code is a DBCS lead byte; if sscanf() returns 2 and index>=256, this
       * is a double byte pair (lead byte + follower)
       */
      if (num>=1 && index<256) {
        bytetable[index]=(wchar_t)code;
      } else if (num==2 && index>=256 && index<LEADBYTE) {
        /* store the DBCS character in wordtable */
        if (wordtabletop>=wordtablesize) {
          /* grow the list */
          int newsize;
          struct wordpair *newblock;
          newsize= (wordtablesize==0) ? 128 : 2*wordtablesize;
          newblock=(struct wordpair *)malloc(newsize*sizeof(*wordtable));
          if (newblock!=NULL) {
            memcpy(newblock,wordtable,wordtabletop*sizeof(*wordtable));
            free(wordtable);
            wordtable=newblock;
            wordtablesize=newsize;
          } /* if */
        } /* if */
        if (wordtabletop<wordtablesize) {
          /* insert at sorted position */
          int pos=wordtabletop;
          assert(wordtable!=NULL);
          while (pos>0 && (unsigned)wordtable[pos-1].index>index) {
            wordtable[pos]=wordtable[pos-1];
            pos--;
          } /* while */
          wordtable[pos].index=(unsigned short)index;
          wordtable[pos].code=(wchar_t)code;
        } /* if */
      } /* if */
    } /* if */
  } /* while */

  fclose(fp);
  return TRUE;
}

SC_FUNC cell cp_translate(const unsigned char *string,const unsigned char **endptr)
{
  wchar_t result;

  result=bytetable[*string++];
  /* check whether this is a leader code */
  if ((unsigned)result==LEADBYTE && wordtable!=NULL) {
    /* look up the code via binary search */
    int low,high,mid;
    unsigned short index=(unsigned short)(((*(string-1)) << 8) | *string);
    string++;
    assert(wordtabletop>0);
    low=0;
    high=wordtabletop-1;
    while (low<high) {
      mid=(low+high)/2;
      assert(low<=mid && mid<high);
      if (index>wordtable[mid].index)
        low=mid+1;
      else
        high=mid;
    } /* while */
    assert(low==high);
    if (wordtable[low].index==index)
      result=wordtable[low].code;
  } /* if */

  if (endptr!=NULL)
    *endptr=string;
  return (cell)result;
}

#endif  /* NO_CODEPAGE */

#if !defined NO_UTF8
SC_FUNC cell get_utf8_char(const unsigned char *string,const unsigned char **endptr)
{
  int follow=0;
  long lowmark=0;
  unsigned char ch;
  cell result=0;

  if (endptr!=NULL)
    *endptr=string;

  for ( ;; ) {
    ch=*string++;

    if (follow>0 && (ch & 0xc0)==0x80) {
      /* leader code is active, combine with earlier code */
      result=(result << 6) | (ch & 0x3f);
      if (--follow==0) {
        /* encoding a character in more bytes than is strictly needed,
         * is not really valid UTF-8; we are strict here to increase
         * the chance of heuristic dectection of non-UTF-8 text
         * (JAVA writes zero bytes as a 2-byte code UTF-8, which is invalid)
         */
        if (result<lowmark)
          return -1;
        /* the code positions 0xd800--0xdfff and 0xfffe & 0xffff do not
         * exist in UCS-4 (and hence, they do not exist in Unicode)
         */
        if (result>=0xd800 && result<=0xdfff || result==0xfffe || result==0xffff)
          return -1;
      } /* if */
      break;
    } else if (follow==0 && (ch & 0x80)==0x80) {
      /* UTF-8 leader code */
      if ((ch & 0xe0)==0xc0) {
        /* 110xxxxx 10xxxxxx */
        follow=1;
        lowmark=0x80L;
        result=ch & 0x1f;
      } else if ((ch & 0xf0)==0xe0) {
        /* 1110xxxx 10xxxxxx 10xxxxxx (16 bits, BMP plane) */
        follow=2;
        lowmark=0x800L;
        result=ch & 0x0f;
      } else if ((ch & 0xf8)==0xf0) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        follow=3;
        lowmark=0x10000L;
        result=ch & 0x07;
      } else if ((ch & 0xfc)==0xf8) {
        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        follow=4;
        lowmark=0x200000L;
        result=ch & 0x03;
      } else if ((ch & 0xfe)==0xfc) {
        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (32 bits) */
        follow=5;
        lowmark=0x4000000L;
        result=ch & 0x01;
      } else {
        /* this is invalid UTF-8 */
        return -1;
      } /* if */
    } else if (follow==0 && (ch & 0x80)==0x00) {
      /* 0xxxxxxx (US-ASCII) */
      result=ch;
      break;
    } else {
      /* this is invalid UTF-8 */
      return -1;
    } /* if */

  } /* for */

  if (endptr!=NULL)
    *endptr=string;
  return result;
}
#endif

SC_FUNC int scan_utf8(FILE *fp,const char *filename)
{
  #if defined NO_UTF8
    return 0;
  #else
    void *resetpos=pc_getpossrc(fp);
    int utf8=TRUE;
    int firstchar=TRUE,bom_found=FALSE;
    const unsigned char *ptr;

    while (utf8 && pc_readsrc(fp,pline,sLINEMAX)!=NULL) {
      ptr=pline;
      if (firstchar) {
        /* check whether the very first character on the very first line
         * starts with a BYTE order mark
         */
        cell c=get_utf8_char(ptr,&ptr);
        bom_found= (c==0xfeff);
        utf8= (c>=0);
        firstchar=FALSE;
      } /* if */
      while (utf8 && *ptr!='\0')
        utf8= (get_utf8_char(ptr,&ptr)>=0);
    } /* while */
    pc_resetsrc(fp,resetpos);
    if (bom_found) {
      unsigned char bom[3];
      if (!utf8)
        error(77,filename);     /* malformed UTF-8 encoding */
      pc_readsrc(fp,bom,3);
      assert(bom[0]==0xef && bom[1]==0xbb && bom[2]==0xbf);
    } /* if */
    return utf8;
  #endif  /* NO_UTF8 */
}
