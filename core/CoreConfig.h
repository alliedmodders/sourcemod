/**
 * vim: set ts=4 :
 * ================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
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
	void OnSourceModLevelChange(const char *mapName);
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

extern bool SM_AreConfigsExecuted();
extern void SM_ExecuteAllConfigs();
extern void SM_ExecuteForPlugin(IPluginContext *ctx);
extern void SM_ConfigsExecuted_Global();
extern void SM_ConfigsExecuted_Plugin(unsigned int serial);

extern CoreConfig g_CoreConfig;

#endif // _INCLUDE_SOURCEMOD_CORECONFIG_H_
