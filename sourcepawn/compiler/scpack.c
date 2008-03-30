/* compress.c -- Byte Pair Encoding compression */
/* Copyright 1996 Philip Gage */

/* This program appeared in the September 1997 issue of
 * C/C++ Users Journal. The original source code may still
 * be found at the web site of the magazine (www.cuj.com).
 *
 * It has been modified by me (Thiadmer Riemersma) to
 * compress only a section of the input file and to store
 * the compressed output along with the input as "C" strings.
 *
 * Compiling instructions:
 *  Borland C++ 16-bit (large memory model is required):
 *      bcc -ml scpack.c
 *
 *  Watcom C/C++ 32-bit:
 *      wcl386 scpack.c
 *
 *  GNU C (Linux), 32-bit:
 *      gcc scpack.c -o scpack
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if UINT_MAX > 0xFFFFU
  #define MAXSIZE 1024*1024L
#else
  #define MAXSIZE UINT_MAX      /* Input file buffer size */
#endif
#define HASHSIZE 8192   /* Hash table size, power of 2 */
#define THRESHOLD   3   /* Increase for speed, min 3 */

#define START_TOKEN "#ifdef SCPACK" /* start reading the buffer here */
#define NAME_TOKEN  "#define SCPACK_TABLE"
#define SEP_TOKEN   "#define SCPACK_SEPARATOR"
#define TERM_TOKEN  "#define SCPACK_TERMINATOR"
#define TEMPFILE    "~SCPACK.TMP"
static char tablename[32+1] = "scpack_table";
static char separator[16]=",";
static char terminator[16]="";

int compress(unsigned char *buffer, unsigned buffersize, unsigned char pairtable[128][2])
{
  unsigned char *left, *right, *count;
  unsigned char a, b, bestcount;
  unsigned i, j, index, bestindex, code=128;

  /* Dynamically allocate buffers and check for errors */
  left = (unsigned char *)malloc(HASHSIZE);
  right = (unsigned char *)malloc(HASHSIZE);
  count = (unsigned char *)malloc(HASHSIZE);
  if (left==NULL || right==NULL || count==NULL) {
    printf("Error allocating memory\n");
    exit(1);
  }

  /* Check for errors */
  for (i=0; i<buffersize; i++)
    if (buffer[i] > 127) {
      printf("This program works only on text files (7-bit ASCII)\n");
      exit(1);
    }

  memset(pairtable, 0, 128*2*sizeof(char));

  do {  /* Replace frequent pairs with bytes 128..255 */

    /* Enter counts of all byte pairs into hash table */
    memset(count,0,HASHSIZE);
    for (i=0; i<buffersize-1; i++) {
      a = buffer[i];
      b = buffer[i+1];
      /* ignore any pair with a '\0' */
      if (a == 0 || b == 0)
        continue;
      index = (a ^ (b << 6)) & (HASHSIZE-1);
      while ((left[index] != a || right[index] != b) &&
             count[index] != 0)
        index = (index + 1) & (HASHSIZE-1);
      left[index] = a;
      right[index] = b;
      if (count[index] < 255)
        count[index] += (unsigned char)1;
    }

    /* Search hash table for most frequent pair */
    bestcount = THRESHOLD - 1;
    for (i=0; i<HASHSIZE; i++) {
      if (count[i] > bestcount) {
        bestcount = count[i];
        bestindex = i;
      }
    }

    /* Compress if enough occurrences of pair */
    if (bestcount >= THRESHOLD) {

      /* Add pair to table using code as index */
      a = pairtable[code-128][0] = left[bestindex];
      b = pairtable[code-128][1] = right[bestindex];

      /* Replace all pair occurrences with unused byte */
      for (i=0, j=0; i<buffersize; i++, j++)
        if (a == buffer[i] && b == buffer[i+1]) {
          buffer[j] = (unsigned char)code;
          ++i;
        }
        else
          buffer[j] = buffer[i];
      buffersize = j;
    }
    else
      break;
  } while (++code < 255);

  /* done */
  free(left); free(right); free(count);
  return buffersize;  /* return adjusted buffersize */
}

static int strmatch(char *str, char *token, int *indent)
{
  int i = 0;

  /* skip whitespace */
  while (*str==' ' || *str=='\t') {
    str++;
    i++;
  } /* while */
  if (strncmp(str,token,strlen(token))!=0)
    return 0;
  if (indent != NULL)
    *indent = i;
  return 1;
}

static void check_if(char *str,int linenr)
{
  if (strmatch(str,"#if",NULL)) {
    printf("Error: \"#if...\" preprocessor statement should not be in SCPACK section "
           "(line %d)\n", linenr);
    exit(1);
  } /* if */
}

