#ifndef _INCLUDE_INSTALLER_UTIL_H_
#define _INCLUDE_INSTALLER_UTIL_H_

#include "platform_headers.h"

bool IsWhiteSpaceA(const char *stream);
size_t UTIL_FormatArgs(TCHAR *buffer, size_t count, const TCHAR *fmt, va_list ap);
size_t UTIL_Format(TCHAR *buffer, size_t count, const TCHAR *fmt, ...);
size_t UTIL_PathFormat(TCHAR *buffer, size_t count, const TCHAR *fmt, ...);
int tstrcasecmp(const TCHAR *str1, const TCHAR *str2);
int BreakStringA(const char *str, char *out, size_t maxchars);
size_t AnsiToUnicode(const char *str, wchar_t *buffer, size_t maxchars);
const TCHAR *GetFileFromPath(const TCHAR *path);
void GenerateErrorMessage(DWORD err, TCHAR *buffer, size_t maxchars);
size_t UTIL_GetFileSize(const TCHAR *file_path);
size_t UTIL_GetFolderSize(const TCHAR *basepath);

INT_PTR AskToExit(HWND hWnd);

#endif //_INCLUDE_INSTALLER_UTIL_H_
