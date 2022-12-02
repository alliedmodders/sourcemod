/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "common_logic.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sm_platform.h>
#include "stringutil.h"
#include "sprintf.h"
#include <am-string.h>
#include "TextParsers.h"

// We're in logic so we don't have this from the SDK.
#ifndef MIN
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

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
		}
		else
		{
			haystack = ++prevloc;
			needle = (char *)substr;
		}
	}

	return NULL;
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	return ke::SafeStrcpy(dest, count, src);
}

unsigned int UTIL_ReplaceAll(char *subject, size_t maxlength, const char *search, const char *replace, bool caseSensitive)
{
	size_t searchLen = strlen(search);
	size_t replaceLen = strlen(replace);

	char *newptr, *ptr = subject;
	unsigned int total = 0;
	while ((newptr = UTIL_ReplaceEx(ptr, maxlength, search, searchLen, replace, replaceLen, caseSensitive)) != NULL)
	{
		total++;
		maxlength -= newptr - ptr;
		ptr = newptr;
		if (*ptr == '\0')
		{
			break;
		}
	}

	return total;
}

/**
 * NOTE: Do not edit this for the love of god unless you have 
 * read the test cases and understand the code behind each one.
 * While I don't guarantee there aren't mistakes, I do guarantee
 * that plugins will end up relying on tiny idiosyncrasies of this
 * function, just like they did with AMX Mod X.
 *
 * There are explicitly more cases than the AMX Mod X version because
 * we're not doing a blind copy.  Each case is specifically optimized
 * for what needs to be done.  Even better, we don't have to error on
 * bad buffer sizes.  Instead, this function will smartly cut off the 
 * string in a way that pushes old data out.
 */
char *UTIL_ReplaceEx(char *subject, size_t maxLen, const char *search, size_t searchLen, const char *replace, size_t replaceLen, bool caseSensitive)
{
	char *ptr = subject;
	size_t browsed = 0;
	size_t textLen = strlen(subject);

	/* It's not possible to search or replace */
	if (searchLen > textLen)
	{
		return NULL;
	}

	/* Handle the case of one byte replacement.
	 * It's only valid in one case.
	 */
	if (maxLen == 1)
	{
		/* If the search matches and the replace length is 0, 
		 * we can just terminate the string and be done.
		 */
		if ((caseSensitive ? strcmp(subject, search) : strcasecmp(subject, search)) == 0 && replaceLen == 0)
		{
			*subject = '\0';
			return subject;
		}
		else
		{
			return NULL;
		}
	}

	/* Subtract one off the maxlength so we can include the null terminator */
	maxLen--;

	while (*ptr != '\0' && (browsed <= textLen - searchLen))
	{
		/* See if we get a comparison */
		if ((caseSensitive ? strncmp(ptr, search, searchLen) : strncasecmp(ptr, search, searchLen)) == 0)
		{
			if (replaceLen > searchLen)
			{
				/* First, see if we have enough space to do this operation */
				if (maxLen - textLen < replaceLen - searchLen)
				{
					/* First, see if the replacement length goes out of bounds. */
					if (browsed + replaceLen >= maxLen)
					{
						/* EXAMPLE CASE:
						 * Subject: AABBBCCC
						 * Buffer : 12 bytes
						 * Search : BBB
						 * Replace: DDDDDDDDDD
						 * OUTPUT : AADDDDDDDDD
						 * POSITION:           ^
						 */
						/* If it does, we'll just bound the length and do a strcpy. */
						replaceLen = maxLen - browsed;
						/* Note, we add one to the final result for the null terminator */
						strncopy(ptr, replace, replaceLen+1);
					}
					else
					{
						/* EXAMPLE CASE:
						 * Subject: AABBBCCC
						 * Buffer : 12 bytes
						 * Search : BBB
						 * Replace: DDDDDDD
						 * OUTPUT : AADDDDDDDCC
						 * POSITION:         ^
						 */
						/* We're going to have some bytes left over... */
						size_t origBytesToCopy = (textLen - (browsed + searchLen)) + 1;
						size_t realBytesToCopy = (maxLen - (browsed + replaceLen)) + 1;
						char *moveFrom = ptr + searchLen + (origBytesToCopy - realBytesToCopy);
						char *moveTo = ptr + replaceLen;

						/* First, move our old data out of the way. */
						memmove(moveTo, moveFrom, realBytesToCopy);

						/* Now, do our replacement. */
						memcpy(ptr, replace, replaceLen);
					}
				}
				else
				{
					/* EXAMPLE CASE:
					 * Subject: AABBBCCC
					 * Buffer : 12 bytes
					 * Search : BBB
					 * Replace: DDDD
					 * OUTPUT : AADDDDCCC
					 * POSITION:      ^
					 */
					/* Yes, we have enough space.  Do a normal move operation. */
					char *moveFrom = ptr + searchLen;
					char *moveTo = ptr + replaceLen;

					/* First move our old data out of the way. */
					size_t bytesToCopy = (textLen - (browsed + searchLen)) + 1;
					memmove(moveTo, moveFrom, bytesToCopy);

					/* Now do our replacement. */
					memcpy(ptr, replace, replaceLen);
				}
			}
			else if (replaceLen < searchLen)
			{
				/* EXAMPLE CASE:
				 * Subject: AABBBCCC
				 * Buffer : 12 bytes
				 * Search : BBB
				 * Replace: D
				 * OUTPUT : AADCCC
				 * POSITION:   ^
				 */
				/* If the replacement does not grow the string length, we do not
				 * need to do any fancy checking at all.  Yay!
				 */
				char *moveFrom = ptr + searchLen;		/* Start after the search pointer */
				char *moveTo = ptr + replaceLen;		/* Copy to where the replacement ends */

				/* Copy our replacement in, if any */
				if (replaceLen)
				{
					memcpy(ptr, replace, replaceLen);
				}

				/* Figure out how many bytes to move down, including null terminator */
				size_t bytesToCopy = (textLen - (browsed + searchLen)) + 1;

				/* Move the rest of the string down */
				memmove(moveTo, moveFrom, bytesToCopy);
			}
			else
			{
				/* EXAMPLE CASE:
				 * Subject: AABBBCCC
				 * Buffer : 12 bytes
				 * Search : BBB
				 * Replace: DDD
				 * OUTPUT : AADDDCCC
				 * POSITION:     ^
				 */
				/* We don't have to move anything around, just do a straight copy */
				memcpy(ptr, replace, replaceLen);
			}

			return ptr + replaceLen;
		}
		ptr++;
		browsed++;
	}

	return NULL;
}

