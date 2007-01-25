#ifndef _INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_
#define _INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_

/**
 * This file parses plugin_settings.cfg and stores the information in cached memory.
 * It provides simplistic abstraction to retrieving the info.
 * :TODO: currently untested
 */

#include "sm_memtable.h"
#include "ITextParsers.h"
#include "IPluginSys.h"
#include "sm_globals.h"

struct PluginOpts
{
	int key;
	int val;
};

struct PluginSettings
{
	void Init();
	int name;
	bool pause_val;
	PluginType type_val;
	int optarray;
	size_t opts_num;
	size_t opts_size;
};

class CPluginInfoDatabase : public ITextListener_SMC
{
public:
	CPluginInfoDatabase();
	~CPluginInfoDatabase();
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
public:
	/**
	 * Returns the number of plugin settings available.
	 */
	unsigned int GetSettingsNum();

	/** 
	 * Given an index, returns the plugin settings block if the filename matches.
	 * Otherwise, returns NULL.
	 */
	PluginSettings *GetSettingsIfMatch(unsigned int index, const char *filename);

	/** 
	 *  Given a plugin settings struct and an index,
	 * returns the given JIT key/value option pair at that index.
	 * If the input is invalid, key and val will be set to NULL.
	 */
	void GetOptionsForPlugin(PluginSettings *settings, unsigned int opt_num, const char **key, const char **val);
private:
	SMCParseResult MakeError(const char *fmt, ...);
private:
	BaseStringTable *m_strtab;
	int m_errmsg;
	bool in_plugins;
	bool in_options;
	int m_infodb;
	size_t m_infodb_count;
	size_t m_infodb_size;
	int cur_plugin;
};


#endif //_INCLUDE_SOURCEMOD_CORE_SYSTEM_PLUGININFODATABASE_H_
