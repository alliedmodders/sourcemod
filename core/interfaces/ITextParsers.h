#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_

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
		 * @param curtoken		Contains the token index of the start of the value string.  
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
		virtual bool ReadINI_RawLine(const char *line, unsigned int *cutok)
		{
			return true;
		}
	};

	/**
	 * :TODO: write this in CFG format so it makes sense
	 * 
	 * The SMC file format is defined as:
	 * WHITESPACE: 0x20, \n, \t, \r
	 * IDENTIFIER: Any ASCII character EXCLUDING ", ', :, WHITESPACE
	 * STRING: Any set of symbols
	 *
	 * Basic syntax is comprised of SECTIONBLOCKs.
	 * A SECTIONBLOCK defined as:
	 *
	 * SECTION: "SECTIONNAME"
	 * {
	 *    OPTION
	 * }
	 * 
	 * OPTION can be repeated any number of times inside a SECTIONBLOCK.
	 * A new line will terminate an OPTION, but there can be more than one OPTION per line.
	 * OPTION is defined any of:
	 * 	  "KEY"  "VALUE"
	 *    "SINGLEKEY"
	 *    SECTIONBLOCK
	 *
	 * SECTION is an IDENTIFIER
	 * SECTIONNAME, KEY, VALUE, and SINGLEKEY are strings
	 *
	 * For an example, see configs/permissions.cfg
	 *
	 * WHITESPACE should be ignored.
	 * Comments are text occuring inside the following tokens, and should be stripped
	 * unless they are inside literal strings:
	 *  ;<TEXT>
	 *  //<TEXT>
	 *  /*<TEXT> */
	class ITextListener_SMC
	{
	public:
		enum SMCParseResult
		{
			SMCParse_Continue,
			SMCParse_SkipSection,
			SMCParse_Halt,
			SMCParse_HaltFail
		};

		/**
		 * @brief Called when entering a new section
		 *
		 * @param name			Name of section, with the colon omitted.
		 * @param option		Optional text after the colon, quotes removed.  NULL if none.
		 * @param colon			Whether or not the required ':' was encountered.
		 * @param start_token	Whether a start token was detected.
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_NewSection(const char *name, 
													const char *option, 
													bool colon, 
													bool start_token)
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
		 * @param end_token		Whether an end token was detected.
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_LeavingSection(bool end_token)
		{
			return SMCParse_Continue;
		}
	};	

	#define SMINTERFACE_TEXTPARSERS_NAME		"ITextParsers"
	#define SMINTERFACE_TEXTPARSERS_VERSION		1

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
		 * @return				True if parsing succeded, false if file couldn't be opened or there was a syntax error.
		 */
		virtual bool ParseFile_SMC(const char *file, 
									ITextListener_SMC *smc_listener, 
									unsigned int *line, 
									unsigned int *col) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_INTERFACE_H_
