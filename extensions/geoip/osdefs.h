/* __MSDOS__    set when compiling for DOS (not Windows)
 * _Windows     set when compiling for any version of Microsoft Windows
 * __WIN32__    set when compiling for Windows95 or WindowsNT (32 bit mode)
 * __32BIT__    set when compiling in 32-bit "flat" mode (DOS or Windows)
 *
 * Copyright 1998-2002, ITB CompuPhase, The Netherlands.
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
#endif

#if defined __linux__
  #include <endian.h>
#endif

#if defined __APPLE__
  #include <sys/types.h>
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

#endif  /* _OSDEFS_H */

