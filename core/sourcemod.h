#ifndef _INCLUDE_SOURCEMOD_GLOBALHEADER_H_
#define _INCLUDE_SOURCEMOD_GLOBALHEADER_H_

#include "sm_globals.h"

class SourceModBase
{
public:
	/**
	 * @brief Initializes SourceMod, or returns an error on failure.
	 */
	bool InitializeSourceMod(char *error, size_t err_max, bool late);
};

extern SourceModBase g_SourceMod;

#endif //_INCLUDE_SOURCEMOD_GLOBALHEADER_H_