static int check_tablename(char *str)
{
  int i;

  if (strmatch(str,NAME_TOKEN,NULL)) {
    str += strlen(NAME_TOKEN);
    while (*str==' ' || *str=='\t')
      str++;
    for (i=0; i<(sizeof tablename - 1) && *str!='\0' && strchr(" \t\n",*str)==NULL; i++, str++)
      tablename[i] = *str;
    tablename[i] = '\0';
    return 1;
  } /* if */
  return 0;
}

static int check_separator(char *str)
{
  int i;

  if (strmatch(str,SEP_TOKEN,NULL)) {
    str += strlen(SEP_TOKEN);
    while (*str==' ' || *str=='\t')
      str++;
    for (i=0; i<(sizeof separator - 1) && *str!='\0' && strchr(" \t\n",*str)==NULL; i++, str++)
      separator[i] = *str;
    separator[i] = '\0';
    return 1;
  } /* if */

  if (strmatch(str,TERM_TOKEN,NULL)) {
    str += strlen(TERM_TOKEN);
    while (*str==' ' || *str=='\t')
      str++;
    for (i=0; i<(sizeof terminator - 1) && *str!='\0' && strchr(" \t\n",*str)==NULL; i++, str++)
      terminator[i] = *str;
    terminator[i] = '\0';
    return 1;
  } /* if */

  return 0;
}

/* readbuffer
 * Reads in the input file and stores all strings in the
 * section between "#ifdef SCPACK" and "#else" in a buffer.
 * Only text that is between double quotes is added to the
 * buffer; the \" escape code is handled. Multiple strings
 * on one line are handled.
 */
unsigned readbuffer(FILE *input, unsigned char *buffer)
{
  char str[256];
  unsigned buffersize;
  int i,linenr;

  linenr=0;
  buffersize=0;

  rewind(input);
  while (!feof(input)) {
    while (fgets(str,sizeof str,input)!=NULL) {
      linenr++;
      check_tablename(str);
      check_separator(str);
      if (strmatch(str,START_TOKEN,NULL))
        break;
    } /* while */
    if (!strmatch(str,START_TOKEN,NULL))
      return buffersize;  /* no (more) section found, quit */

    while (fgets(str,sizeof str,input)!=NULL) {
      linenr++;
      check_if(str,linenr);
      if (check_tablename(str))
        printf("Error: table name definition should not be in SCPACK section (line %d)\n", linenr);
      check_separator(str);
      if (strmatch(str,"#else",NULL))
        break;          /* done */
      /* add to the buffer only what is between double quotes */
      i=0;
      do {
        while (str[i]!='\0' && str[i]!='"')
          i++;
        if (str[i]=='"') {
          /* we are in a string */
          i++;
          while (str[i]!='\0' && str[i]!='"') {
            /* handle escape sequences */
            if (str[i]=='\\') {
              i++;
              switch (str[i]) {
              case 'a': /* alarm */
                buffer[buffersize++]='\a';
                i++;
                break;
              case 'b': /* backspace */
                buffer[buffersize++]='\b';
                i++;
                break;
              case 'f': /* form feed */
                buffer[buffersize++]='\f';
                i++;
                break;
              case 'n': /* newline */
                buffer[buffersize++]='\n';
                i++;
                break;
              case 'r': /* carriage return */
                buffer[buffersize++]='\n';
                i++;
                break;
              case 't': /* tab */
                buffer[buffersize++]='\t';
                i++;
                break;
              case '\'':
                buffer[buffersize++]='\'';
                i++;
                break;
              case '"':
                buffer[buffersize++]='"';
                i++;
                break;
              default:
                // ??? octal character code escapes and hexadecimal escapes
                //     not supported
                printf("Unknown escape sequence '\\%c' on line %d\n",
                       str[i], linenr);
              } /* switch */
            } else {
              buffer[buffersize++]=str[i++];
            } /* if */
          } /* while */
          if (str[i]=='"') {
            buffer[buffersize++]='\0'; /* terminate each string */
            i++;
          } else {
            printf("Error: unterminated string on line %d\n",linenr);
          } /* if */
        } /* if */
      } while (str[i]!='\0');
    } /* while - in SCPACK section */
    /* put in another '\0' to terminate the section */
    buffer[buffersize++]='\0';
  } /* while - !feof(input) */
  return buffersize;
}

static void write_pairtable(FILE *output, unsigned char pairtable[128][2], char *tablename)
{
  int i;

  /* dump the pair table */
  fprintf(output, "/*-*SCPACK start of pair table, do not change or remove this line */\n");
  fprintf(output, "unsigned char %s[][2] = {", tablename);
  for (i=0; i<128 && pairtable[i][0]!=0 && pairtable[i][1]!=0; i++) {
    if ((i % 16)==0)
      fprintf(output, "\n  ");
    else
      fprintf(output, " ");
    fprintf(output, "{%d,%d}", pairtable[i][0], pairtable[i][1]);
    /* check if something follows this pair */
    if (i+1<128 && pairtable[i+1][0]!=0 && pairtable[i+1][1]!=0)
      fprintf(output, ",");
  } /* for */
  fprintf(output, "\n};\n");
  fprintf(output, "/*-*SCPACK end of pair table, do not change or remove this line */\n");
}

