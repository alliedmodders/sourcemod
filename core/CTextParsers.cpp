#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "CTextParsers.h"
#include "ShareSys.h"

CTextParsers g_TextParser;

static int g_ini_chartable1[255] = {0};
static int g_ws_chartable[255] = {0};

CTextParsers::CTextParsers()
{
	g_ini_chartable1[(unsigned)'_'] = 1;
	g_ini_chartable1[(unsigned)'-'] = 1;
	g_ini_chartable1[(unsigned)','] = 1;
	g_ini_chartable1[(unsigned)'+'] = 1;
	g_ini_chartable1[(unsigned)'.'] = 1;
	g_ini_chartable1[(unsigned)'$'] = 1;
	g_ini_chartable1[(unsigned)'?'] = 1;
	g_ini_chartable1[(unsigned)'/'] = 1;
	g_ws_chartable[(unsigned)'\n'] = 1;
	g_ws_chartable[(unsigned)'\v'] = 1;
	g_ws_chartable[(unsigned)'\r'] = 1;
	g_ws_chartable[(unsigned)'\t'] = 1;
	g_ws_chartable[(unsigned)'\f'] = 1;
	g_ws_chartable[(unsigned)' '] = 1;
}

void CTextParsers::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

unsigned int CTextParsers::GetUTF8CharBytes(const char *stream)
{
	return _GetUTF8CharBytes(stream);
}

/**
 * Character streams
 */

struct CharStream
{
	const char *curpos;
};

bool CharStreamReader(void *stream, char *buffer, size_t maxlength, unsigned int *read)
{
	CharStream *srdr = (CharStream *)stream;

	const char *ptr = srdr->curpos;
	for (size_t i=0; i<maxlength; i++)
	{
		if (*ptr == '\0')
		{
			break;
		}
		*buffer++ = *ptr++;
	}

	*read = ptr - srdr->curpos;

	srdr->curpos = ptr;

	return true;
}

SMCParseError CTextParsers::ParseString_SMC(const char *stream, 
					 ITextListener_SMC *smc,
					 unsigned int *line,
					 unsigned int *col)
{
	CharStream srdr = { stream };

	return ParseStream_SMC(&srdr, CharStreamReader, smc, line, col);
}

/**
 * File streams
 */

bool FileStreamReader(void *stream, char *buffer, size_t maxlength, unsigned int *read)
{
	size_t num = fread(buffer, 1, maxlength, (FILE *)stream);

	*read = static_cast<unsigned int>(num);

	if (num == 0 && feof((FILE *)stream))
	{
		return true;
	}

	return (ferror((FILE *)stream) == 0);
}

SMCParseError CTextParsers::ParseFile_SMC(const char *file, ITextListener_SMC *smc, unsigned int *line, unsigned int *col)
{
	FILE *fp = fopen(file, "rt");

	if (!fp)
	{
		return SMCParse_StreamOpen;
	}

	SMCParseError result = ParseStream_SMC(fp, FileStreamReader, smc, line, col);

	fclose(fp);

	return result;
}

/** 
 * Raw parsing of streams with helper functions
 */

struct StringInfo
{
	StringInfo() : quoted(false), ptr(NULL), end(NULL), special(false) { }
	bool quoted;
	char *ptr;
	char *end;
	bool special;
};

const char *FixupString(StringInfo &data)
{
	if (!data.ptr)
	{
		return NULL;
	}

	if (data.quoted)
	{
		data.ptr++;
	}
#if defined _DEBUG
	else {
		/* A string will never have beginning whitespace because we ignore it in the stream.
		 * Furthermore, if there is trailing whitespace, the end ptr will point to it, so it is valid
		 * to overwrite!  Lastly, the last character must be whitespace or a comment/invalid character.
		 */
 	}
#endif

	if (data.special)
	{
		//:TODO: this string has special tokens in it, like \, and we must
		//resolve these before passing the string back to the app
	}

	*(data.end) = '\0';

	return data.ptr;
}

const char *rotate(StringInfo info[3])
{
	if (info[2].ptr != NULL)
	{
		return info[2].ptr;
	}

	if (info[0].ptr != NULL)
	{
		info[2] = info[1];
		info[1] = info[0];
		info[0] = StringInfo();
	}
	
	return NULL;
}

