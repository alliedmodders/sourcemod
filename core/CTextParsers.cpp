#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include "CTextParsers.h"

CTextParsers g_TextParse;

static int g_ini_chartable1[255] = {0};
static int g_ws_chartable[255] = {0};

CTextParsers::CTextParsers()
{
	g_ini_chartable1['_'] = 1;
	g_ini_chartable1['-'] = 1;
	g_ini_chartable1[','] = 1;
	g_ini_chartable1['+'] = 1;
	g_ini_chartable1['.'] = 1;
	g_ini_chartable1['$'] = 1;
	g_ini_chartable1['?'] = 1;
	g_ini_chartable1['/'] = 1;
	g_ws_chartable['\n'] = 1;
	g_ws_chartable['\v'] = 1;
	g_ws_chartable['\r'] = 1;
	g_ws_chartable['\t'] = 1;
	g_ws_chartable['\f'] = 1;
	g_ws_chartable[' '] = 1;
}

unsigned int CTextParsers::GetUTF8CharBytes(const char *stream)
{
	return _GetUTF8CharBytes(stream);
}

bool CTextParsers::ParseFile_SMC(const char *file, ITextListener_SMC *smc_listener, unsigned int *line, unsigned int *col, bool strict)
{
	/* This beast is a lot more rigorous than our INI friend. */

	return false;
}

