#ifndef _INCLUDE_SOURCEMOD_TEXTPARSERS_H_
#define _INCLUDE_SOURCEMOD_TEXTPARSERS_H_

#include <IShareSys.h>

namespace SourceMod
{
	class ITextListener_INI
	{
	public:
		/**
		 * @brief Called when a new section is encountered in an INI file.
		 * 
		 * @param section		Name of section in between the [ and ] characters.
		 * @return				True to keep parsing, false otherwise.
		 */
		virtual bool ReadINI_NewSection(const char *section)
		{
			return true;
		}

		/**
		 * @brief Called when encountering a key/value pair in an INI file.
		 * 
		 * @param key			Name of key.
		 * @param value			Name of value.
		 * @return				True to keep parsing, false otherwise.
		 */
		virtual bool ReadINI_KeyValue(const char *key, const char *value)
		{
			return true;
		}
	};

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
		 * @return				SMCParseResult directive.
		 */
		virtual SMCParseResult ReadSMC_NewSection(const char *name, const char *option, bool colon)
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
	};	

	class ITextParsers : public SMInterface
	{
	public:
		virtual bool ParseFile_INI(const char *file, ITextListener_INI *ini_listener) =0;
		virtual bool ParseFile_SMC(const char *file, ITextListener_SMC *smc_listener) =0;
	};
};

#endif //_INCLUDE_SOURCEMOD_TEXTPARSERS_H_
