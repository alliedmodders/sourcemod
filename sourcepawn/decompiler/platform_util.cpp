#include <new>
#include <stdio.h>
#include <stdlib.h>
#include "platform_util.h"

size_t Sp_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	size_t len;

	va_list ap;
	va_start(ap, fmt);
	len = Sp_FormatArgs(buffer, maxlength, fmt, ap);
	va_end(ap);

	return len;
}

size_t Sp_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap)
{
	size_t len;

	len = vsnprintf(buffer, maxlength, fmt, ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

/* Overload a few things to prevent libstdc++ linking */
#if defined __linux__
extern "C" void __cxa_pure_virtual(void)
{
}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[](size_t size) 
{
	return malloc(size);
}

void operator delete(void *ptr) 
{
	free(ptr);
}

void operator delete[](void * ptr)
{
	free(ptr);
}
#endif
