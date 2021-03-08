// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.

#include "common_logic.h"
#include "Translator.h"
#include "sprintf.h"
#include <am-float.h>
#include <am-string.h>
#include <IDBDriver.h>
#include <ITranslator.h>
#include <bridge/include/IScriptManager.h>
#include <bridge/include/CoreProvider.h>

using namespace SourceMod;

IDatabase *g_FormatEscapeDatabase = NULL;

#define LADJUST			0x00000001		/* left adjustment */
#define ZEROPAD			0x00000002		/* zero (as opposed to blank) pad */
#define UPPERDIGITS		0x00000004		/* make alpha digits uppercase */
#define NOESCAPE		0x00000008		/* do not escape strings (they are only escaped if a database connection is provided) */
#define to_digit(c)		((c) - '0')
#define is_digit(c)		((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
	if ((arg+x) > args) { \
		pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "String formatted incorrectly - parameter %d (total %d)", arg, args); \
		return 0; \
	}

inline void ReorderTranslationParams(const Translation *pTrans, cell_t *params)
{
	cell_t new_params[MAX_TRANSLATE_PARAMS];
	for (unsigned int i = 0; i < pTrans->fmt_count; i++)
	{
		new_params[i] = params[pTrans->fmt_order[i]];
	}
	memcpy(params, new_params, pTrans->fmt_count * sizeof(cell_t));
}

size_t Translate(char *buffer,
				 size_t maxlen,
				 IPluginContext *pCtx,
				 const char *key,
				 cell_t target,
				 const cell_t *params,
				 int *arg,
				 bool *error)
{
	unsigned int langid;
	*error = false;
	Translation pTrans;
	IPlugin *pl = scripts->FindPluginByContext(pCtx->GetContext());
	unsigned int max_params = 0;
	IPhraseCollection *pPhrases;

	pPhrases = pl->GetPhrases();

try_serverlang:
	if (target == SOURCEMOD_SERVER_LANGUAGE)
	{
		langid = g_Translator.GetServerLanguage();
 	}
	else if ((target >= 1) && (target <= bridge->MaxClients()))
	{
		langid = g_Translator.GetClientLanguage(target);
	}
	else
	{
		pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Translation failed: invalid client index %d (arg %d)", target, *arg);
		goto error_out;
	}

	if (pPhrases->FindTranslation(key, langid, &pTrans) != Trans_Okay)
	{
		if (target != SOURCEMOD_SERVER_LANGUAGE && langid != g_Translator.GetServerLanguage())
		{
			target = SOURCEMOD_SERVER_LANGUAGE;
			goto try_serverlang;
		}
		else if (langid != SOURCEMOD_LANGUAGE_ENGLISH)
		{
			if (pPhrases->FindTranslation(key, SOURCEMOD_LANGUAGE_ENGLISH, &pTrans) != Trans_Okay)
			{
				pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found (arg %d)", key, *arg);
				goto error_out;
			}
		}
		else
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found (arg %d)", key, *arg);
			goto error_out;
		}
	}

	max_params = pTrans.fmt_count;

	if (max_params)
	{
		cell_t new_params[MAX_TRANSLATE_PARAMS];

		/* Check if we're going to over the limit */
		if ((*arg) + (max_params - 1) > (size_t)params[0])
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAMS_MAX, 
				"Translation string formatted incorrectly - missing at least %d parameters (arg %d)", 
				((*arg + (max_params - 1)) - params[0]), *arg);
			goto error_out;
		}

		/* If we need to re-order the parameters, do so with a temporary array.
		 * Otherwise, we could run into trouble with continual formats, a la ShowActivity().
		 */
		memcpy(new_params, params, sizeof(cell_t) * (params[0] + 1));
		ReorderTranslationParams(&pTrans, &new_params[*arg]);

		return atcprintf(buffer, maxlen, pTrans.szPhrase, pCtx, new_params, arg);
	}
	else
	{
		return atcprintf(buffer, maxlen, pTrans.szPhrase, pCtx, params, arg);
	}
	
