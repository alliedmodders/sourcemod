/**
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
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
	 * COMMENTS should be stripped, and are defined as text occuring in:
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
	 * Comments are text occuring inside the following tokens, and should be stripped
	 * unless they are inside literal strings:
	 *  ;<TEXT>
	 *  //<TEXT>
	 *  / *<TEXT> */

	/**
	 * @brief Lists actions to take when an SMC parse hook is done.
	 */
	enum SMCParseResult
	{
		SMCParse_Continue,		/**< Continue parsing */
		SMCParse_Halt,			/**< Stop parsing here */
		SMCParse_HaltFail		/**< Stop parsing and return SMCParseError_Custom */
	};

	/**
	 * @brief Lists error codes possible from parsing an SMC file.
	 */
	enum SMCParseError
	{
		SMCParse_Okay = 0,			/**< No error */
		SMCParse_StreamOpen,		/**< Stream failed to open */
		SMCParse_StreamError,		/**< The stream died... somehow */
		SMCParse_Custom,			/**< A custom handler threw an error */
		SMCParse_InvalidSection1,	/**< A section was declared without quotes, and had extra tokens */
		SMCParse_InvalidSection2,	/**< A section was declared without any header */
		SMCParse_InvalidSection3,	/**< A section ending was declared with too many unknown tokens */
		SMCParse_InvalidSection4,	/**< A section ending has no matching beginning */
		SMCParse_InvalidSection5,	/**< A section beginning has no matching ending */
		SMCParse_InvalidTokens,		/**< There were too many unidentifiable strings on one line */
		SMCParse_TokenOverflow,		/**< The token buffer overflowed */
		SMCParse_InvalidProperty1,	/**< A property was declared outside of any section */
	};

	/**
	 * @brief Describes the events available for reading an SMC stream.
	 */
	class ITextListener_SMC
	{
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
		 * @brief Called when a warning occurs.
		 * @param error				By-reference variable containing the error message of the warning.
		 * @param tokens			Pointer to the token stream causing the error.
		 * @return					SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_OnWarning(SMCParseError &error, const char *tokens)
		{
			return SMCParse_HaltFail;
		}

		/**
		 * @brief Called when entering a new section
		 *
		 * @param name			Name of section, with the colon omitted.
		 * @param opt_quotes	Whether or not the option string was enclosed in quotes.
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes)
		{
			return SMCParse_Continue;
		}

		/**
		 * @brief Called when encountering a key/value pair in a section.
		 * 
		 * @param key			Key string.
		 * @param value			Value string.  If no quotes were specified, this will be NULL, 
								and key will contain the entire string.
		 * @param key_quotes	Whether or not the key was in quotation marks.
		 * @param value_quotes	Whether or not the value was in quotation marks.
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_KeyValue(const char *key, 
												const char *value, 
												bool key_quotes, 
												bool value_quotes)
		{
			return SMCParse_Continue;
		}

		/**
		 * @brief Called when leaving the current section.
		 *
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_LeavingSection()
		{
			return SMCParse_Continue;
		}

		/**
		 * @brief Called after an input line has been preprocessed.
		 *
		 * @param line			String containing line input.
		 * @param curline		Number of line in file.
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_RawLine(const char *line, unsigned int curline)
		{
			return SMCParse_Continue;
		}
	};	

	#define SMINTERFACE_TEXTPARSERS_NAME		"ITextParsers"
	#define SMINTERFACE_TEXTPARSERS_VERSION		1

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
	public:
		/**
		 * @brief Parses an INI-format file.
		 *
		 * @param file			Path to file.
		 * @param ini_listener	Event handler for reading file.
		 * @param line			If non-NULL, will contain last line parsed (0 if file could not be opened).
		 * @param col			If non-NULL, will contain last column parsed (undefined if file could not be opened).
		 * @return				True if parsing succeded, false if file couldn't be opened or there was a syntax error.
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
		 * @param line			If non-NULL, will contain last line parsed (0 if file could not be opened).
		 * @param col			If non-NULL, will contain last column parsed (undefined if file could not be opened).
		 * @return				An SMCParseError result code.
		 */
		virtual SMCParseError ParseFile_SMC(const char *file, 
									ITextListener_SMC *smc_listener, 
									unsigned int *line, 
									unsigned int *col) =0;

		/**
		 * @brief Converts an SMCParseError to a stirng.
		 *
		 * @param err			SMCParseError.
		 * @return				String error message, or NULL if none.
		 */
		virtual const char *GetSMCErrorString(SMCParseError err) =0;

	public:
		/**
		 * @brief Returns the number of bytes that a multi-byte character contains in a UTF-8 stream.
		 * If the current character is not multi-byte, the function returns 1.
		 *
		 * @param stream		Pointer to multi-byte ANSI character string.
		 * @return				Number of bytes in current character.
		 */
		virtual unsigned int GetUTF8CharBytes(const char *stream) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_
