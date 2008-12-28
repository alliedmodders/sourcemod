/* __MSDOS__    set when compiling for DOS (not Windows)
 * _Windows     set when compiling for any version of Microsoft Windows
 * __WIN32__    set when compiling for Windows95 or WindowsNT (32 bit mode)
 * __32BIT__    set when compiling in 32-bit "flat" mode (DOS or Windows)
 *
 * Copyright 1998-2005, ITB CompuPhase, The Netherlands.
 * info@compuphase.com.
 */

#ifndef _OSDEFS_H
#define _OSDEFS_H

/* Every compiler uses different "default" macros to indicate the mode
 * it is in. Throughout the source, we use the Borland C++ macros, so
 * the macros of Watcom C/C++ and Microsoft Visual C/C++ are mapped to
 * those of Borland C++.
 */
#if defined(__WATCOMC__)
#  if defined(__WINDOWS__) || defined(__NT__)
#    define _Windows    1
#  endif
#  ifdef __386__
#    define __32BIT__   1
#  endif
#  if defined(_Windows) && defined(__32BIT__)
#    define __WIN32__   1
#  endif
#elif defined(_MSC_VER)
#  if defined(_WINDOWS) || defined(_WIN32)
#    define _Windows    1
#  endif
#  ifdef _WIN32
#    define __WIN32__   1
#    define __32BIT__   1
#  endif
#  if _MSC_VER >= 1400
#    if !defined _CRT_SECURE_NO_DEPRECATE
#      define _CRT_SECURE_NO_DEPRECATE
#    endif
#    define strdup		_strdup
#    define stricmp		_stricmp
#    define access		_access
#    define chdir		_chdir
#    define strdup		_strdup
#    define unlink		_unlink
#  endif
#endif

#if defined __FreeBSD__
   #include <sys/endian.h>
#elif defined __APPLE__
   #include <machine/endian.h>
#elif defined LINUX
   #include <endian.h>
#endif

/* Linux NOW has these */
#if !defined BIG_ENDIAN
  #define BIG_ENDIAN    4321
#endif
#if !defined LITTLE_ENDIAN
  #define LITTLE_ENDIAN 1234
#endif

/* educated guess, BYTE_ORDER is undefined, i386 is common => little endian */
#if !defined BYTE_ORDER
  #if defined UCLINUX
    #define BYTE_ORDER BIG_ENDIAN
  #else
    #define BYTE_ORDER LITTLE_ENDIAN
  #endif
#endif

#if defined __MSDOS__ || defined __WIN32__ || defined _Windows
  #define DIRSEP_CHAR '\\'
#elif defined macintosh
  #define DIRSEP_CHAR ':'
#else
  #define DIRSEP_CHAR '/'   /* directory separator character */
#endif

/* _MAX_PATH is sometimes called differently and it may be in limits.h or
 * stdlib.h instead of stdio.h.
 */
#if !defined _MAX_PATH
  /* not defined, perhaps stdio.h was not included */
  #if !defined PATH_MAX
    #include <stdio.h>
  #endif
  #if !defined _MAX_PATH && !defined PATH_MAX
    /* no _MAX_PATH and no MAX_PATH, perhaps it is in limits.h */
    #include <limits.h>
  #endif
  #if !defined _MAX_PATH && !defined PATH_MAX
    /* no _MAX_PATH and no MAX_PATH, perhaps it is in stdlib.h */
    #include <stdlib.h>
  #endif
  /* if _MAX_PATH is undefined, try common alternative names */
  #if !defined _MAX_PATH
    #if defined MAX_PATH
      #define _MAX_PATH    MAX_PATH
    #elif defined _POSIX_PATH_MAX
      #define _MAX_PATH  _POSIX_PATH_MAX
    #else
      /* everything failed, actually we have a problem here... */
      #define _MAX_PATH  1024
    #endif
  #endif
#endif

#endif  /* _OSDEFS_H */