error_out:
	*error = true;
	return 0;
}

bool AddString(char **buf_p, size_t &maxlen, const char *string, int width, int prec, int flags)
{
	int size = 0;
	char *buf;
	static char nlstr[] = {'(','n','u','l','l',')','\0'};

	buf = *buf_p;

	if (string == NULL)
	{
		string = nlstr;
		prec = -1;
		flags |= NOESCAPE;
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
	}
	else
	{
		while (string[size++]);
		size--;
	}

	if (size > (int)maxlen)
	{
		size = maxlen;
	}

	width -= size;

	if (g_FormatEscapeDatabase && (flags & NOESCAPE) == 0)
	{
		char *tempBuffer = NULL;
		if (prec != -1)
		{
			// I doubt anyone will ever do this, so just allocate.
			tempBuffer = new char[maxlen + 1];
			memcpy(tempBuffer, string, size);
			tempBuffer[size] = '\0';
		}

		size_t newSize;
		bool ret = g_FormatEscapeDatabase->QuoteString(tempBuffer ? tempBuffer : string, buf, maxlen + 1, &newSize);

		if (tempBuffer)
		{
			delete[] tempBuffer;
		}

		if (!ret)
		{
			return false;
		}

		maxlen -= newSize;
		buf += newSize;
		size = 0; // Consistency.
	}
	else
	{
		maxlen -= size;

		while (size--)
		{
			*buf++ = *string++;
		}
	}

	while ((width-- > 0) && maxlen)
	{
		*buf++ = ' ';
		maxlen--;
	}

	*buf_p = buf;

	return true;
}

