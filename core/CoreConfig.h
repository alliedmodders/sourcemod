/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_CORECONFIG_H_
#define _INCLUDE_SOURCEMOD_CORECONFIG_H_

#include "sm_globals.h"
#include <ITextParsers.h>
#include <IRootConsoleMenu.h>

using namespace SourceMod;

class CoreConfig : 
	public SMGlobalClass,
	public ITextListener_SMC,
	public IRootConsoleCommand
{
public: // SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: // ITextListener_SMC
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
public: // IRootConsoleCommand
	void OnRootConsoleCommand(const char *command, unsigned int argcount);
public:
	/**
	 * Initializes CoreConfig by reading from core.cfg file
	 */
	void Initialize();
private:
	/**
	 * Sets configuration option by notifying SourceMod components that rely on core.cfg
	 */
	ConfigResult SetConfigOption(const char *option, const char *value, ConfigSource, char *Error, size_t maxlength);
};

extern CoreConfig g_CoreConfig;

#endif // _INCLUDE_SOURCEMOD_CORECONFIG_H_
