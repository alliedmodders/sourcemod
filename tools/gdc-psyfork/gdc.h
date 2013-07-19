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

int checkSigStringW(int file, const char* symbol);
int checkSigStringL(void* handle, const char* symbol);

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...);
size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr);
unsigned int strncopy(char *dest, const char *src, size_t count);
void CheckLinuxSigOffset(char* name, const char* symbol, void * handle);
void CheckWindowsSigOffset(char* name, const char* symbol, int file);
void *GetLinuxSigPtr(void *handle, const char* symbol);
void *GetWindowsSigPtr(int file, const char* symbol);
int GetOffset(const char* key, bool windows);

#endif // _INCLUDE_GDC_MAIN_H_
