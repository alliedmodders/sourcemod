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

#ifndef _INCLUDE_SOURCEMOD_TRANSLATOR_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_TRANSLATOR_INTERFACE_H_

#include <IShareSys.h>

#define SMINTERFACE_TRANSLATOR_NAME		"ITranslator"
#define SMINTERFACE_TRANSLATOR_VERSION	5

#define MAX_TRANSLATE_PARAMS		32
#define CORELANG_ENGLISH			0

/**
 * @file ITranslator.h
 * @brief Defines interfaces related to translation files.
 */

namespace SourceMod
{
	/**
	 * @brief SourceMod hardcodes the English language (default) to ID 0.
	 * This cannot be changed and languages.cfg should never have it as anything 
	 * other than the first index.
	 */
	#define SOURCEMOD_LANGUAGE_ENGLISH			0

	/**
	 * @brief For %T formats, specifies that the language should be that of the 
	 * server and not a specific client.
	 */
	#define SOURCEMOD_SERVER_LANGUAGE			0

	/**
	 * @brief Translation error codes.
	 */
	enum TransError
	{
		Trans_Okay = 0,					/**< Translation succeeded. */
		Trans_BadLanguage = 1,			/**< Bad language ID. */
		Trans_BadPhrase = 2,			/**< Phrase not found. */
		Trans_BadPhraseLanguage = 3,	/**< Phrase not found in the given language. */
		Trans_BadPhraseFile = 4,		/**< Phrase file was unreadable. */
	};

	/**
	 * @brief Contains information about a translation phrase.
	 */
	struct Translation
	{
		const char *szPhrase;		/**< Translated phrase. */
		unsigned int fmt_count;		/**< Number of format parameters. */
		int *fmt_order;				/**< Array of size fmt_count where each 
										 element is the numerical order of 
										 parameter insertion, starting from 
										 0.
										 */
	};

	/**
	 * @brief Represents a phrase file from SourceMod's "translations" folder.
	 */
	class IPhraseFile
	{
	public:
		/**
		 * @brief Attempts to find a translation phrase in a phrase file.
		 *
		 * @param szPhrase			String containing the phrase name.
		 * @param lang_id			Language ID.
		 * @param pTrans			Buffer to store translation info.
		 * @return					Translation error code indicating success 
		 *							(pTrans is filled) or failure (pTrans 
		 *							contents is undefined).
		 */
		virtual TransError GetTranslation(
			const char *szPhrase, 
			unsigned int lang_id, 
			Translation *pTrans) =0;

		/**
		 * @brief Returns the file name of this translation file.
		 *
		 * @return					File name.
		 */
		virtual const char *GetFilename() =0;
		
		/**
		 * @brief Determines if a translation phrase exists within
		 * the file.
		 *
		 * @param phrase				Phrase to search for.
		 * @return						True if the phrase was found.
		 */
		virtual bool TranslationPhraseExists(const char* phrase) =0;
	};

	/**
	 * Represents a collection of phrase files.
	 */
	class IPhraseCollection
	{
	public:
		/**
		 * @brief Adds a phrase file to the collection, using a cached one 
		 * if already found.  The return value is provided for informational 
		 * purposes and does not need to be saved.  The life time of the 
		 * return pointer is equal to the life time of the collection.
		 *
		 * This function will internally ignore dupliate additions but still 
		 * return a valid pointer.
		 *
		 * @param filename			File name, without the ".txt" extension, of 
		 *							the phrase file in the translations folder.
		 * @return					An IPhraseFile pointer, even if the file does 
		 *							not exist.
		 */
		virtual IPhraseFile *AddPhraseFile(const char *filename) =0;

		/**
		 * @brief Returns the number of contained phrase files.
		 *
		 * @return					Number of contained phrase files.
		 */
		virtual unsigned int GetFileCount() =0;
		
