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

#ifndef _INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_
#define _INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_

/**
 * @file IRootConsoleMenu.h
 * @brief Defines the interface for adding options to the "sm" console command.
 *
 *  This interface is not yet exported.  It will be eventually.  The initial reason should 
 * be obvious: we do not want users actually touching it.  If we exposed this, every little 
 * plugin would be dropping down a silly set of user commands, and exploiting/cluttering the menu.
 * Since this menu is explicitly provided for stuff that only Core itself is capable of managing,
 * we won't expose it until a legitimate reason comes up.
 */

namespace SourceMod
{
	/**
	 * @brief Handles a root console menu action.
	 */
	class IRootConsoleCommand
	{
	public:
		virtual void OnRootConsoleCommand(const char *command, unsigned int argcount) =0;
	};

	/**
	 * @brief Manages the root console menu - the "sm" command for servers.
	 */
	class IRootConsole
	{
	public:
		/**
		 * @brief Adds a root console command handler.  The command must be unique.
		 *
		 * @param cmd			String containing the console command.
		 * @param text			Description text.
		 * @param pHandler		An IRootConsoleCommand pointer to handle the command.
		 * @return				True on success, false on too many commands or duplicate command.
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
		 * @brief Returns the string of an argument.
		 *
		 * @param argno			The index of the argument.
		 * @return				A string containing the argument, or nothing if invalid.
		 */
		virtual const char *GetArgument(unsigned int argno) =0;

		/**
		 * @brief Returns the number of arguments.
		 *
		 * @return				Number of arguments.
		 */
		virtual unsigned int GetArgumentCount() =0;

		/** 
		 * @brief Returns the entire argument string.
		 *
		 * @return				String containing all arguments.
		 */
		virtual const char *GetArguments() =0;

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
	};
}

#endif //_INCLUDE_SOURCEMOD_ROOT_CONSOLE_MENU_H_
