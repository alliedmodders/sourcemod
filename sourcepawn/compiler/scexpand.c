/* expand.c -- Byte Pair Encoding decompression */
/* Copyright 1996 Philip Gage */

/* Byte Pair Compression appeared in the September 1997
 * issue of C/C++ Users Journal. The original source code
 * may still be found at the web site of the magazine
 * (www.cuj.com).
 *
 * The decompressor has been modified by me (Thiadmer
 * Riemersma) to accept a string as input, instead of a
 * complete file.
 */
#include <assert.h>
#include <stdio.h>
#include "sc.h"

#define STACKSIZE 16

SC_FUNC int strexpand(char *dest, unsigned char *source, int maxlen, unsigned char pairtable[128][2])
{
  unsigned char stack[STACKSIZE];
  short c, top = 0;
  int len;

  assert(maxlen > 0);
  len = 1;              /* already 1 byte for '\0' */
  for (;;) {

    /* Pop byte from stack or read byte from the input string */
    if (top)
      c = stack[--top];
    else if ((c = *(unsigned char *)source++) == '\0')
      break;

    /* Push pair on stack or output byte to the output string */
    if (c > 127) {
      assert(top+2 <= STACKSIZE);
      stack[top++] = pairtable[c-128][1];
      stack[top++] = pairtable[c-128][0];
    }
    else {
      len++;
      if (maxlen > 1) { /* reserve one byte for the '\0' */
        *dest++ = (char)c;
        maxlen--;
      }
    }
  }
  *dest = '\0';
  return len;           /* return number of bytes decoded */
}

#if 0 /*for testing*/
#include "sc5.scp"

int main (int argc, char **argv)
{
  int i;
  char str[128];

  for (i=0; i<58; i++) {
    strexpand(str, errmsg[i], sizeof str, SCPACK_TABLE);
    printf("%s", str);
  } /* for */
  return 0;
}
#endif