		/**
		 * @brief Returns the pointer to a contained phrase file.
		 *
		 * @param file				File index, from 0 to GetFileCount()-1.
		 * @return					IPhraseFile pointer, or NULL if out of 
		 *							range.
		 */
		virtual IPhraseFile *GetFile(unsigned int file) =0;

		/**
		 * @brief Destroys the phrase collection, freeing all internal 
		 * resources and invalidating the object.
		 */
		virtual void Destroy() =0;

		/**
		 * @brief Attempts a translation across a given language.  All 
		 * contained files are searched for an appropriate match; the 
		 * first valid match is returned.
		 *
		 * @param key				String containing the phrase name.
		 * @param langid			Language ID to translate to.
		 * @param pTrans			Translation buffer.
		 * @return					Translation error code; on success, 
		 *							pTrans is valid.  On failure, the 
		 *							contents of pTrans is undefined.
		 */
		virtual TransError FindTranslation(
			const char *key,
			unsigned int langid,
			Translation *pTrans) =0;

		/**
		 * @brief Formats a phrase given a parameter stack.  The parameter 
		 * stack size must exactly match the expected parameter count.  If 
		 * this count is too small or too large, the format fails.
		 *
		 * @param buffer			Buffer to store formatted text.
		 * @param maxlength			Maximum length of the buffer.
		 * @param format			String containing format information.  
		 *							This is equivalent to SourceMod's Format() 
		 *							native, and sub-translations are acceptable. 
		 * @param params			An array of pointers to each parameter. 
		 *							Integer parameters must have a pointer to the integer.
		 *							Float parameters must have a pointer to a float.
		 *							String parameters must be a string pointer.
		 *							Char parameters must be a pointer to a char.
		 *							Translation parameters fill multiple indexes in the 
		 *							array.  For %T translations, the expected stack is:
		 *							[phrase string pointer] [int target id pointer] [...] 
		 *							Where [...] is the required parameters for the translation,  
		 *							in the order expected by the phrase, not the phrase's 
		 *							translation.  For example, say the format is:
		 *							"%d %T" and the phrase's format is {1:s,2:f}, then the 
		 *							parameter stack should be:
		 *							int *, const char *, int *, const char *, float *
		 *							The %t modifier is the same except the target id pointer 
		 *							would be removed:
		 *							int *, const char *, const char *, float *
		 * @param numparams			Number of parameters in the params array.
		 * @param pOutLength		Optional pointer filled with output length on success.
		 * @param pFailPhrase		Optional pointer; on failure, is filled with NULL if the 
		 *							failure was not due to a failed translation phrase.  
		 *							Otherwise, it is filled with the given phrase name pointer 
		 *							from the parameter stack.  Undefined on success.
		 * @return					True on success.  False if the parameter stack was not 
		 *							exactly the right length, or if a translation phrase 
		 *							could not be found.
		 */
		virtual bool FormatString(
			char *buffer,
			size_t maxlength,
			const char *format,
			void **params,
			unsigned int numparams,
			size_t *pOutLength,
			const char **pFailPhrase) =0;
			
		/**
		 * @brief Determines if a translation phrase exists within
		 * the collection.
		 *
		 * @param phrase				Phrase to search for.
		 * @return						True if the phrase was found.
		 */
		virtual bool TranslationPhraseExists(const char *phrase) =0;
	};

	/**
	 * @brief Provides functions for translation.
	 */
	 class ITranslator : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName() =0;
		virtual unsigned int GetInterfaceVersion() =0;
	public:
		/**
		 * @brief Creates a new phrase collection object.
		 *
		 * @return			A new phrase collection object, which must be 
		 *					destroyed via IPhraseCollection::Destroy() when 
		 *					no longer needed.
		 */
		virtual IPhraseCollection *CreatePhraseCollection() =0;

		/**
		 * @brief Returns the server language.
		 *
		 * @return			Server language index.
		 */
		virtual unsigned int GetServerLanguage() =0;

