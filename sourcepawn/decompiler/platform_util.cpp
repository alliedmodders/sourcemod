#include <stdio.h>
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

