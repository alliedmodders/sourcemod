/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "TextParsers.h"
//#include "ShareSys.h"
//#include "sm_stringutil.h"
//#include "LibrarySys.h"

TextParsers g_TextParser;
ITextParsers *textparsers = &g_TextParser;

static int g_ini_chartable1[255] = {0};
static int g_ws_chartable[255] = {0};

bool TextParsers::IsWhitespace(const char *stream)
{
	return g_ws_chartable[(unsigned)*stream] == 1;
}

TextParsers::TextParsers()
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

unsigned int TextParsers::GetUTF8CharBytes(const char *stream)
{
	return _GetUTF8CharBytes(stream);
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

SMCError TextParsers::ParseFile_SMC(const char *file, ITextListener_SMC *smc, SMCStates *states)
{
	FILE *fp = fopen(file, "rt");

	if (!fp)
	{
		if (states != NULL)
		{
			states->line = 0;
			states->col = 0;
		}
		return SMCError_StreamOpen;
	}

	SMCError result = ParseStream_SMC(fp, FileStreamReader, smc, states);

	fclose(fp);

	return result;
}

SMCError TextParsers::ParseSMCFile(const char *file,
								   ITextListener_SMC *smc_listener,
								   SMCStates *states,
								   char *buffer,
								   size_t maxsize)
{
	const char *errstr;
	FILE *fp = fopen(file, "rt");

	if (fp == NULL)
	{
		char error[256] = "unknown";
		if (states != NULL)
		{
			states->line = 0;
			states->col = 0;
		}
		//g_LibSys.GetPlatformError(error, sizeof(error));
		UTIL_Format(buffer, maxsize, "File could not be opened: %s", error);
		return SMCError_StreamOpen;
	}

	SMCError result = ParseStream_SMC(fp, FileStreamReader, smc_listener, states);

	fclose(fp);

	errstr = GetSMCErrorString(result);
	UTIL_Format(buffer, maxsize, "%s", errstr != NULL ? errstr : "Unknown error");

	return result;
}

struct RawStream
{
	const char *stream;
	size_t length;
	size_t pos;
};

bool RawStreamReader(void *stream, char *buffer, size_t maxlength, unsigned int *read)
{
	RawStream *rs = (RawStream *)stream;

	if (rs->pos >= rs->length)
	{
		return false;
	}

	size_t remaining = rs->length - rs->pos;

	/* Use the smaller of the two */
	size_t copy = (remaining > maxlength) ? maxlength : remaining;

	memcpy(buffer, &rs->stream[rs->pos], copy);
	rs->pos += copy;
	*read = copy;
	assert(rs->pos <= rs->length);

	return true;
}

SMCError TextParsers::ParseSMCStream(const char *stream,
									 size_t length,
									 ITextListener_SMC *smc_listener,
									 SMCStates *states,
									 char *buffer,
									 size_t maxsize)
{
	RawStream rs;
	SMCError result;

	rs.stream = stream;
	rs.length = length;
	rs.pos = 0;

	result = ParseStream_SMC(&rs, RawStreamReader, smc_listener, states);

	const char *errstr = GetSMCErrorString(result);
	UTIL_Format(buffer, maxsize, "%s", errstr != NULL ? errstr : "Unknown error");

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

	/* Do some extra work on strings that have special quoted characters. */
	if (data.special)
	{
		char *outptr = data.ptr;
		size_t len = data.end - data.ptr;
		if (len >= 2)
		{
			for (size_t i=0; i<len; i++)
			{
				if (data.ptr[i] == '\\' && i < len - 1)
				{
					/* Resolve the next character. */
					i++;
					if (data.ptr[i] == 'n')
					{
						data.ptr[i] = '\n';
					} else if (data.ptr[i] == 't') {
						data.ptr[i] = '\t';
					} else if (data.ptr[i] == 'r') {
						data.ptr[i] = '\r';
					} else if (data.ptr[i] != '\\'
						&& data.ptr[i] != '"') {
						/* This character is invalid, so go back one */
						i--;
					}
				}
				*outptr++ = data.ptr[i];
			}
			*outptr = '\0';
		}
	}

	if (data.end)
	{
		*(data.end) = '\0';
	}

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

SMCError TextParsers::ParseStream_SMC(void *stream, 
								   STREAMREADER srdr, 
								   ITextListener_SMC *smc, 
								   SMCStates *pStates)
{
	char *reparse_point = NULL;
	char in_buf[4096];
	char *parse_point = in_buf;
	char *line_begin = in_buf;
	unsigned int read;
	unsigned int curlevel = 0;
	bool in_quote = false;
	bool ignoring = false;
	bool eol_comment = false;
	bool ml_comment = false;
	unsigned int i;
	SMCError err = SMCError_Okay;
	SMCResult res;
	SMCStates states;
	char c;

	StringInfo strings[3];
	StringInfo emptystring;

	states.line = 1;
	states.col = 0;

	smc->ReadSMC_ParseStart();

	/**
	 * The stream reader reads in as much as it can fill the buffer with.
	 * It then processes the buffer.  If the buffer cannot be fully processed, for example, 
	 * a line is left hanging with no newline, then the contents of the buffer is shifted 
	 * down, and the buffer is filled from the stream reader again.
	 *
	 * What makes this particularly annoying is that we cache pointers everywhere, so when 
	 * the shifting process takes place, all those pointers must be shifted as well.
	 */
	while (srdr(stream, parse_point, sizeof(in_buf) - (parse_point - in_buf) - 1, &read))
	{
		if (!read)
		{
			break;
		}

		/* Check for BOM markings, which is only relevant on the first line.
		 * Not worth it, but it could be moved out of the loop.
		 */
		if (states.line == 1 && 
			in_buf[0] == (char)0xEF && 
			in_buf[1] == (char)0xBB && 
			in_buf[2] == (char)0xBF)
		{
			/* Move EVERYTHING down :\ */
			memmove(in_buf, &in_buf[3], read - 3);
			read -= 3;
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
						err = SMCError_InvalidTokens;
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

				/* Pass the raw line onto the listener.  We terminate the line so the receiver 
				 * doesn't get tons of useless info.  We restore the newline after.
				 */
				parse_point[i] = '\0';
				if ((res=smc->ReadSMC_RawLine(&states, line_begin)) != SMCResult_Continue)
				{
					err = (res == SMCResult_HaltFail) ? SMCError_Custom : SMCError_Okay;
					goto failed;
				}
				parse_point[i] = '\n';

				/* Now we check the sanity of our staged strings! */
				if (strings[2].ptr)
				{
					if (!curlevel)
					{
						err = SMCError_InvalidProperty1;
						goto failed;
					}
					/* Assume the next string is a property and pass the info on. */
					if ((res=smc->ReadSMC_KeyValue(
						&states,
						FixupString(strings[2]),
						FixupString(strings[1]))) != SMCResult_Continue)
					{
						err = (res == SMCResult_HaltFail) ? SMCError_Custom : SMCError_Okay;
						goto failed;
					}
					scrap(strings);
				}

				/* Change the states for the next line */
				states.col = 0;
				states.line++;
				line_begin = &parse_point[i+1];		//Note: safe because this gets relocated later
			} 
			else if (ignoring) 
			{
				if (in_quote)
				{
					/* If i was 0, we could have reparsed, so make sure there's no buffer underrun */
					if ((&parse_point[i] != in_buf) && c == '"' && parse_point[i-1] != '\\')
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
							err = SMCError_InvalidTokens;
							goto failed;
						}
					} 
					else if (c == '\\') 
					{
						strings[0].special = true;
						if (i == (read - 1))
						{
							reparse_point = &parse_point[i];
							break;
						}
					}
				} 
				else if (ml_comment) 
				{
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
							states.col++;
						}
					}
				}
			} 
			else 
			{
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
								reparse_point = &parse_point[i];
								break;
							}
							if (parse_point[i + 1] == '/')
							{
								/* standard comment */
								ignoring = true;
								eol_comment = true;
								restage = true;
							} 
							else if (parse_point[i+1] == '*') 
							{
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
						} 
						else 
						{
							ignoring = true;
							eol_comment = true;
							restage = true;
						}
					} 
					else if (c == '{') 
					{
						/* If we are staging a string, we must rotate here */
						if (strings[0].ptr)
						{
							/* We have unacceptable tokens on this line */
							if (rotate(strings) != NULL)
							{
								err = SMCError_InvalidSection1;
								goto failed;
							}
						}
						/* Sections must always be alone */
						if (strings[2].ptr != NULL)
						{
							err = SMCError_InvalidSection1;
							goto failed;
						} 
						else if (strings[1].ptr == NULL)
						{
							err = SMCError_InvalidSection2;
							goto failed;
						}
						if ((res=smc->ReadSMC_NewSection(&states, FixupString(strings[1])))
							!= SMCResult_Continue)
						{
							err = (res == SMCResult_HaltFail) ? SMCError_Custom : SMCError_Okay;
							goto failed;
						}
						strings[1] = emptystring;
						curlevel++;
					} 
					else if (c == '}') 
					{
						/* Unlike our matching friend, this can be on the same line as something prior */
						if (rotate(strings) != NULL)
						{
							err = SMCError_InvalidSection3;
							goto failed;
						}
						if (strings[2].ptr)
						{
							if (!curlevel)
							{
								err = SMCError_InvalidProperty1;
								goto failed;
							}
							if ((res=smc->ReadSMC_KeyValue(
											&states,
											FixupString(strings[2]),
											FixupString(strings[1])))
								!= SMCResult_Continue)
							{
								err = (res == SMCResult_HaltFail) ? SMCError_Custom : SMCError_Okay;
								goto failed;
							}
						} 
						else if (strings[1].ptr) 
						{
							err = SMCError_InvalidSection3;
							goto failed;
						} 
						else if (!curlevel) 
						{
							err = SMCError_InvalidSection4;
							goto failed;
						}
						/* Now it's safe to leave the section */
						scrap(strings);
						if ((res=smc->ReadSMC_LeavingSection(&states)) != SMCResult_Continue)
						{
							err = (res == SMCResult_HaltFail) ? SMCError_Custom : SMCError_Okay;
							goto failed;
						}
						curlevel--;
					} 
					else if (c == '"') 
					{
						/* If we get a quote mark, we always restage, but we need to do it beforehand */
						if (strings[0].ptr)
						{
							strings[0].end = &parse_point[i];
							if (rotate(strings) != NULL)
							{
								err = SMCError_InvalidTokens;
								goto failed;
							}
						}
						strings[0].ptr = &parse_point[i];
						in_quote = true;
						ignoring = true;
					} 
					else if (!strings[0].ptr) 
					{
						/* If we have no string, we must start one */
						strings[0].ptr = &parse_point[i];
					}
					if (restage && strings[0].ptr)
					{
						strings[0].end = &parse_point[i];
						if (rotate(strings) != NULL)
						{
							err = SMCError_InvalidTokens;
							goto failed;
						}
					}
				} 
				else 
				{
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
						} 
						else if (!strings[1].quoted) 
						{
							err = SMCError_InvalidTokens;
							goto failed;
						}
					}
				}
			}

			/* Advance which token we're on */
			states.col++;
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
		} 
		else if (read == sizeof(in_buf) - 1) 
		{
			err = SMCError_TokenOverflow;
			goto failed;
		}
	}

	/* If we're done parsing and there are tokens left over... */
	if (curlevel)
	{
		err = SMCError_InvalidSection5;
		goto failed;
	} 
	else if (strings[0].ptr || strings[1].ptr) 
	{
		err = SMCError_InvalidTokens;
		goto failed;
	}
	
	smc->ReadSMC_ParseEnd(false, false);
	
	if (pStates != NULL)
	{
		*pStates = states;
	}

	return SMCError_Okay;

failed:
	if (pStates != NULL)
	{
		*pStates = states;
	}

	smc->ReadSMC_ParseEnd(true, (err == SMCError_Custom));

	return err;
}


/**
 * INI parser 
 */

bool TextParsers::ParseFile_INI(const char *file, ITextListener_INI *ini_listener, unsigned int *line, unsigned int *col)
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

const char *TextParsers::GetSMCErrorString(SMCError err)
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
		"Section beginning without a matching ending",
		"Line contained too many invalid tokens",
		"Token buffer overflowed",
		"A property was declared outside of a section",
	};

	if (err < SMCError_Okay || err > SMCError_InvalidProperty1)
	{
		return NULL;
	}

	return s_errors[err];
}
