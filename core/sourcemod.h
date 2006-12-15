#ifndef _INCLUDE_SOURCEMOD_GLOBALHEADER_H_
#define _INCLUDE_SOURCEMOD_GLOBALHEADER_H_

#include "sm_globals.h"

/**
 * @brief Implements SourceMod's global overall management, API, and logic
 */

class SourceModBase
{
public:
	SourceModBase();
public:
	/**
	 * @brief Initializes SourceMod, or returns an error on failure.
	 */
	bool InitializeSourceMod(char *error, size_t err_max, bool late);

	/** 
	 * @brief Starts everything SourceMod needs to run
	 */
	void StartSourceMod(bool late);

	/**
	 * @brief Map change hook
	 */
	bool LevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);

	/** 
	 * @brief Returns whether or not a mapload is in progress
	 */
	bool IsMapLoading();

	/**
	 * @brief Returns the base SourceMod folder.
	 */
	const char *GetSMBaseDir();

	/**
	 * @brief Returns whether our load in this map is late.
	 */
	bool IsLateLoadInMap();
private:
	/**
	 * @brief Loading plugins
	 */
	void DoGlobalPluginLoads();
private:
	char m_SMBaseDir[PLATFORM_MAX_PATH+1];
	bool m_IsMapLoading;
	bool m_IsLateLoadInMap;
};

/** 
 * @brief Any class deriving from this will be automatically initiated/shutdown by SourceMod
 */
class SMGlobalClass
{
	friend class SourceModBase;
public:
	SMGlobalClass();
public:
	/**
	 * @brief Called when SourceMod is initially loading
	 */
	virtual void OnSourceModStartup(bool late)
	{
	}

	/**
	 * @brief Called after all global classes have initialized
	 */
	virtual void OnSourceModAllInitialized()
	{
	}

	/**
	 * @brief Called when SourceMod is shutting down
	 */
	virtual void OnSourceModShutdown()
	{
	}
private:
	SMGlobalClass *m_pGlobalClassNext;
	static SMGlobalClass *head;
};

extern SourceModBase g_SourceMod;

#endif //_INCLUDE_SOURCEMOD_GLOBALHEADER_H_
