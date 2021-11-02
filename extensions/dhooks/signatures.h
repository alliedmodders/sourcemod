/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SIGNATURES_H_
#define _INCLUDE_SIGNATURES_H_

#include "extension.h"
#include "util.h"
#include <string>
#include <vector>
#include <sm_stringhashmap.h>

struct ArgumentInfo {
	ArgumentInfo() : name()
	{ }

	ArgumentInfo(std::string name, ParamInfo info) : name(name), info(info)
	{ }

	std::string name;
	ParamInfo info;
};

class SignatureWrapper {
public:
	std::string signature;
	std::string address;
	std::string offset;
	std::vector<ArgumentInfo> args;
	CallingConvention callConv;
	HookType hookType;
	ReturnType retType;
	ThisPointerType thisType;
};

class SignatureGameConfig : public ITextListener_SMC {
public:
	SignatureWrapper *GetFunctionSignature(const char *function);
public:
	//ITextListener_SMC
	SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
	SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
	SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	void ReadSMC_ParseStart();

private:
	ReturnType GetReturnTypeFromString(const char *str);
	HookParamType GetHookParamTypeFromString(const char *str);
	Register_t GetCustomRegisterFromString(const char *str);

private:
	StringHashMap<SignatureWrapper *> signatures_;
};

extern SignatureGameConfig *g_pSignatures;
#endif
