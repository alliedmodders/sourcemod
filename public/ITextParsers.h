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

#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_

/**
 * @file ITextParsers.h
 * @brief Defines various text/file parsing functions, as well as UTF-8 support code.
 */

#include <IShareSys.h>

namespace SourceMod
{

	#define SMINTERFACE_TEXTPARSERS_NAME		"ITextParsers"
	#define SMINTERFACE_TEXTPARSERS_VERSION		4

	/**
	 * The INI file format is defined as:
	 * WHITESPACE: 0x20, \n, \t, \r
	 * IDENTIFIER: A-Z a-z 0-9 _ - , + . $ ? / 
	 * STRING: Any set of symbols
	 * 
	 * Basic syntax is comprised of SECTIONs.
	 * A SECTION is defined as:
	 * [SECTIONNAME]
	 * OPTION
	 * OPTION
	 * OPTION...
	 *
	 * SECTIONNAME is an IDENTIFIER.
	 * OPTION can be repeated any number of times, once per line.
	 * OPTION is defined as one of:
	 *  KEY = "VALUE"
	 *  KEY = VALUE
	 *  KEY
	 * Where KEY is an IDENTIFIER and VALUE is a STRING.
	 * 
	 * WHITESPACE should always be omitted.
	 * COMMENTS should be stripped, and are defined as text occurring in:
	 * ;<TEXT>
	 * 
	 * Example file below.  Note that
	 * The second line is technically invalid.  The event handler
	 * must decide whether this should be allowed.
	 * --FILE BELOW--
	 * [gaben]
	 * hi = clams
	 * bye = "NO CLAMS"
	 *
	 * [valve]
	 * cannot
	 * maintain
	 * products
	 */

	/**
	 * @brief Contains parse events for INI files.
	 */
	class ITextListener_INI
	{
	public:
		/** 
		 * @brief Returns version number.
		 */
		virtual unsigned int GetTextParserVersion1()
		{
			return SMINTERFACE_TEXTPARSERS_VERSION;
		}
	public:
		/**
		 * @brief Called when a new section is encountered in an INI file.
		 * 
		 * @param section		Name of section in between the [ and ] characters.
		 * @param invalid_tokens True if invalid tokens were detected in the name.
		 * @param close_bracket	True if a closing bracket was detected, false otherwise.
		 * @param extra_tokens	True if extra tokens were detected on the line.
		 * @param curtok		Contains current token in the line where the section name starts.
		 *						You can add to this offset when failing to point to a token.
		 * @return				True to keep parsing, false otherwise.
		 */
		virtual bool ReadINI_NewSection(const char *section,
										bool invalid_tokens,
										bool close_bracket,
										bool extra_tokens,
										unsigned int *curtok)
		{
			return true;
		}

		/**
		 * @brief Called when encountering a key/value pair in an INI file.
		 * 
		 * @param key			Name of key.
		 * @param value			String containing value (with quotes stripped, if any).
		 * @param invalid_tokens Whether or not the key contained invalid tokens.
		 * @param equal_token	There was an '=' sign present (in case the value is missing).
		 * @param quotes		Whether value was enclosed in quotes.
		 * @param curtok		Contains the token index of the start of the value string.  
		 *						This can be changed when returning false.
		 * @return				True to keep parsing, false otherwise.
		 */
		virtual bool ReadINI_KeyValue(const char *key, 
									  const char *value, 
									  bool invalid_tokens,
									  bool equal_token,
									  bool quotes,
									  unsigned int *curtok)
		{
			return true;
		}

		/**
		 * @brief Called after a line has been preprocessed, if it has text.
		 *
		 * @param line			Contents of line.
		 * @param curtok		Pointer to optionally store failed position in string.
		 * @return				True to keep parsing, false otherwise.
		 */
		virtual bool ReadINI_RawLine(const char *line, unsigned int *curtok)
		{
			return true;
		}
	};