void AddFloat(char **buf_p, size_t &maxlen, double fval, int width, int prec, int flags)
{
	int digits;					// non-fraction part digits
	double tmp;					// temporary
	char *buf = *buf_p;			// output buffer pointer
	int val;					// temporary
	int sign = 0;				// 0: positive, 1: negative
	int fieldlength;			// for padding
	int significant_digits = 0;	// number of significant digits written
	const int MAX_SIGNIFICANT_DIGITS = 16;

	if (ke::IsNaN(fval))
	{
		AddString(buf_p, maxlen, "NaN", width, prec, flags | NOESCAPE);
		return;
	}

	// default precision
	if (prec < 0)
	{
		prec = 6;
	}

	// get the sign
	if (fval < 0)
	{
		fval = -fval;
		sign = 1;
	}

	// compute whole-part digits count
	digits = (int)log10(fval) + 1;

	// Only print 0.something if 0 < fval < 1
	if (digits < 1)
	{
		digits = 1;
	}

	// compute the field length
	fieldlength = digits + prec + ((prec > 0) ? 1 : 0) + sign;

	// minus sign BEFORE left padding if padding with zeros
	if (sign && maxlen && (flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// right justify if required
	if ((flags & LADJUST) == 0)
	{
		while ((fieldlength < width) && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	// minus sign AFTER left padding if padding with spaces
	if (sign && maxlen && !(flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// write the whole part
	tmp = pow(10.0, digits-1);
	while ((digits--) && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
			tmp *= 0.1;
		}
		maxlen--;
	}

	// write the fraction part
	if (maxlen && prec)
	{
		*buf++ = '.';
		maxlen--;
	}

	tmp = pow(10.0, prec);

	fval *= tmp;
	while (prec-- && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			tmp *= 0.1;
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
		}
		maxlen--;
	}

	// left justify if required
	if (flags & LADJUST)
	{
		while ((fieldlength < width) && maxlen)
		{
			// right-padding only with spaces, ZEROPAD is ignored
			*buf++ = ' ';
			width--;
			maxlen--;
		}
	}

	// update parent's buffer pointer
	*buf_p = buf;
}

void AddBinary(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		if (val & 1)
		{
			text[digits++] = '1';
		}
		else
		{
			text[digits++] = '0';
		}
		val >>= 1;
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
	}
	else
	{
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

void AddHex(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;
	char digit;
	int hexadjust;

	if (flags & UPPERDIGITS)
	{
		hexadjust = 'A' - '9' - 1;
	}
	else
	{
		hexadjust = 'a' - '9' - 1;
	}

	digits = 0;
	do 
	{
		digit = ('0' + val % 16);
		if (digit > '9')
		{
			digit += hexadjust;
		}

		text[digits++] = digit;
		val /= 16;
	} while(val);

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

bool gnprintf(char *buffer,
			  size_t maxlen,
			  const char *format,
			  IPhraseCollection *pPhrases,
			  void **params,
			  unsigned int numparams,
			  unsigned int &curparam,
			  size_t *pOutLength,
			  const char **pFailPhrase)
{
	if (!buffer || !maxlen)
	{
		if (pOutLength != NULL)
		{
			*pOutLength = 0;
		}
		return true;
	}

	if (numparams > MAX_TRANSLATE_PARAMS)
	{
		if (pFailPhrase != NULL)
		{
			*pFailPhrase = NULL;
		}
		return false;
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
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				char *c = (char *)params[curparam];
				curparam++;
				*buf_p++ = *c;
				llen--;
				arg++;
				break;
			}
		case 'b':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				int *value = (int *)params[curparam];
				curparam++;
				AddBinary(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'd':
		case 'i':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				int *value = (int *)params[curparam];
				curparam++;
				AddInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'u':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				unsigned int *value = (unsigned int *)params[curparam];
				curparam++;
				AddUInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'f':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				float *value = (float *)params[curparam];
				curparam++;
				AddFloat(&buf_p, llen, *value, width, prec, flags);
				arg++;
				break;
			}
		case 's':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				const char *str = (const char *)params[curparam];
				curparam++;
				AddString(&buf_p, llen, str, width, prec, flags);
				arg++;
				break;
			}
		case 'T':
		case 't':
			{
				int target;
				const char *key;
				size_t out_length;
				Translation trans;
				unsigned int lang_id;

				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				key = (const char *)(params[curparam]);
				curparam++;

				if (ch == 'T')
				{
					if (curparam >= numparams)
					{
						if (pFailPhrase != NULL)
						{
							*pFailPhrase = NULL;
						}
						return false;
					}
					target = *((int *)(params[curparam]));
					curparam++;
				}
				else
				{
					target = g_Translator.GetGlobalTarget();
				}

try_again:
				if (target == SOURCEMOD_SERVER_LANGUAGE)
				{
					lang_id = g_Translator.GetServerLanguage();
				}
				else if (target >= 1 && target <= bridge->MaxClients())
				{
					lang_id = g_Translator.GetClientLanguage(target);
				}
				else
				{
					lang_id = g_Translator.GetServerLanguage();
				}

				if (pPhrases == NULL)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = key;
					}
					return false;
				}

				if (pPhrases->FindTranslation(key, lang_id, &trans) != Trans_Okay)
				{
					if (target != SOURCEMOD_SERVER_LANGUAGE && lang_id != g_Translator.GetServerLanguage())
					{
						target = SOURCEMOD_SERVER_LANGUAGE;
						goto try_again;
					}
					else if (lang_id != SOURCEMOD_LANGUAGE_ENGLISH)
					{
						if (pPhrases->FindTranslation(key, SOURCEMOD_LANGUAGE_ENGLISH, &trans) != Trans_Okay)
						{
							if (pFailPhrase != NULL)
							{
								*pFailPhrase = key;
							}
							return false;
						}
					}
					else
					{
						if (pFailPhrase != NULL)
						{
							*pFailPhrase = key;
						}
						return false;
					}
				}

				if (trans.fmt_count)
				{
					unsigned int i;
					void *new_params[MAX_TRANSLATE_PARAMS];

					if (curparam + trans.fmt_count > numparams)
					{
						if (pFailPhrase != NULL)
						{
							*pFailPhrase = NULL;
						}
						return false;
					}

					/* Copy the array and re-order the stack */
					memcpy(new_params, params, sizeof(void *) * numparams);
					for (i = 0; i < trans.fmt_count; i++)
					{
						new_params[curparam + i] = const_cast<void *>(params[curparam + trans.fmt_order[i]]);
					}

					if (!gnprintf(buf_p,
							llen,
							trans.szPhrase,
							pPhrases,
							new_params,
							numparams,
							curparam,
							&out_length,
							pFailPhrase))
					{
						return false;
					}
				}
				else
				{
					if (!gnprintf(buf_p,
						llen,
						trans.szPhrase,
						pPhrases,
						params,
						numparams,
						curparam,
						&out_length,
						pFailPhrase))
					{
						return false;
					}
				}

				buf_p += out_length;
				llen -= out_length;

				break;
			}
		case 'X':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				unsigned int *value = (unsigned int *)params[curparam];
				curparam++;
				flags |= UPPERDIGITS;
				AddHex(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'x':
			{
				if (curparam >= numparams)
				{
					if (pFailPhrase != NULL)
					{
						*pFailPhrase = NULL;
					}
					return false;
				}
				unsigned int *value = (unsigned int *)params[curparam];
				curparam++;
				AddHex(&buf_p, llen, *value, width, flags);
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

	if (pOutLength != NULL)
	{
		*pOutLength = (maxlen - llen - 1);
	}

	return true;
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
		case '!':
			{
				flags |= NOESCAPE;
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
		case 'b':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddBinary(&buf_p, llen, *value, width, flags);
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
				AddFloat(&buf_p, llen, sp_ctof(*value), width, prec, flags);
				arg++;
				break;
			}
		case 'L':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				char buffer[255];
				if (*value)
				{
					const char *name;
					const char *auth;
					int userid;
					if (!bridge->DescribePlayer(*value, &name, &auth, &userid))
						return pCtx->ThrowNativeError("Client index %d is invalid (arg %d)", *value, arg);
					
					ke::SafeSprintf(buffer, sizeof(buffer), "%s<%d><%s><>", name, userid, auth);
				}
				else
				{
					ke::SafeStrcpy(buffer, sizeof(buffer), "Console<0><Console><Console>");
				}
				if (!AddString(&buf_p, llen, buffer, width, prec, flags))
					return pCtx->ThrowNativeError("Escaped string would be truncated (arg %d)", arg);
				arg++;
				break;
			}
		case 'N':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);

				const char *name = "Console";
				if (*value) {
					if (!bridge->DescribePlayer(*value, &name, nullptr, nullptr))
						return pCtx->ThrowNativeError("Client index %d is invalid (arg %d)", *value, arg);
				}
				if (!AddString(&buf_p, llen, name, width, prec, flags))
					return pCtx->ThrowNativeError("Escaped string would be truncated (arg %d)", arg);
				arg++;
				break;
			}
		case 's':
			{
				CHECK_ARGS(0);
				char *str;
				pCtx->LocalToString(params[arg], &str);
				if (!AddString(&buf_p, llen, str, width, prec, flags))
					return pCtx->ThrowNativeError("Escaped string would be truncated (arg %d)", arg);
				arg++;
				break;
			}
		case 'T':
			{
				CHECK_ARGS(1);
				char *key;
				bool error;
				size_t res;
				cell_t *target;
				pCtx->LocalToString(params[arg++], &key);
				pCtx->LocalToPhysAddr(params[arg++], &target);
				res = Translate(buf_p, llen, pCtx, key, *target, params, &arg, &error);
				if (error)
				{
					return 0;
				}
				buf_p += res;
				llen -= res;
				break;
			}
		case 't':
			{
				CHECK_ARGS(0);
				char *key;
				bool error;
				size_t res;
				cell_t target = bridge->GetGlobalTarget();
				pCtx->LocalToString(params[arg++], &key);
				res = Translate(buf_p, llen, pCtx, key, target, params, &arg, &error);
				if (error)
				{
					return 0;
				}
				buf_p += res;
				llen -= res;
				break;
			}
		case 'X':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				flags |= UPPERDIGITS;
				AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
				arg++;
				break;
			}
		case 'x':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
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

