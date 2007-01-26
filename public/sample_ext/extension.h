#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Sample extension code header.
 */


#include "smsdk_ext.h"


/**
 * @brief Sample implementation of the SDK Extension.
 * Note: Uncomment one of the pre-defined virtual functions in order to use it.
 */
class Sample : public SDKExtension
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param err_max	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	//virtual bool SDK_OnLoad(char *error, size_t err_max, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	//virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	//virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param err_max	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	//virtual void QueryRunning(char *error, size_t maxlength);
public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * Read smext_base.h for documentation on these.
	 */

	//virtual bool SDK_OnMetamodLoad(char *error, size_t err_max, bool late);
	//virtual bool SDK_OnMetamodUnload(char *error, size_t err_max);
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t err_max);
#endif
};

#endif //_INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
