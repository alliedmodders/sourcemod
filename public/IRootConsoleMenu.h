/**
 * vim: set ts=4 sw=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_
#define _INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_

/**
 * @file IRootConsoleMenu.h
 * @brief Defines the interface for adding options to the "sm" console command.
 *
 * You must be using the compat_wrappers.h header file to use this on 
 * Original/Episode1 builds.  
 */

#define SMINTERFACE_ROOTCONSOLE_NAME			"IRootConsole"
#define SMINTERFACE_ROOTCONSOLE_VERSION			3

class CCommand;

namespace SourceMod
{
	/**
	 * @brief Wrapper around CCommand.
	 */
	class ICommandArgs
	{
	public:
		/**
		 * @brief Returns an argument by number.
		 *
		 * @brief n		Argument number.
		 * @return		Argument string.
		 */
		virtual const char *Arg(int n) const = 0;

		/**
		 * @brief Returns the argument count.
		 *
		 * @return		Argument count.
		 */
		virtual int ArgC() const = 0;

		/**
		 * @brief Returns the full argument string.
		 *
		 * @return		Argument string.
		 */
		virtual const char *ArgS() const = 0;
	};

	/**
	 * @brief Handles a root console menu action.
	 */
	class IRootConsoleCommand
	{
	public:
		virtual unsigned int GetApiVersion() const {
            return SMINTERFACE_ROOTCONSOLE_VERSION;
        }

		virtual void OnRootConsoleCommand(const char *cmdname, const ICommandArgs *args) = 0;
	};

	/**
	 * @brief Manages the root console menu - the "sm" command for servers.
	 */
	class IRootConsole : public SMInterface
	{
	public:
		/**
		 * @brief Removed.
		 *
		 * @param cmd			Unused.
		 * @param text			Unused.
		 * @param pHandler		Unused.
		 * @return				False.
		 */
		virtual bool AddRootConsoleCommand(const char *cmd, const char *text, IRootConsoleCommand *pHandler) =0;

		/**
		 * @brief Removes a root console command handler.
		 *
		 * @param cmd			String containing the console command.
		 * @param pHandler		An IRootConsoleCommand pointer for verification.
		 * @return				True on success, false otherwise.
		 */
		virtual bool RemoveRootConsoleCommand(const char *cmd, IRootConsoleCommand *pHandler) =0;

		/**
		 * @brief Prints text back to the console.
		 *
		 * @param fmt			Format of string.
		 * @param ...			Format arguments.
		 */
		virtual void ConsolePrint(const char *fmt, ...) =0;

		/**
		 * @brief Draws a generic command/description pair.
		 * NOTE: The pair is currently four spaces indented and 16-N spaces of separation,
		 * N being the length of the command name.  This is subject to change in case we
		 * account for Valve's font choices.
		 *
		 * @param cmd		String containing the command option.
		 * @param text		String containing the command description.
		 */
		virtual void DrawGenericOption(const char *cmd, const char *text) =0;

		/**
		 * @brief Removed.
		 *
		 * @param cmd			Unused.
		 * @param text			Unused.
		 * @param pHandler		Unused.
		 * @return				False.
		 */
		virtual bool AddRootConsoleCommand2(const char *cmd,
											const char *text,
											IRootConsoleCommand *pHandler) =0;

		/**
		 * @brief Adds a root console command handler.  The command must be unique.
		 *
		 * @param cmd			String containing the console command.
		 * @param text			Description text.
		 * @param pHandler		An IRootConsoleCommand pointer to handle the command.
		 * @return				True on success, false on too many commands or duplicate command.
		 */
		virtual bool AddRootConsoleCommand3(const char *cmd,
											const char *text,
											IRootConsoleCommand *pHandler) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_