void writefile(FILE *input, FILE *output, unsigned char *buffer, unsigned buffersize, unsigned char pairtable[128][2])
{
  char str[256];
  int insection, indent, needseparator;
  unsigned char *bufptr;

  bufptr = buffer;
  insection = 0;

  rewind(input);
  while (!feof(input)) {
    while (fgets(str,sizeof str,input)!=NULL) {
      fprintf(output,"%s",str);
      if (check_tablename(str)) {
        write_pairtable(output, pairtable, tablename);
        /* strip an existing pair table from the file */
        if (fgets(str,sizeof str,input)!=NULL) {
          if (strmatch(str,"/*-*SCPACK",NULL)) {
            while (fgets(str,sizeof str,input)!=NULL)
              if (strmatch(str,"/*-*SCPACK",NULL))
                break;
          } else {
            fprintf(output,"%s",str);
          } /* if */
        } /* if */
      } /* if */
      if (strmatch(str,START_TOKEN,NULL))
        insection = 1;
      if (insection && strmatch(str,"#else",NULL))
        break;
    } /* while */
    if (!strmatch(str,"#else",&indent))
      return;           /* no (more) section found, quit */
    insection=0;

    /* dump the buffer as strings, separated with commas */
    needseparator = 0;
    while (*bufptr != '\0') {
      assert((unsigned)(bufptr-buffer) < buffersize);
      if (needseparator)
        fprintf(output, "%s\n",separator);
      fprintf(output, "%*c\"",indent+2,' ');
      /* loop over string */
      while (*bufptr != '\0') {
        if (*bufptr<' ' || *bufptr >= 128 || *bufptr == '"' || *bufptr == '\\')
          fprintf(output, "\\%03o", *bufptr);
        else
          fprintf(output, "%c", *bufptr);
        bufptr++;
      } /* while */
      fprintf(output, "\"");
      needseparator = 1;
      bufptr++;           /* skip '\0' */
    } /* while */
    fprintf(output, "%s\n",terminator);
    bufptr++;

    /* skip the input file until the #endif section */
    while (fgets(str,sizeof str,input)!=NULL) {
      if (strmatch(str,"#endif",NULL)) {
        fprintf(output,"%s",str);
        break;          /* done */
      } /* if */
    } /* while */
  } /* while - !feof(input) */
}

static void usage(void)
{
  printf("Usage: scpack <filename> [output file]\n");
  exit(1);
}

int main(int argc, char **argv)
{
  FILE *in, *out;
  unsigned char *buffer;
  unsigned buffersize, orgbuffersize;
  unsigned char pairtable[128][2];

  if (argc < 2 || argc > 3)
    usage();
  if ((in=fopen(argv[1],"rt"))==NULL) {
    printf("SCPACK: error opening input %s\n",argv[1]);
    usage();
  } /* if */
  if (argc == 2) {
    if ((out=fopen(TEMPFILE,"wt"))==NULL) {
      printf("SCPACK: error opening temporary file %s\n",TEMPFILE);
      usage();
    } /* if */
  } else {
    if ((out=fopen(argv[2],"wt"))==NULL) {
      printf("SCPACK: error opening output file %s\n",argv[2]);
      usage();
    } /* if */
  } /* if */

  buffer = (unsigned char *)malloc(MAXSIZE);
  if (buffer == NULL) {
    printf("SCPACK: error allocating memory\n");
    return 1;
  } /* if */
  /* 1. read the buffer
   * 2. compress the buffer
   * 3. copy the file, insert the compressed buffer
   */
  buffersize = readbuffer(in, buffer);
  orgbuffersize = buffersize;
  if (buffersize > 0) {
    buffersize = compress(buffer, buffersize, pairtable);
    writefile(in, out, buffer, buffersize, pairtable);
    printf("SCPACK: compression ratio: %ld%% (%d -> %d)\n",
           100L-(100L*buffersize)/orgbuffersize, orgbuffersize, buffersize);
  } else {
    printf("SCPACK: no SCPACK section found, nothing to do\n");
  } /* if */
  fclose(out);
  fclose(in);
  /* let the new file replace the old file */
  if (buffersize == 0) {
    if (argc == 2)
      remove(TEMPFILE);
    else
      remove(argv[2]);
  } else if (argc == 2) {
    remove(argv[1]);
    rename(TEMPFILE,argv[1]);
  } /* if */
  return 0;
}
