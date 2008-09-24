#ifndef _INCLUDE_SP_DECOMP_PLATFORM_UTIL_H_
#define _INCLUDE_SP_DECOMP_PLATFORM_UTIL_H_

#include <stdarg.h>

size_t Sp_Format(char *buffer, size_t maxlength, const char *fmt, ...);
size_t Sp_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap);


#endif //_INCLUDE_SP_DECOMP_PLATFORM_UTIL_H_