void scrap(StringInfo info[3])
{
	info[2] = StringInfo();
	info[1] = StringInfo();
	info[0] = StringInfo();
}

void reloc(StringInfo &data, unsigned int bytes)
{
	if (data.ptr)
	{
		data.ptr -= bytes;
	}
	if (data.end)
	{
		data.end -= bytes;
	}
}

char *lowstring(StringInfo info[3])
{
	for (int i=2; i>=0; i--)
	{
		if (info[i].ptr)
		{
			return info[i].ptr;
		}
	}

	return NULL;
}

SMCParseError CTextParsers::ParseStream_SMC(void *stream, 
								   STREAMREADER srdr, 
								   ITextListener_SMC *smc, 
								   unsigned int *line, 
								   unsigned int *col)
{
	char in_buf[4096];
	char *reparse_point = NULL;
	char *parse_point = in_buf;
	char *line_begin = in_buf;
	unsigned int read;
	unsigned int curline = 1;
	unsigned int curtok = 0;
	unsigned int curlevel = 0;
	bool in_quote = false;
	bool ignoring = false;
	bool eol_comment = false;
	bool ml_comment = false;
	unsigned int i;
	SMCParseError err = SMCParse_Okay;
	SMCParseResult res;
	char c;

	StringInfo strings[3];
	StringInfo emptystring;

	smc->ReadSMC_ParseStart();

	while (srdr(stream, parse_point, sizeof(in_buf) - (parse_point - line_begin) - 1, &read))
	{
		if (!read)
		{
			break;
		}

		/* :TODO: do this outside of the main loop somehow
		 * This checks for BOM markings
		 */
		if (curline == 1 && 
			in_buf[0] == (char)0xEF && 
			in_buf[1] == (char)0xBB && 
			in_buf[2] == (char)0xBF)
		{
			parse_point = &in_buf[3];
		}

		if (reparse_point)
		{
			read += (parse_point - reparse_point);
			parse_point = reparse_point;
			reparse_point = NULL;
		}

		for (i=0; i<read; i++)
		{
			c = parse_point[i];
			if (c == '\n')
			{
				/* If we got a newline, there's a lot of things that could have happened in the interim.
				 * First, let's make sure the staged strings are rotated.
				 */
				if (strings[0].ptr)
				{
					strings[0].end = &parse_point[i];
					if (rotate(strings) != NULL)
					{
						err = SMCParse_InvalidTokens;
						goto failed;
					}
				}

				/* Next, let's clear some line-based values that may no longer have meaning */
				eol_comment = false;
				in_quote = false;
				if (ignoring && !ml_comment)
				{
					ignoring = false;
				}

				/* Pass the raw line onto the listener */
				if ((res=smc->ReadSMC_RawLine(line_begin, curline)) != SMCParse_Continue)
				{
					err = (res == SMCParse_HaltFail) ? SMCParse_Custom : SMCParse_Okay;
					goto failed;
				}

				/* Now we check the sanity of our staged strings! */
				if (strings[2].ptr)
				{
					if (!curlevel)
					{
						err = SMCParse_InvalidProperty1;
						goto failed;
					}
					/* Assume the next string is a property and pass the info on. */
					if ((res=smc->ReadSMC_KeyValue(
						FixupString(strings[2]),
						FixupString(strings[1]),
						strings[2].quoted,
						strings[1].quoted)) != SMCParse_Continue)
					{
						err = (res == SMCParse_HaltFail) ? SMCParse_Custom : SMCParse_Okay;
						goto failed;
					}
					scrap(strings);
				}

				/* Change the states for the next line */
				curtok = 0;
				curline++;
				line_begin = &parse_point[i+1];		//Note: safe because this gets relocated later
			} else if (ignoring) {
				if (in_quote)
				{
					/* If i was 0, this case is impossible due to reparsing */
					if ((i != 0) && c == '"' && parse_point[i-1] != '\\')
					{
						/* If we reached a quote in an ignore phase,
						 * we're staging a string and we must rotate it out.
						 */
						in_quote = false;
						ignoring = false;
						/* Set our info */
						strings[0].end = &parse_point[i];
						strings[0].quoted = true;
						if (rotate(strings) != NULL)
						{
							/* If we rotated too many strings, there was too much crap on one line */
							err = SMCParse_InvalidTokens;
							goto failed;
						}
					} else if (c == '\\' && i == (read - 1)) {
						strings[0].special = true;
						reparse_point = &parse_point[i];
						break;
					}
				} else if (ml_comment) {
					if (c == '*')
					{
						/* Check if we need to get more input first */
						if (i == read - 1)
						{
							reparse_point = &parse_point[i];
							break;
						}
						if (parse_point[i+1] == '/')
						{
							ml_comment = false;
							ignoring = false;
							/* We should not be staging anything right now. */
							assert(strings[0].ptr == NULL);
							/* Advance the input stream so we don't choke on this token */
							i++;
							curtok++;
						}
					}
				}
			} else {
				/* Check if we're whitespace or not */
				if (!g_ws_chartable[(unsigned)c])
				{
					bool restage = false;
					/* Check various special tokens:
					 * ;
					 * //
					 * / *
					 * {
					 * }
					 */
					if (c == ';' || c == '/')
					{
						/* If it's a line-based comment (that is, ; or //)
						 * we will need to scrap everything until the end of the line.
						 */
						if (c == '/')
						{
							if (i == read - 1)
							{
								/* If we reached the end of the look-ahead, we need to re-check our input.
								 * Breaking out will force this to be the new reparse point!
								 */
								reparse_point = &reparse_point[i];
								break;
							}
							if (parse_point[i + 1] == '/')
							{
								/* standard comment */
								ignoring = true;
								eol_comment = true;
								restage = true;
							} else if (parse_point[i+1] == '*') {
								/* inline comment - start ignoring */
								ignoring = true;
								ml_comment = true;
								/* yes, we restage, meaning that:
								 * STR/ *stuff* /ING  (space because ml comments don't nest in C++)
								 * will not generate 'STRING', but rather 'STR' and 'ING'.
								 * This should be a rare occurrence and is done here for convenience.
								 */
								restage = true;
							}
						} else {
							ignoring = true;
							eol_comment = true;
							restage = true;
						}
					} else if (c == '{') {
						/* If we are staging a string, we must rotate here */
						if (strings[0].ptr)
						{
							/* We have unacceptable tokens on this line */
							if (rotate(strings) != NULL)
							{
								err = SMCParse_InvalidSection1;
								goto failed;
							}
						}
						/* Sections must always be alone */
						if (strings[2].ptr != NULL)
						{
							err = SMCParse_InvalidSection1;
							goto failed;
						} else if (strings[1].ptr == NULL) {
							err = SMCParse_InvalidSection2;
							goto failed;
						}
						if ((res=smc->ReadSMC_NewSection(FixupString(strings[1]), strings[1].quoted))
							!= SMCParse_Continue)
						{
							err = (res == SMCParse_HaltFail) ? SMCParse_Custom : SMCParse_Okay;
							goto failed;
						}
						strings[1] = emptystring;
						curlevel++;
					} else if (c == '}') {
						/* Unlike our matching friend, this can be on the same line as something prior */
						if (rotate(strings) != NULL)
						{
							err = SMCParse_InvalidSection3;
							goto failed;
						}
						if (strings[2].ptr)
						{
							if (!curlevel)
							{
								err = SMCParse_InvalidProperty1;
								goto failed;
							}
							if ((res=smc->ReadSMC_KeyValue(
											FixupString(strings[2]),
											FixupString(strings[1]),
											strings[2].quoted,
											strings[1].quoted))
								!= SMCParse_Continue)
							{
								err = (res == SMCParse_HaltFail) ? SMCParse_Custom : SMCParse_Okay;
								goto failed;
							}
						} else if (strings[1].ptr) {
							err = SMCParse_InvalidSection3;
							goto failed;
						} else if (!curlevel) {
							err = SMCParse_InvalidSection4;
							goto failed;
						}
						/* Now it's safe to leave the section */
						scrap(strings);
						if ((res=smc->ReadSMC_LeavingSection()) != SMCParse_Continue)
						{
							err = (res == SMCParse_HaltFail) ? SMCParse_Custom : SMCParse_Okay;
							goto failed;
						}
						curlevel--;
					} else if (c == '"') {
						/* If we get a quote mark, we always restage, but we need to do it beforehand */
						if (strings[0].ptr)
						{
							strings[0].end = &parse_point[i];
							if (rotate(strings) != NULL)
							{
								err = SMCParse_InvalidTokens;
								goto failed;
							}
						}
						strings[0].ptr = &parse_point[i];
						in_quote = true;
						ignoring = true;
					} else if (!strings[0].ptr) {
						/* If we have no string, we must start one */
						strings[0].ptr = &parse_point[i];
					}
					if (restage && strings[0].ptr)
					{
						strings[0].end = &parse_point[i];
						if (rotate(strings) != NULL)
						{
							err = SMCParse_InvalidTokens;
							goto failed;
						}
					}
				} else {
					/* If we're eating a string and get whitespace, we need to restage.
					 * (Note that if we are quoted, this is being ignored)
					 */
					if (strings[0].ptr)
					{
						/*
						 * The specification says the second string in a pair does not need to be quoted.
						 * Thus, we check if there's already a string on the stack.
						 * If there's a newline, we always rotate so the newline has an empty starter.
						 */
						if (!strings[1].ptr)
						{
							/* There's no string, so we must move this one down and eat up another */
							strings[0].end = &parse_point[i];
							rotate(strings);
						} else if (!strings[1].quoted) {
							err = SMCParse_InvalidTokens;
							goto failed;
						}
					}
				}
			}

			/* Advance which token we're on */
			curtok++;
		}

		if (line_begin != in_buf)
		{
			/* The line buffer has advanced, so it's safe to copy N bytes back to the beginning.
			 * What's N?  N is the lowest point we're currently relying on.
			 */
			char *stage = lowstring(strings);
			if (!stage || stage > line_begin)
			{
				stage = line_begin;
			}
			unsigned int bytes = read - (stage - parse_point);

			/* It is now safe to delete everything before the staged point */
			memmove(in_buf, stage, bytes);

			/* Calculate the number of bytes in the new buffer */
			bytes = stage - in_buf;
			/* Relocate all the cached pointers to our new base */
			line_begin -= bytes;
			reloc(strings[0], bytes);
			reloc(strings[1], bytes);
			reloc(strings[2], bytes);
			if (reparse_point)
			{
				reparse_point -= bytes;
			}
			if (parse_point)
			{
				parse_point = &parse_point[read];
				parse_point -= bytes;
			}
		} else if (read == sizeof(in_buf) - 1) {
			err = SMCParse_TokenOverflow;
			goto failed;
		}
	}

	/* If we're done parsing and there are tokens left over... */
	if (curlevel)
	{
		err = SMCParse_InvalidSection5;
		goto failed;
	} else if (strings[0].ptr || strings[1].ptr) {
		err = SMCParse_InvalidTokens;
		goto failed;
	}
	
	smc->ReadSMC_ParseEnd(false, false);

	return SMCParse_Okay;

failed:
	if (line)
	{
		*line = curline;
	}

	smc->ReadSMC_ParseEnd(true, (err == SMCParse_Custom));

	if (col)
	{
		*col = curtok;
	}

	return err;
}


/**
 * INI parser 
 */

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
		while (*ptr != '\0' && g_ws_chartable[(unsigned)*ptr] != 0)
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
			if (g_ws_chartable[(unsigned)ptr[i]])
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
					alnum = (isalnum(c) != 0) || (g_ini_chartable1[(unsigned)c] != 0);
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
					alnum = (isalnum(c) != 0) || (g_ini_chartable1[(unsigned)c] != 0);
				}

				if (!alnum)
				{
					if (g_ws_chartable[(unsigned)c])
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
				while ((*val_ptr != '\0') && g_ws_chartable[(unsigned)*val_ptr] != 0)
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

const char *CTextParsers::GetSMCErrorString(SMCParseError err)
{
	static const char *s_errors[] = 
	{
		NULL,
		"Stream failed to open",
		"Stream returned read error",
		NULL,
		"Un-quoted section has invalid tokens",
		"Section declared without header",
		"Section declared with unknown tokens",
		"Section ending without a matching section beginning",
		"Line contained too many invalid tokens",
		"Token buffer overflowed",
		"A property was declared outside of a section",
	};

	if (err < SMCParse_Okay || err > SMCParse_InvalidProperty1)
	{
		return NULL;
	}

	return s_errors[err];
}