	/**
	 * :TODO: write this in CFG (context free grammar) format so it makes sense
	 * 
	 * The SMC file format is defined as:
	 * WHITESPACE: 0x20, \n, \t, \r
	 * IDENTIFIER: Any ASCII character EXCLUDING ", {, }, ;, //, / *, or WHITESPACE.
	 * STRING: Any set of symbols enclosed in quotes.
	 * Note: if a STRING does not have quotes, it is parsed as an IDENTIFIER.
	 *
	 * Basic syntax is comprised of SECTIONBLOCKs.
	 * A SECTIONBLOCK defined as:
	 *
	 * SECTIONNAME
	 * {
	 *    OPTION
	 * }
	 * 
	 * OPTION can be repeated any number of times inside a SECTIONBLOCK.
	 * A new line will terminate an OPTION, but there can be more than one OPTION per line.
	 * OPTION is defined any of:
	 * 	  "KEY"  "VALUE"
	 *    SECTIONBLOCK
	 *
	 * SECTIONNAME, KEY, VALUE, and SINGLEKEY are strings
	 * SECTIONNAME cannot have trailing characters if quoted, but the quotes can be optionally removed.
	 * If SECTIONNAME is not enclosed in quotes, the entire sectionname string is used (minus surrounding whitespace).
	 * If KEY is not enclosed in quotes, the key is terminated at first whitespace.
	 * If VALUE is not properly enclosed in quotes, the entire value string is used (minus surrounding whitespace).
	 * The VALUE may have inner quotes, but the key string may not.
	 *
	 * For an example, see configs/permissions.cfg
	 *
	 * WHITESPACE should be ignored.
	 * Comments are text occurring inside the following tokens, and should be stripped
	 * unless they are inside literal strings:
	 *  ;<TEXT>
	 *  //<TEXT>
	 *  / *<TEXT> */

	/**
	 * @brief Lists actions to take when an SMC parse hook is done.
	 */
	enum SMCResult
	{
		SMCResult_Continue,		/**< Continue parsing */
		SMCResult_Halt,			/**< Stop parsing here */
		SMCResult_HaltFail		/**< Stop parsing and return SMCError_Custom */
	};

	/**
	 * @brief Lists error codes possible from parsing an SMC file.
	 */
	enum SMCError
	{
		SMCError_Okay = 0,			/**< No error */
		SMCError_StreamOpen,		/**< Stream failed to open */
		SMCError_StreamError,		/**< The stream died... somehow */
		SMCError_Custom,			/**< A custom handler threw an error */
		SMCError_InvalidSection1,	/**< A section was declared without quotes, and had extra tokens */
		SMCError_InvalidSection2,	/**< A section was declared without any header */
		SMCError_InvalidSection3,	/**< A section ending was declared with too many unknown tokens */
		SMCError_InvalidSection4,	/**< A section ending has no matching beginning */
		SMCError_InvalidSection5,	/**< A section beginning has no matching ending */
		SMCError_InvalidTokens,		/**< There were too many unidentifiable strings on one line */
		SMCError_TokenOverflow,		/**< The token buffer overflowed */
		SMCError_InvalidProperty1,	/**< A property was declared outside of any section */
	};

	/**
	 * @brief States for line/column
	 */
	struct SMCStates
	{
		unsigned int line;			/**< Current line */
		unsigned int col;			/**< Current col */
	};

	/**
	 * @brief Describes the events available for reading an SMC stream.
	 */
	class ITextListener_SMC
	{
	public:
		/** 
		 * @brief Returns version number.
		 */
		virtual unsigned int GetTextParserVersion2()
		{
			return SMINTERFACE_TEXTPARSERS_VERSION;
		}
	public:
		/**
		 * @brief Called when starting parsing.
		 */
		virtual void ReadSMC_ParseStart()
		{
		};

		/**
		 * @brief Called when ending parsing.
		 *
		 * @param halted			True if abnormally halted, false otherwise.
		 * @param failed			True if parsing failed, false otherwise.
		 */
		virtual void ReadSMC_ParseEnd(bool halted, bool failed)
		{
		}

		/**
		 * @brief Called when entering a new section
		 *
		 * @param states		Parsing states.
		 * @param name			Name of section, with the colon omitted.
		 * @return				SMCResult directive.
		 */
		virtual SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
		{
			return SMCResult_Continue;
		}

		/**
		 * @brief Called when encountering a key/value pair in a section.
		 * 
		 * @param states		Parsing states.
		 * @param key			Key string.
		 * @param value			Value string.  If no quotes were specified, this will be NULL, 
		 *						and key will contain the entire string.
		 * @return				SMCResult directive.
		 */
		virtual SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
		{
			return SMCResult_Continue;
		}

		/**
		 * @brief Called when leaving the current section.
		 *
		 * @param states		Parsing states.
		 * @return				SMCResult directive.
		 */
		virtual SMCResult ReadSMC_LeavingSection(const SMCStates *states)
		{
			return SMCResult_Continue;
		}

