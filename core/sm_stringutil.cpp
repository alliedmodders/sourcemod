#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "sm_stringutil.h"
#include "CLogger.h"

#define LADJUST			0x00000004		/* left adjustment */
#define ZEROPAD			0x00000080		/* zero (as opposed to blank) pad */
#define to_digit(c)		((c) - '0')
#define is_digit(c)		((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
	if ((arg+x) > args) { \
		g_Logger.LogError("String formatted incorrectly - parameter %d (total %d)", arg, args); \
		return 0; \
	}

//:TODO: review this code before we choose a license

void AddString(char **buf_p, size_t &maxlen, const char *string, int width, int prec)
{
	int size = 0;
	char *buf;
	static char nlstr[] = {'(','n','u','l','l',')','\0'};

	buf = *buf_p;

	if (string == NULL)
	{
		string = nlstr;
		prec = -1;
	}

	if (prec >= 0)
	{
		for (size = 0; size < prec; size++) 
		{
			if (string[size] == '\0')
			{
				break;
			}
		}
	} else {
		while (string[size++]);
		size--;
	}

	if (size > (int)maxlen)
	{
		size = maxlen;
	}

	maxlen -= size;
	width -= size;

	while (size--)
	{
		*buf++ = *string++;
	}

	while ((width-- > 0) && maxlen)
	{
		*buf++ = ' ';
		maxlen--;
	}

	*buf_p = buf;
}

