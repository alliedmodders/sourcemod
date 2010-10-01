#ifndef _INCLUDE_GDC_MAIN_H_
#define _INCLUDE_GDC_MAIN_H_

#include <stdarg.h>
#if 0
#include "TextParsers.h"
#include "GameConfigs.h"

using namespace SourceMod;
#endif 

int findFuncPos(const char* sym);
int findVFunc(void *handle, void **vt, const char *symbol);

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
unsigned int strncopy(char *dest, const char *src, size_t count);

#endif // _INCLUDE_GDC_MAIN_H_
