#include "main.h"
#include <stdarg.h>

BuildMD5ableBuffer g_MD5Builder;

int main(int argc, char **argv)
{
	SMCStates states;
	SMCError error;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: <file>\n");
		return -1;
	}

	error = g_TextParser.ParseFile_SMC(argv[1], &g_MD5Builder, &states);

	if (error == SMCError_Okay)
	{
		printf("%s", g_MD5Builder.GetMD5String());
		return 0;
	}

	fprintf(stderr, "Failed to parse file, Error %i at line %i and col %i\n", error, states.line, states.col);
	return -1;
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

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
