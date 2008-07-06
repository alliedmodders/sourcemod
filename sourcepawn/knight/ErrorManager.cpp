#include <stdio.h>
#include <stdarg.h>
#include "ErrorManager.h"

using namespace Knight;

static const char *g_ErrorMessages[] = 
{
	"expected token %s, found %s"
};

static const char *g_FatalMessages[] = 
{
	"fatal internal bug: %s"
};

ErrorManager::ErrorManager() : m_NumErrors(0)
{
}

void ErrorManager::AddError(ErrorMessage err, unsigned int line, ...)
{
	va_list ap;

	if (err < 0 || err >= ErrorMessages_Total)
	{
		ThrowFatal(FatalMessage_Assert, "invalid error message");
		return;
	}

	va_start(ap, line);
	vfprintf(stderr, g_ErrorMessages[err], ap);
	va_end(ap);

	m_NumErrors++;
}

void ErrorManager::ThrowFatal(FatalMessage err, ...)
{
	va_list ap;

	if (err < 0 || err >= FatalMessages_Total)
	{
		ThrowFatal(FatalMessage_Assert, "invalid fatal error message");
		return;
	}

	va_start(ap, err);
	vfprintf(stderr, g_FatalMessages[err], ap);
	va_end(ap);
}
