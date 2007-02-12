/**
 * vim: set ts=4 :
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
 
#if defined _textparse_included
 #endinput
#endif
#define _textparse_included


/********************************
 * Everything below describes the SMC Parse, or "SourceMod Configuration" format.
 * This parser is entirely event based.  You must hook events to receive data.
 * The file format itself is nearly identical to Valve's KeyValues format.
 ********************************/

enum SMCResult
{
	SMCParse_Continue,			/**< Continue parsing */
	SMCParse_Halt,				/**< Stop parsing here */
	SMCParse_HaltFail			/**< Stop parsing and return failure */
};

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
 * Creates a new SMC file format parser.  This is used to set parse hooks.
 *
 * @return			A new Handle to an SMC Parse structure.
 */
native Handle:SMC_CreateParser();

/**
 * Parses an SMC file.
 *
 * @param smc			A Handle to an SMC Parse structure.
 * @param file			A string containing the file path.
 * @param line			An optional by reference cell to store the last line number read.
 * @param col			An optional by reference cell to store the last column number read.
 * @return				An SMCParseError result.
 * @error				Invalid or corrupt Handle. 
 */
native SMCError:SMC_ParseFile(Handle:smc, const String:file[], &line=0, &col=0);

/**
 * Gets an error string for an SMCError code.
 * @note SMCError_Okay returns false.
 * @note SMCError_Custom (which is thrown on SMCParse_HaltFail) returns false.
 *
 * @param error			The SMCParseError code.
 * @param buffer		A string buffer for the error (contents undefined on failure).
 * @param buf_max		The maximum size of the buffer.
 * @return				True on success, false otherwise.
 */
native bool:SMC_GetErrorString(SMCError:error, String:buffer[], buf_max);

/**
 * Called when parsing is started.
 *
 * @param smc			The SMC Parse Handle.
 * @noreturn
 */
functag SMC_ParseStart public(Handle:smc);

/**
 * Sets the SMC_ParseStart function of a parse Handle.
 *
 * @param smc			Handle to an SMC Parse.
 * @param func			SMC_ParseStart function.
 * @noreturn
 * @error				Invalid or corrupt Handle. 
 */
native SMC_SetParseStart(Handle:smc, SMC_ParseStart:func);

/**
 * Called when parsing is halted.
 *
 * @param smc			The SMC Parse Handle.
 * @param halted		True if abnormally halted, false otherwise.
 * @param failed		True if parsing failed, false otherwise.
 * @noreturn
 */
functag SMC_ParseEnd public(Handle:smc, bool:halted, bool:failed);

/**
 * Sets the SMC_ParseEnd of a parse handle.
 *
 * @param smc			Handle to an SMC Parse.
 * @param func			SMC_ParseEnd function.
 * @noreturn
 * @error				Invalid or corrupt Handle. 
 */
native SMC_SetParseEnd(Handle:smc, SMC_ParseEnd:func);

/**
 * Called when the parser is entering a new section or sub-section.
 * @note Enclosing quotes are always stripped.
 *
 * @param smc			The SMC Parse Handle.
 * @param name			String containing section name.
 * @param opt_quotes	True if the section name was quote-enclosed in the file.
 * @return				An SMCResult action to take.
 */
functag SMC_NewSection SMCResult:public(Handle:smc, const String:name[], bool:opt_quotes);

/**
 * Called when the parser finds a new key/value pair.
 * @note Enclosing quotes are always stripped.
 *
 * @param smc			The SMC Parse Handle.
 * @param key			String containing key name.
 * @param value			String containing value name.
 * @param key_quotes	Whether or not the key was enclosed in quotes.
 * @param value_quotes	Whether or not the value was enclosed in quotes.
 * @return				An SMCResult action to take.
 */
functag SMC_KeyValue SMCResult:public(Handle:smc, const String:key[], const String:value[], bool:key_quotes, bool:value_quotes);

/**
 * Called when the parser finds the end of the current section.
 *
 * @param smc			The SMC Parse Handle.
 * @return				An SMCResult action to take.
 */
functag SMC_EndSection SMCResult:public(Handle:smc);

/**
 * Sets the three main reader functions.
 *
 * @param smc			An SMC parse Handle.
 * @param ns			An SMC_NewSection function pointer.
 * @param kv			An SMC_KeyValue function pointer.
 * @param es			An SMC_EndSection function pointer.
 * @noreturn
 */
native SMC_SetReaders(Handle:smc, SMC_NewSection:ns, SMC_KeyValue:kv, SMC_EndSection:es);

/**
 * Called whenever a raw line is read.
 *
 * @param smc			The SMC Parse Handle.
 * @param line			A string containing the raw line from the file.
 * @param lineno		The line number it occurs on.
 * @return				An SMCResult action to take.
 */
functag SMC_RawLine SMCResult:public(Handle:smc, const String:line[], lineno);

/**
 * Sets a raw line reader on an SMC parser Handle.
 *
 * @param smc			Handle to an SMC Parse.
 * @param func			SMC_RawLine function.
 * @noreturn
 */
native SMC_SetRawLine(Handle:smc, SMC_RawLine:func);