		/**
		 * @brief Called after an input line has been preprocessed.
		 *
		 * @param states		Parsing states.
		 * @param line			Contents of the line, null terminated at the position 
		 * 						of the newline character (thus, no newline will exist).
		 * @return				SMCResult directive.
		 */
		virtual SMCResult ReadSMC_RawLine(const SMCStates *states, const char *line)
		{
			return SMCResult_Continue;
		}
	};	

	/**
	 * @brief Contains various text stream parsing functions.
	 */
	class ITextParsers : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_TEXTPARSERS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_TEXTPARSERS_VERSION;
		}
		virtual bool IsVersionCompatible(unsigned int version)
		{
			if (version < 2)
			{
				return false;
			}
			return SMInterface::IsVersionCompatible(version);
		}
	public:
		/**
		 * @brief Parses an INI-format file.
		 *
		 * @param file			Path to file.
		 * @param ini_listener	Event handler for reading file.
		 * @param line			If non-NULL, will contain last line parsed (0 if file could not be opened).
		 * @param col			If non-NULL, will contain last column parsed (undefined if file could not be opened).
		 * @return				True if parsing succeeded, false if file couldn't be opened or there was a syntax error.
		 */
		virtual bool ParseFile_INI(const char *file, 
									ITextListener_INI *ini_listener,
									unsigned int *line,
									unsigned int *col) =0;

		/**
		 * @brief Parses an SMC-format text file.
		 * Note that the parser makes every effort to obey broken syntax.
		 * For example, if an open brace is missing, but the section name has a colon,
		 * it will let you know.  It is up to the event handlers to decide whether to be strict or not.
		 *
		 * @param file			Path to file.
		 * @param smc_listener	Event handler for reading file.
		 * @param states		Optional pointer to store last known states.
		 * @return				An SMCError result code.
		 */
		virtual SMCError ParseFile_SMC(const char *file, 
									ITextListener_SMC *smc_listener, 
									SMCStates *states) =0;

		/**
		 * @brief Converts an SMCError to a string.
		 *
		 * @param err			SMCError.
		 * @return				String error message, or NULL if none.
		 */
		virtual const char *GetSMCErrorString(SMCError err) =0;

	public:
		/**
		 * @brief Returns the number of bytes that a multi-byte character contains in a UTF-8 stream.
		 * If the current character is not multi-byte, the function returns 1.
		 *
		 * @param stream		Pointer to multi-byte ANSI character string.
		 * @return				Number of bytes in current character.
		 */
		virtual unsigned int GetUTF8CharBytes(const char *stream) =0;

		/**
		 * @brief Returns whether the first multi-byte character in the given stream
		 * is a whitespace character.
		 *
		 * @param stream		Pointer to multi-byte character string.
		 * @return				True if first character is whitespace, false otherwise.
		 */
		virtual bool IsWhitespace(const char *stream) =0;

		/**
		 * @brief Same as ParseFile_SMC, but with an extended error buffer.
		 *
		 * @param file			Path to file.
		 * @param smc_listener	Event handler for reading file.
		 * @param states		Optional pointer to store last known states.
		 * @param buffer		Error message buffer.
		 * @param maxsize		Maximum size of the error buffer.
		 * @return 				Error code.
		 */
		virtual SMCError ParseSMCFile(const char *file,
			ITextListener_SMC *smc_listener,
			SMCStates *states,
			char *buffer,
			size_t maxsize) =0;

		/**
		 * @brief Parses a raw UTF8 stream as an SMC file.
		 *
		 * @param stream		Memory containing data.
		 * @param length		Number of bytes in the stream.
		 * @param smc_listener	Event handler for reading file.
		 * @param states		Optional pointer to store last known states.
		 * @param buffer		Error message buffer.
		 * @param maxsize		Maximum size of the error buffer.
		 * @return 				Error code.
		 */
		virtual SMCError ParseSMCStream(const char *stream,
			size_t length,
			ITextListener_SMC *smc_listener,
			SMCStates *states,
			char *buffer,
			size_t maxsize) =0;
	};

	inline unsigned int _GetUTF8CharBytes(const char *stream)
	{
		unsigned char c = *(unsigned char *)stream;
		if (c & (1<<7))
		{
			if (c & (1<<5))
			{
				if (c & (1<<4))
				{
					return 4;
				}
				return 3;
			}
			return 2;
		}
		return 1;
	}
}

extern SourceMod::ITextParsers *textparsers;

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_