size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[written - 1] = r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')

void UTIL_StripExtension(const char *in, char *out, int outSize)
{
	// Find the last dot. If it's followed by a dot or a slash, then it's part of a 
	// directory specifier like ../../somedir/./blah.

	// scan backward for '.'
	int end = strlen(in) - 1;
	while (end > 0 && in[end] != '.' && !PATHSEPARATOR(in[end]))
	{
		--end;
	}

	if (end > 0 && !PATHSEPARATOR(in[end]) && end < outSize)
	{
		int nChars = MIN(end, outSize-1);
		if (out != in)
		{
			memcpy(out, in, nChars);
		}
		out[nChars] = 0;
	}
	else
	{
		// nothing found
		if (out != in)
		{
			strncopy(out, in, outSize);
		}
	}
}

char *UTIL_TrimWhitespace(char *str, size_t &len)
{
	char *end = str + len - 1;

	if (!len)
	{
		return str;
	}

	/* Iterate backwards through string until we reach first non-whitespace char */
	while (end >= str && g_TextParser.IsWhitespace(end))
	{
		end--;
		len--;
	}

	/* Replace first whitespace char (at the end) with null terminator.
	 * If there is none, we're just replacing the null terminator. 
	 */
	*(end + 1) = '\0';

	while (*str != '\0' && g_TextParser.IsWhitespace(str))
	{
		str++;
		len--;
	}

	return str;
}

class StaticCharBuf
{
    char *buffer;
    size_t max_size;
public:
    StaticCharBuf() : buffer(NULL), max_size(0)
    {
    }
    ~StaticCharBuf()
    {
        free(buffer);
    }
    char* GetWithSize(size_t len)
    {
        if (len > max_size)
        {
            auto *newbuffer = static_cast<char *>(realloc(buffer, len));
            if (!newbuffer)
                return nullptr;

            buffer = newbuffer;
            max_size = len;
        }
        return buffer;
    }
};

static char g_formatbuf[2048];
static StaticCharBuf g_extrabuf;
cell_t InternalFormat(IPluginContext *pCtx, const cell_t *params, int start)
{
	char *buf, *fmt, *destbuf;
	cell_t start_addr, end_addr, maxparam;
	size_t res, maxlen;
	int arg = start + 4;
	bool copy = false;
	char *__copy_buf;

	pCtx->LocalToString(params[start + 1], &destbuf);
	pCtx->LocalToString(params[start + 3], &fmt);

	maxlen = static_cast<size_t>(params[start + 2]);
	start_addr = params[start + 1];
	end_addr = params[start + 1] + maxlen;
	maxparam = params[0];

	for (cell_t i = (start + 3); i <= maxparam; i++)
	{
		if ((params[i] >= start_addr) && (params[i] <= end_addr))
		{
			copy = true;
			break;
		}
	}

	if (copy)
	{
		if (maxlen > sizeof(g_formatbuf))
		{
			char *tmpbuff = g_extrabuf.GetWithSize(maxlen);
			if (!tmpbuff)
				return pCtx->ThrowNativeError("Unable to allocate buffer with a size of \"%u\"", maxlen);

			__copy_buf = tmpbuff;
		}
		else
		{
			__copy_buf = g_formatbuf;
		}
	}

	buf = (copy) ? __copy_buf : destbuf;
	res = atcprintf(buf, maxlen, fmt, pCtx, params, &arg);

	if (copy)
	{
		memcpy(destbuf, __copy_buf, res+1);
	}

	return static_cast<cell_t>(res);
}