void AddFloat(char **buf_p, size_t &maxlen, double fval, int width, int prec)
{
	char text[32];
	int digits;
	double signedVal;
	char *buf;
	int val;

	// get the sign
	signedVal = fval;
	if (fval < 0)
	{
		fval = -fval;
	}

	// write the float number
	digits = 0;
	val = (int)fval;
	do
	{
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while (val);

	if (signedVal < 0)
	{
		text[digits++] = '-';
	}

	buf = *buf_p;

	while ((digits < width) && maxlen)
	{
		*buf++ = ' ';
		width--;
		maxlen--;
	}

	while ((digits--) && maxlen)
	{
		*buf++ = text[digits];
		maxlen--;
	}

	*buf_p = buf;

	if (prec < 0)
	{
		prec = 6;
	}
	// write the fraction
	digits = 0;
	while (digits < prec)
	{
		fval -= (int)fval;
		fval *= 10.0;
		val = (int)fval;
		text[digits++] = '0' + val % 10;
	}

	if ((digits > 0) && maxlen)
	{
		buf = *buf_p;
		*buf++ = '.';
		maxlen--;
		for (prec = 0; maxlen && (prec < digits); prec++)
		{
			*buf++ = text[prec];
			maxlen--;
		}
		*buf_p = buf;
	}
}

void AddUInt(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while (val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

void AddInt(char **buf_p, size_t &maxlen, int val, int width, int flags)
{
	char text[32];
	int digits;
	int signedVal;
	char *buf;
	unsigned int unsignedVal;

	digits = 0;
	signedVal = val;
	if (val < 0)
	{
		/* we want the unsigned version */
		unsignedVal = abs(val);
	} else {
		unsignedVal = val;
	}
	do
	{
		text[digits++] = '0' + unsignedVal % 10;
		unsignedVal /= 10;
	} while (unsignedVal);

	if (signedVal < 0)
	{
		text[digits++] = '-';
	}

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while ((digits < width) && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

size_t gnprintf(char *buffer, size_t maxlen, const char *format, void **args)
{
	if (!buffer || !maxlen)
	{
		return 0;
	}

	int arg = 0;
	char *buf_p;
	char ch;
	int flags;
	int width;
	int prec;
	int n;
	char sign;
	const char *fmt;
	size_t llen = maxlen - 1;

	buf_p = buffer;
	fmt = format;

	while (true)
	{
		// run through the format string until we hit a '%' or '\0'
		for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
		{
			*buf_p++ = ch;
			llen--;
		}
		if ((ch == '\0') || (llen <= 0))
		{
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch(ch)
		{
		case '-':
			{
				flags |= LADJUST;
				goto rflag;
			}
		case '.':
			{
				n = 0;
				while(is_digit((ch = *fmt++)))
				{
					n = 10 * n + (ch - '0');
				}
				prec = (n < 0) ? -1 : n;
				goto reswitch;
			}
		case '0':
			{
				flags |= ZEROPAD;
				goto rflag;
			}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				n = 0;
				do
				{
					n = 10 * n + (ch - '0');
					ch = *fmt++;
				} while(is_digit(ch));
				width = n;
				goto reswitch;
			}
		case 'c':
			{
				if (!llen)
				{
					goto done;
				}
				char *c = (char *)args[arg];
				*buf_p++ = *c;
				llen--;
				arg++;
				break;
			}
		case 'd':
		case 'i':
			{
				int *value = (int *)args[arg];
				AddInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'u':
			{
				unsigned int *value = (unsigned int *)args[arg];
				AddUInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'f':
			{
				float *value = (float *)args[arg];
				AddFloat(&buf_p, llen, *value, width, prec);
				arg++;
				break;
			}
		case 's':
			{
				const char *str = (const char *)args[arg];
				AddString(&buf_p, llen, str, width, prec);
				arg++;
				break;
			}
		case '%':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		case '\0':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = '%';
				llen--;
				goto done;
			}
		default:
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		}
	}

done:
	*buf_p = '\0';

	return (maxlen - llen - 1);
}

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param)
{
	if (!buffer || !maxlen)
	{
		return 0;
	}

	int arg;
	int args = params[0];
	char *buf_p;
	char ch;
	int flags;
	int width;
	int prec;
	int n;
	char sign;
	const char *fmt;
	size_t llen = maxlen - 1;

	buf_p = buffer;
	arg = *param;
	fmt = format;

	while (true)
	{
		// run through the format string until we hit a '%' or '\0'
		for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
		{
			*buf_p++ = ch;
			llen--;
		}
		if ((ch == '\0') || (llen <= 0))
		{
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch(ch)
		{
		case '-':
			{
				flags |= LADJUST;
				goto rflag;
			}
		case '.':
			{
				n = 0;
				while(is_digit((ch = *fmt++)))
				{
					n = 10 * n + (ch - '0');
				}
				prec = (n < 0) ? -1 : n;
				goto reswitch;
			}
		case '0':
			{
				flags |= ZEROPAD;
				goto rflag;
			}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				n = 0;
				do
				{
					n = 10 * n + (ch - '0');
					ch = *fmt++;
				} while(is_digit(ch));
				width = n;
				goto reswitch;
			}
		case 'c':
			{
				CHECK_ARGS(0);
				if (!llen)
				{
					goto done;
				}
				char *c;
				pCtx->LocalToString(params[arg], &c);
				*buf_p++ = *c;
				llen--;
				arg++;
				break;
			}
		case 'd':
		case 'i':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddInt(&buf_p, llen, static_cast<int>(*value), width, flags);
				arg++;
				break;
			}
		case 'u':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddUInt(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
				arg++;
				break;
			}
		case 'f':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddFloat(&buf_p, llen, sp_ctof(*value), width, prec);
				arg++;
				break;
			}
		case 's':
			{
				CHECK_ARGS(0);
				char *str;
				pCtx->LocalToString(params[arg], &str);
				AddString(&buf_p, llen, str, width, prec);
				arg++;
				break;
			}
		case '%':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		case '\0':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = '%';
				llen--;
				goto done;
			}
		default:
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		}
	}

done:
	*buf_p = '\0';
	*param = arg;
	return (maxlen - llen - 1);
}

const char *stristr(const char *str, const char *substr)
{
	if (!*substr)
	{
		return ((char *)str);
	}

	char *needle = (char *)substr;
	char *prevloc = (char *)str;
	char *haystack = (char *)str;

	while (*haystack)
	{
		if (tolower(*haystack) == tolower(*needle))
		{
			haystack++;
			if (!*++needle)
			{
				return prevloc;
			}
		} else {
			haystack = ++prevloc;
			needle = (char *)substr;
		}
	}

	return NULL;
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	return (len >= maxlength) ? (maxlength - 1) : len;
}