		/**
		 * @brief Returns a client's language.
		 *
		 * @param client	Client index.
		 * @return			Client language index, or server's if client's is 
		 *					not known.
		 */
		virtual unsigned int GetClientLanguage(int client) =0;

		/**
		 * @brief Sets the global client SourceMod will use for assisted 
		 * translations (that is, %t).
		 *
		 * @param index		Client index (0 for server).
		 * @return			Old global client value.
		 */
		virtual int SetGlobalTarget(int index) =0;

		/**
		 * @brief Returns the global client SourceMod is currently using 
		 * for assisted translations (that is, %t).
		 *
		 * @return			Global client index (0 for server).
		 */
		virtual int GetGlobalTarget() const =0;

		/**
		 * @brief Formats a phrase given a parameter stack.  The parameter 
		 * stack size must exactly match the expected parameter count.  If 
		 * this count is too small or too large, the format fails.
		 *
		 * Note: This is the same as IPhraseCollection::FormatString(), except 
		 * that the IPhraseCollection parameter is explicit instead of implicit.
		 *
		 * @param buffer			Buffer to store formatted text.
		 * @param maxlength			Maximum length of the buffer.
		 * @param format			String containing format information.  
		 *							This is equivalent to SourceMod's Format() 
		 *							native, and sub-translations are acceptable. 
		 * @param pPhrases			Optional phrase collection pointer to search for 
		 *							phrases.
		 * @param params			An array of pointers to each parameter. 
		 *							Integer parameters must have a pointer to the integer.
		 *							Float parameters must have a pointer to a float.
		 *							String parameters must be a string pointer.
		 *							Char parameters must be a pointer to a char.
		 *							Translation parameters fill multiple indexes in the 
		 *							array.  For %T translations, the expected stack is:
		 *							[phrase string pointer] [int target id pointer] [...] 
		 *							Where [...] is the required parameters for the translation,  
		 *							in the order expected by the phrase, not the phrase's 
		 *							translation.  For example, say the format is:
		 *							"%d %T" and the phrase's format is {1:s,2:f}, then the 
		 *							parameter stack should be:
		 *							int *, const char *, int *, const char *, float *
		 *							The %t modifier is the same except the target id pointer 
		 *							would be removed:
		 *							int *, const char *, const char *, float *
		 * @param numparams			Number of parameters in the params array.
		 * @param pOutLength		Optional pointer filled with output length on success.
		 * @param pFailPhrase		Optional pointer; on failure, is filled with NULL if the 
		 *							failure was not due to a failed translation phrase.  
		 *							Otherwise, it is filled with the given phrase name pointer 
		 *							from the parameter stack.  Undefined on success.
		 * @return					True on success.  False if the parameter stack was not 
		 *							exactly the right length, or if a translation phrase 
		 *							could not be found.
		 */
		virtual bool FormatString(
			char *buffer,
			size_t maxlength,
			const char *format,
			IPhraseCollection *pPhrases,
			void **params,
			unsigned int numparams,
			size_t *pOutLength,
			const char **pFailPhrase) =0;

		/**
		 * @brief Get number of languages.
		 *
		 * @return                  Number of languages.
		 */
		virtual unsigned int GetLanguageCount() =0;

		/**
		 * @brief Find a language number by name.
		 *
		 * @param name              Language name.
		 * @param index             Optional pointer to store language index.
		 * @return                  True if found, false otherwise.
		 */
		virtual bool GetLanguageByName(const char *name, unsigned int *index) =0;

		/**
		 * @brief Retrieves info about a given language number.
		 *
		 * @param number            Language number.
		 * @param code              Pointer to store the language code.
		 * @param name              Pointer to store language name.
		 * @return                  True if language number is valid, false otherwise.
		 */
		virtual bool GetLanguageInfo(unsigned int number, const char **code, const char **name) =0;

		/**
		 * @brief Reparses all loaded translations files.
		 */
		virtual void RebuildLanguageDatabase() =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_TRANSLATOR_INTERFACE_H_

