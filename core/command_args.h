// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2015 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _INCLUDE_SOURCEMOD_CCOMMANDARGS_IMPL_H_
#define _INCLUDE_SOURCEMOD_CCOMMANDARGS_IMPL_H_

#include "sourcemm_api.h"
#include <IRootConsoleMenu.h>
#include <compat_wrappers.h>
#include "logic_bridge.h"
#include <amtl/am-vector.h>
#include <amtl/am-string.h>

#if SOURCE_ENGINE==SE_EPISODEONE || SOURCE_ENGINE==SE_DARKMESSIAH
class EngineArgs : public ICommandArgs
{
public:
	EngineArgs(const CCommand& _cmd)
	{
	}
	const char *Arg(int n) const
	{
		return engine->Cmd_Argv(n);
	}
	int ArgC() const
	{
		return engine->Cmd_Argc();
	}
	const char *ArgS() const
	{
		return engine->Cmd_Args();
	}
};

class ArgsFromString : public ICommandArgs
{
public:
	ArgsFromString(const char *args) :
		arg_string_(nullptr)
	{
		size_t arglen = strlen(args);
		char *arg = new char[arglen + 1];
		arg[0] = 0;

		// Break all the arguments including the command name at whitespace.
		size_t pos = 0;
		const char *arg_pos = args;
		while ((pos = logicore.BreakString(arg_pos, arg, arglen + 1)) != -1)
		{
			args_.append(arg);

			// Skip the command name in the argument string.
			if (!arg_string_)
				arg_string_ = &args[pos];

			arg_pos += pos;
		}

		// Add the last argument as well.
		if (strlen(arg) > 0)
			args_.append(arg);

		// No arguments?
		if (!arg_string_)
			arg_string_ = &args[arglen];

		delete [] arg;
	}

	const char *Arg(int n) const
	{
		if (n < 0 || (size_t)n >= args_.length())
			return nullptr;

		return args_.at(n).chars();
	}
	int ArgC() const
	{
		return args_.length();
	}
	const char *ArgS() const
	{
		return arg_string_;
	}

private:
	ke::Vector<ke::AString> args_;
	const char *arg_string_;
};
#else
class EngineArgs : public ICommandArgs
{
	const CCommand *cmd;
public:
	EngineArgs(const CCommand& _cmd) : cmd(&_cmd)
	{
	}
	const char *Arg(int n) const
	{
		return cmd->Arg(n);
	}
	int ArgC() const
	{
		return cmd->ArgC();
	}
	const char *ArgS() const
	{
		return cmd->ArgS();
	}
};

class ArgsFromString : public ICommandArgs
{
public:
	ArgsFromString(const char *args)
	{
		cmd.Tokenize(args);
	}
	const char *Arg(int n) const
	{
		return cmd.Arg(n);
	}
	int ArgC() const
	{
		return cmd.ArgC();
	}
	const char *ArgS() const
	{
		return cmd.ArgS();
	}

private:
	CCommand cmd;
};
#endif

#endif // _INCLUDE_SOURCEMOD_CCOMMANDARGS_IMPL_H_