bool CTextParsers::ParseFile_INI(const char *file, ITextListener_INI *ini_listener, unsigned int *line, unsigned int *col)
{
	FILE *fp = fopen(file, "rt");
	unsigned int curline = 0;
	unsigned int curtok;
	size_t len;

	if (!fp)
	{
		if (line)
		{
			*line = 0;
		}

		return false;
	}

	char buffer[2048];
	char *ptr, *save_ptr;
	bool in_quote;

	while (!feof(fp))
	{
		curline++;
		curtok = 0;
		buffer[0] = '\0';
		if (fgets(buffer, sizeof(buffer), fp) == NULL)
		{
			break;
		}

		//:TODO: this will only run once, so find a nice way to move it out of the while loop
		/* If this is the first line, check the first three bytes for BOM */
		if (curline == 1 && 
			buffer[0] == (char)0xEF && 
			buffer[1] == (char)0xBB && 
			buffer[2] == (char)0xBF)
		{
			/* We have a UTF-8 marked file... skip these bytes */
			ptr = &buffer[3];
		} else {
			ptr = buffer;
		}

		/***************************************************
		 * We preprocess the string before parsing tokens! *
		 ***************************************************/

		/* First strip beginning whitespace */
		while (*ptr != '\0' && g_ws_chartable[*ptr] != 0)
		{
			ptr++;
		}

		len = strlen(ptr);

		if (!len)
		{
			continue;
		}

		/* Now search for comment characters */
		in_quote = false;
		save_ptr = ptr;
		for (size_t i=0; i<len; i++,ptr++)
		{
			if (!in_quote)
			{
				switch (*ptr)
				{
				case '"':
					{
						in_quote = true;
						break;
					}
				case ';':
					{
						/* Stop the loop */
						len = i;
						/* Terminate the string here */
						*ptr = '\0';
						break;
					}
				}
			} else {
				if (*ptr == '"')
				{
					in_quote = false;
				}
			}
		}

		if (!len)
		{
			continue;
		}

		ptr = save_ptr;

		/* Lastly, strip ending whitespace off */
		for (size_t i=len-1; i>=0 && i<len; i--)
		{
			if (g_ws_chartable[ptr[i]])
			{
				ptr[i] = '\0';
				len--;
			} else {
				break;
			}
		}

		if (!len)
		{
			continue;
		}

		if (!ini_listener->ReadINI_RawLine(ptr, &curtok))
		{
			goto event_failed;
		}

		if (*ptr == '[')
		{
			bool invalid_tokens = false;
			bool got_bracket = false;
			bool extra_tokens = false;
			char c;
			bool alnum;
			wchar_t wc;

			for (size_t i=1; i<len; i++)
			{
				c = ptr[i];
				alnum = false;

				if (c & (1<<7))
				{
					if (mbtowc(&wc, &ptr[i], len-i) != -1)
					{
						alnum = (iswalnum(wc) != 0);
						i += _GetUTF8CharBytes(&ptr[i]) - 1;
					}
				} else {
					alnum = (isalnum(c) != 0) || (g_ini_chartable1[c] != 0);
				}
				if (!alnum)
				{
					/* First check - is this a bracket? */
					if (c == ']')
					{
						/* Yes! */
						got_bracket = true;
						/* If this isn't the last character... */
						if (i != len - 1)
						{
							extra_tokens = true;
						}
						/* terminate */
						ptr[i] = '\0';
						break;
					} else {
						/* n...No! Continue copying. */
						invalid_tokens = true;
					}
				}
			}

			/* Tell the handler */
			if (!ini_listener->ReadINI_NewSection(&ptr[1], invalid_tokens, got_bracket, extra_tokens, &curtok))
			{
				goto event_failed;
			}
		} else {
			char *key_ptr = ptr;
			char *val_ptr = NULL;
			char c;
			size_t first_space = 0;
			bool invalid_tokens = false;
			bool equal_token = false;
			bool quotes = false;
			bool alnum;
			wchar_t wc;

			for (size_t i=0; i<len; i++)
			{
				c = ptr[i];
				alnum = false;
				/* is this an invalid char? */
				if (c & (1<<7))
				{
					if (mbtowc(&wc, &ptr[i], len-i) != -1)
					{
						alnum = (iswalnum(wc) != 0);
						i += _GetUTF8CharBytes(&ptr[i]) - 1;
					}
				} else {
					alnum = (isalnum(c) != 0) || (g_ini_chartable1[c] != 0);
				}

				if (!alnum)
				{
					if (g_ws_chartable[c])
					{
						/* if it's a space, keep track of the first occurring space */
						if (!first_space)
						{
							first_space = i;
						}
					} else {
						if (c == '=')
						{
							/* if it's an equal sign, we're done with the key */
							if (first_space)
							{
								/* remove excess whitespace */
								key_ptr[first_space] = '\0';
							} else {
								/* remove the equal sign */
								key_ptr[i] = '\0';
							}
							if (ptr[++i] != '\0')
							{
								/* If this isn't the end, set next pointer */
								val_ptr = &ptr[i];
							}
							equal_token = true;
							break;
						} else {
							/* Mark that we got something invalid! */
							invalid_tokens = true;
							first_space = 0;
						}
					}
				}
			}

			/* Now we need to parse the value, if any */
			if (val_ptr)
			{
				/* eat up spaces! there shouldn't be any h*/
				while ((*val_ptr != '\0') && g_ws_chartable[*val_ptr] != 0)
				{
					val_ptr++;
				}
				if (*val_ptr == '\0')
				{
					val_ptr = NULL;
					goto skip_value;
				}
				/* Do we have an initial quote? If so, the parsing rules change! */
				if (*val_ptr == '"' && *val_ptr != '\0')
				{
					len = strlen(val_ptr);
					if (val_ptr[len-1] == '"')
					{
						/* Strip quotes! */
						val_ptr[--len] = '\0';
						val_ptr++;
						quotes = true;
					}
				}
			}
skip_value:
			/* We're done! */
			curtok = val_ptr - buffer;
			if (!ini_listener->ReadINI_KeyValue(key_ptr, val_ptr, invalid_tokens, equal_token, quotes, &curtok))
			{
				curtok = 0;
				goto event_failed;
			}
		}
	}

	if (line)
	{
		*line = curline;
	}

	fclose(fp);

	return true;

event_failed:
	if (line)
	{
		*line = curline;
	}

	if (col)
	{
		*col = curtok;
	}

	fclose(fp);

	return false;
}
