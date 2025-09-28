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

#include "signatures.hpp"
#include "globals.hpp"

namespace dhooks {

SignatureGameConfig* dhooks_config = nullptr;
SignatureWrapper* g_CurrentSignature;

enum class ParseState
{
	None,
	Root,
	Function,
	Arguments,
	Argument
};

ParseState g_ParseState;
unsigned int g_IgnoreLevel;
// The parent section type of a platform specific "windows" or "linux" section.
ParseState g_PlatformOnlyState;
std::string g_CurrentFunctionName;
ArgumentInfo g_CurrentArgumentInfo;

SignatureWrapper* SignatureGameConfig::GetFunctionSignature(const char* function)
{
	auto sig = signatures_.find(function);
	if (sig == signatures_.end())
		return nullptr;

	return (*sig).second;
}

/**
 * Game config "Functions" section parsing.
 */
SourceMod::SMCResult SignatureGameConfig::ReadSMC_NewSection(const SourceMod::SMCStates *states, const char *name)
{
	// We're ignoring the parent section. Ignore all child sections as well.
	if (g_IgnoreLevel > 0)
	{
		g_IgnoreLevel++;
		return SourceMod::SMCResult_Continue;
	}

	// Handle platform specific sections first.
#ifdef DHOOKS_X86_64
#if defined WIN32
	if (!strcmp(name, "windows64"))
#elif defined _LINUX
	if (!strcmp(name, "linux64"))
#elif defined _OSX
	if (!strcmp(name, "mac64"))
#endif
#else
#if defined WIN32
	if (!strcmp(name, "windows"))
#elif defined _LINUX
	if (!strcmp(name, "linux"))
#elif defined _OSX
	if (!strcmp(name, "mac"))
#endif
#endif
	{
		// We're already in a section for a different OS that we're ignoring. Can't have a section for our OS in here.
		if (g_IgnoreLevel > 0)
		{
			globals::sourcemod->LogError(globals::myself, "Unreachable platform specific section in \"%s\" Function: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}

		// We don't support nested (useless) sections of the same OS like "windows" { "windows" { "foo" "bar" } }
		if (g_PlatformOnlyState != ParseState::None)
		{
			globals::sourcemod->LogError(globals::myself, "Duplicate platform specific section for \"%s\". Already parsing only for that OS: line: %i col: %i", name, states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}

		// This is a specific block for us.
		g_PlatformOnlyState = g_ParseState;
		return SourceMod::SMCResult_Continue;
	}
	else if (!strcmp(name, "windows") || !strcmp(name, "linux") || !strcmp(name, "mac")
	|| !strcmp(name, "windows64") || !strcmp(name, "linux64") || !strcmp(name, "mac64"))
	{
		if (g_PlatformOnlyState != ParseState::None)
		{
			globals::sourcemod->LogError(globals::myself, "Unreachable platform specific section in \"%s\" Function: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}

		// A specific block for a different platform.
		g_IgnoreLevel++;
		return SourceMod::SMCResult_Continue;
	}

	switch (g_ParseState) {
		case ParseState::Root: {
			auto sig = signatures_.find(name);
			if (sig != signatures_.end())
				g_CurrentSignature = (*sig).second;
			else
				g_CurrentSignature = new SignatureWrapper();
			g_CurrentFunctionName = name;
			g_ParseState = ParseState::Function;
			break;
		}

		case ParseState::Function: {
			if (!strcmp(name, "arguments"))
			{
				g_ParseState = ParseState::Arguments;
			}
			else
			{
				globals::sourcemod->LogError(globals::myself, "Unknown subsection \"%s\" (expected \"arguments\"): line: %i col: %i", name, states->line, states->col);
				return SourceMod::SMCResult_HaltFail;
			}
			break;
		}
		case ParseState::Arguments: {
			g_ParseState = ParseState::Argument;
			g_CurrentArgumentInfo.name = name;

			// Reset the parameter info.
			ParamInfo info;
			info.type = sp::HookParamType_Unknown;
			info.size = 0;
			info.custom_register = sp::DHookRegister_Default;
			info.flags = sp::DHookPass_ByVal;
			g_CurrentArgumentInfo.info = info;

			// See if we already have info about this argument.
			for (auto &arg : g_CurrentSignature->args) {
				if (!arg.name.compare(name)) {
					// Continue changing that argument now.
					g_CurrentArgumentInfo.info = arg.info;
					break;
				}
			}
			break;
		}
		default: {
			globals::sourcemod->LogError(globals::myself, "Unknown subsection \"%s\": line: %i col: %i", name, states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}
	}

	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult SignatureGameConfig::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	// We don't care for anything in this section or subsections.
	if (g_IgnoreLevel > 0)
		return SourceMod::SMCResult_Continue;

	switch (g_ParseState)
	{
		case ParseState::Function: {
			if (!strcmp(key, "signature"))
			{
				if (g_CurrentSignature->address.length() > 0 || g_CurrentSignature->offset.length() > 0)
				{
					globals::sourcemod->LogError(globals::myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
				g_CurrentSignature->signature = value;
			}
			else if (!strcmp(key, "address"))
			{
				if (g_CurrentSignature->signature.length() > 0 || g_CurrentSignature->offset.length() > 0)
				{
					globals::sourcemod->LogError(globals::myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
				g_CurrentSignature->address = value;
			}
			else if (!strcmp(key, "offset"))
			{
				if (g_CurrentSignature->address.length() > 0 || g_CurrentSignature->signature.length() > 0)
				{
					globals::sourcemod->LogError(globals::myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
				g_CurrentSignature->offset = value;
			}
			else if (!strcmp(key, "callconv"))
			{
				sp::CallingConvention callConv;

				if (!strcmp(value, "cdecl"))
					callConv = sp::CallConv_CDECL;
				else if (!strcmp(value, "thiscall"))
					callConv = sp::CallConv_THISCALL;
				else if (!strcmp(value, "stdcall"))
					callConv = sp::CallConv_STDCALL;
				else if (!strcmp(value, "fastcall"))
					callConv = sp::CallConv_FASTCALL;
				else
				{
					globals::sourcemod->LogError(globals::myself, "Invalid calling convention \"%s\": line: %i col: %i", value, states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}

				g_CurrentSignature->callConv = callConv;
			}
			else if (!strcmp(key, "hooktype"))
			{
				sp::HookType hookType;

				if (!strcmp(value, "entity"))
					hookType = sp::HookType_Entity;
				else if (!strcmp(value, "gamerules"))
					hookType = sp::HookType_GameRules;
				else if (!strcmp(value, "raw"))
					hookType = sp::HookType_Raw;
				else
				{
					globals::sourcemod->LogError(globals::myself, "Invalid hook type \"%s\": line: %i col: %i", value, states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}

				g_CurrentSignature->hookType = hookType;
			}
			else if (!strcmp(key, "return"))
			{
				g_CurrentSignature->retType = GetReturnTypeFromString(value);

				if (g_CurrentSignature->retType == sp::ReturnType_Unknown)
				{
					globals::sourcemod->LogError(globals::myself, "Invalid return type \"%s\": line: %i col: %i", value, states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
			else if (!strcmp(key, "this"))
			{
				if (!strcmp(value, "ignore"))
					g_CurrentSignature->thisType = sp::ThisPointer_Ignore;
				else if (!strcmp(value, "entity"))
					g_CurrentSignature->thisType = sp::ThisPointer_CBaseEntity;
				else if (!strcmp(value, "address"))
					g_CurrentSignature->thisType = sp::ThisPointer_Address;
				else
				{
					globals::sourcemod->LogError(globals::myself, "Invalid this type \"%s\": line: %i col: %i", value, states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
			else
			{
				globals::sourcemod->LogError(globals::myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
				return SourceMod::SMCResult_HaltFail;
			}
			break;
		}
		case ParseState::Argument: {
			if (!strcmp(key, "type"))
			{
				g_CurrentArgumentInfo.info.type = GetHookParamTypeFromString(value);
				if (g_CurrentArgumentInfo.info.type == sp::HookParamType_Unknown)
				{
					globals::sourcemod->LogError(globals::myself, "Invalid argument type \"%s\" for argument \"%s\": line: %i col: %i", value, g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
			else if (!strcmp(key, "size"))
			{
				g_CurrentArgumentInfo.info.size = static_cast<int>(strtol(value, NULL, 0));

				if (g_CurrentArgumentInfo.info.size < 1)
				{
					globals::sourcemod->LogError(globals::myself, "Invalid argument size \"%s\" for argument \"%s\": line: %i col: %i", value, g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
			else if (!strcmp(key, "flags"))
			{
				sp::DHookPassFlag flags = static_cast<sp::DHookPassFlag>(0);
				if (strstr(value, "byval"))
					flags = static_cast<sp::DHookPassFlag>(static_cast<int>(flags)|sp::DHookPass_ByVal);
				if (strstr(value, "byref"))
					flags = static_cast<sp::DHookPassFlag>(static_cast<int>(flags)|sp::DHookPass_ByRef);
	/*
				if (strstr(value, "odtor"))
					flags |= PASSFLAG_ODTOR;
				if (strstr(value, "octor"))
					flags |= PASSFLAG_OCTOR;
				if (strstr(value, "oassignop"))
					flags |= PASSFLAG_OASSIGNOP;
	#ifdef PASSFLAG_OCOPYCTOR
				if (strstr(value, "ocopyctor"))
					flags |= PASSFLAG_OCOPYCTOR;
	#endif
	#ifdef PASSFLAG_OUNALIGN
				if (strstr(value, "ounalign"))
					flags |= PASSFLAG_OUNALIGN;
	#endif
	*/
				g_CurrentArgumentInfo.info.flags = flags;
			}
			else if (!strcmp(key, "register"))
			{
				g_CurrentArgumentInfo.info.custom_register = GetCustomRegisterFromString(value);

				if (g_CurrentArgumentInfo.info.custom_register == sp::DHookRegister_Default)
				{
					globals::sourcemod->LogError(globals::myself, "Invalid register \"%s\": line: %i col: %i", value, states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
			else
			{
				globals::sourcemod->LogError(globals::myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
				return SourceMod::SMCResult_HaltFail;
			}
			break;
		}
		default: {
			globals::sourcemod->LogError(globals::myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}
	}
	return SourceMod::SMCResult_Continue;
}

SourceMod::SMCResult SignatureGameConfig::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	// We were ignoring this section.
	if (g_IgnoreLevel > 0)
	{
		g_IgnoreLevel--;
		return SourceMod::SMCResult_Continue;
	}

	// We were in a section only for our OS.
	if (g_PlatformOnlyState == g_ParseState)
	{
		g_PlatformOnlyState = ParseState::None;
		return SourceMod::SMCResult_Continue;
	}

	switch (g_ParseState)
	{
	case ParseState::Function:
		g_ParseState = ParseState::Root;

		if (!g_CurrentSignature->address.length() && !g_CurrentSignature->signature.length() && !g_CurrentSignature->offset.length())
		{
			globals::sourcemod->LogError(globals::myself, "Function \"%s\" doesn't have a \"signature\", \"offset\" nor \"address\" set: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}

		if (!g_CurrentSignature->offset.length())
		{
			// DynamicDetours doesn't expose the passflags concept like SourceHook.
			// See if we're trying to set some invalid flags on detour arguments.
			for (auto &arg : g_CurrentSignature->args)
			{
				if ((arg.info.flags & ~sp::DHookPass_ByVal) > 0)
				{
					globals::sourcemod->LogError(globals::myself, "Function \"%s\" uses unsupported pass flags in argument \"%s\". Flags are only supported for virtual hooks: line: %i col: %i", g_CurrentFunctionName.c_str(), arg.name.c_str(), states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
		}

		// Save this function signature in our cache.
		signatures_[g_CurrentFunctionName] = g_CurrentSignature;
		g_CurrentFunctionName = "";
		g_CurrentSignature = nullptr;
		break;
	case ParseState::Arguments:
		g_ParseState = ParseState::Function;
		break;
	case ParseState::Argument:
		g_ParseState = ParseState::Arguments;

		if (g_CurrentArgumentInfo.info.type == sp::HookParamType_Unknown)
		{
			globals::sourcemod->LogError(globals::myself, "Missing argument type for argument \"%s\": line: %i col: %i", g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
			return SourceMod::SMCResult_HaltFail;
		}

		// The size wasn't set in the config. See if that's fine and we can guess it from the type.
		if (!g_CurrentArgumentInfo.info.size)
		{
			if (g_CurrentArgumentInfo.info.type == sp::HookParamType_Object)
			{
				globals::sourcemod->LogError(globals::myself, "Object param \"%s\" being set with no size: line: %i col: %i", g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
				return SourceMod::SMCResult_HaltFail;
			}
			else
			{
				g_CurrentArgumentInfo.info.size = sp::GetParamTypeSize(g_CurrentArgumentInfo.info.type);
				if (g_CurrentArgumentInfo.info.size == 0)
				{
					globals::sourcemod->LogError(globals::myself, "param \"%s\" size could not be auto-determined!", g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
					return SourceMod::SMCResult_HaltFail;
				}
			}
		}

		// See if we were changing an existing argument.
		bool changed = false;
		for (auto &arg : g_CurrentSignature->args)
		{
			if (!arg.name.compare(g_CurrentArgumentInfo.name))
			{
				arg.info = g_CurrentArgumentInfo.info;
				changed = true;
				break;
			}
		}
		// This was a new argument. Add it to the end of the list.
		if (!changed)
			g_CurrentSignature->args.push_back(g_CurrentArgumentInfo);

		g_CurrentArgumentInfo.name = "";
		break;
	}

	return SourceMod::SMCResult_Continue;
}

void SignatureGameConfig::ReadSMC_ParseStart()
{
	g_ParseState = ParseState::Root;
	g_IgnoreLevel = 0;
	g_PlatformOnlyState = ParseState::None;
	g_CurrentSignature = nullptr;
	g_CurrentFunctionName = "";
	g_CurrentArgumentInfo.name = "";
}

sp::ReturnType SignatureGameConfig::GetReturnTypeFromString(const char* str)
{
	if (!strcmp(str, "void"))
		return sp::ReturnType_Void;
	else if (!strcmp(str, "int"))
		return sp::ReturnType_Int;
	else if (!strcmp(str, "bool"))
		return sp::ReturnType_Bool;
	else if (!strcmp(str, "float"))
		return sp::ReturnType_Float;
	else if (!strcmp(str, "string"))
		return sp::ReturnType_String;
	else if (!strcmp(str, "stringptr"))
		return sp::ReturnType_StringPtr;
	else if (!strcmp(str, "charptr"))
		return sp::ReturnType_CharPtr;
	else if (!strcmp(str, "vector"))
		return sp::ReturnType_Vector;
	else if (!strcmp(str, "vectorptr"))
		return sp::ReturnType_VectorPtr;
	else if (!strcmp(str, "cbaseentity"))
		return sp::ReturnType_CBaseEntity;
	else if (!strcmp(str, "edict"))
		return sp::ReturnType_Edict;

	return sp::ReturnType_Unknown;
}

sp::HookParamType SignatureGameConfig::GetHookParamTypeFromString(const char* str)
{
	if (!strcmp(str, "int"))
		return sp::HookParamType_Int;
	else if (!strcmp(str, "bool"))
		return sp::HookParamType_Bool;
	else if (!strcmp(str, "float"))
		return sp::HookParamType_Float;
	else if (!strcmp(str, "string"))
		return sp::HookParamType_String;
	else if (!strcmp(str, "stringptr"))
		return sp::HookParamType_StringPtr;
	else if (!strcmp(str, "charptr"))
		return sp::HookParamType_CharPtr;
	else if (!strcmp(str, "vectorptr"))
		return sp::HookParamType_VectorPtr;
	else if (!strcmp(str, "cbaseentity"))
		return sp::HookParamType_CBaseEntity;
	else if (!strcmp(str, "objectptr"))
		return sp::HookParamType_ObjectPtr;
	else if (!strcmp(str, "edict"))
		return sp::HookParamType_Edict;
	else if (!strcmp(str, "object"))
		return sp::HookParamType_Object;
	return sp::HookParamType_Unknown;
}

sp::DHookRegister SignatureGameConfig::GetCustomRegisterFromString(const char* str)
{
	if (!strcmp(str, "al"))
		return sp::DHookRegister_AL;
	else if (!strcmp(str, "cl"))
		return sp::DHookRegister_CL;
	else if (!strcmp(str, "dl"))
		return sp::DHookRegister_DL;
	else if (!strcmp(str, "bl"))
		return sp::DHookRegister_BL;
	else if (!strcmp(str, "ah"))
		return sp::DHookRegister_AH;
	else if (!strcmp(str, "ch"))
		return sp::DHookRegister_CH;
	else if (!strcmp(str, "dh"))
		return sp::DHookRegister_DH;
	else if (!strcmp(str, "bh"))
		return sp::DHookRegister_BH;
	else if (!strcmp(str, "ax"))
		return sp::DHookRegister_EAX;
	else if (!strcmp(str, "cx"))
		return sp::DHookRegister_ECX;
	else if (!strcmp(str, "dx"))
		return sp::DHookRegister_EDX;
	else if (!strcmp(str, "bx"))
		return sp::DHookRegister_EBX;
	else if (!strcmp(str, "sp"))
		return sp::DHookRegister_ESP;
	else if (!strcmp(str, "bp"))
		return sp::DHookRegister_EBP;
	else if (!strcmp(str, "si"))
		return sp::DHookRegister_ESI;
	else if (!strcmp(str, "di"))
		return sp::DHookRegister_EDI;
	else if (!strcmp(str, "eax"))
		return sp::DHookRegister_EAX;
	else if (!strcmp(str, "ecx"))
		return sp::DHookRegister_ECX;
	else if (!strcmp(str, "edx"))
		return sp::DHookRegister_EDX;
	else if (!strcmp(str, "ebx"))
		return sp::DHookRegister_EBX;
	else if (!strcmp(str, "esp"))
		return sp::DHookRegister_ESP;
	else if (!strcmp(str, "ebp"))
		return sp::DHookRegister_EBP;
	else if (!strcmp(str, "esi"))
		return sp::DHookRegister_ESI;
	else if (!strcmp(str, "edi"))
		return sp::DHookRegister_EDI;
	else if (!strcmp(str, "rax"))
		return sp::DHookRegister_RAX;
	else if (!strcmp(str, "rcx"))
		return sp::DHookRegister_RCX;
	else if (!strcmp(str, "rdx"))
		return sp::DHookRegister_RDX;
	else if (!strcmp(str, "rbx"))
		return sp::DHookRegister_RBX;
	else if (!strcmp(str, "rsp"))
		return sp::DHookRegister_RSP;
	else if (!strcmp(str, "rbp"))
		return sp::DHookRegister_RBP;
	else if (!strcmp(str, "rsi"))
		return sp::DHookRegister_RSI;
	else if (!strcmp(str, "rdi"))
		return sp::DHookRegister_RDI;
	else if (!strcmp(str, "r8"))
		return sp::DHookRegister_R8;
	else if (!strcmp(str, "r9"))
		return sp::DHookRegister_R9;
	else if (!strcmp(str, "r10"))
		return sp::DHookRegister_R10;
	else if (!strcmp(str, "r11"))
		return sp::DHookRegister_R11;
	else if (!strcmp(str, "r12"))
		return sp::DHookRegister_R12;
	else if (!strcmp(str, "r13"))
		return sp::DHookRegister_R13;
	else if (!strcmp(str, "r14"))
		return sp::DHookRegister_R14;
	else if (!strcmp(str, "r15"))
		return sp::DHookRegister_R15;
	else if (!strcmp(str, "mm0"))
		return sp::DHookRegister_XMM0;
	else if (!strcmp(str, "mm1"))
		return sp::DHookRegister_XMM1;
	else if (!strcmp(str, "mm2"))
		return sp::DHookRegister_XMM2;
	else if (!strcmp(str, "mm3"))
		return sp::DHookRegister_XMM3;
	else if (!strcmp(str, "mm4"))
		return sp::DHookRegister_XMM4;
	else if (!strcmp(str, "mm5"))
		return sp::DHookRegister_XMM5;
	else if (!strcmp(str, "mm6"))
		return sp::DHookRegister_XMM6;
	else if (!strcmp(str, "mm7"))
		return sp::DHookRegister_XMM7;
	else if (!strcmp(str, "xmm0"))
		return sp::DHookRegister_XMM0;
	else if (!strcmp(str, "xmm1"))
		return sp::DHookRegister_XMM1;
	else if (!strcmp(str, "xmm2"))
		return sp::DHookRegister_XMM2;
	else if (!strcmp(str, "xmm3"))
		return sp::DHookRegister_XMM3;
	else if (!strcmp(str, "xmm4"))
		return sp::DHookRegister_XMM4;
	else if (!strcmp(str, "xmm5"))
		return sp::DHookRegister_XMM5;
	else if (!strcmp(str, "xmm6"))
		return sp::DHookRegister_XMM6;
	else if (!strcmp(str, "xmm7"))
		return sp::DHookRegister_XMM7;
	else if (!strcmp(str, "xmm8"))
		return sp::DHookRegister_XMM8;
	else if (!strcmp(str, "xmm9"))
		return sp::DHookRegister_XMM9;
	else if (!strcmp(str, "xmm10"))
		return sp::DHookRegister_XMM10;
	else if (!strcmp(str, "xmm11"))
		return sp::DHookRegister_XMM11;
	else if (!strcmp(str, "xmm12"))
		return sp::DHookRegister_XMM12;
	else if (!strcmp(str, "xmm13"))
		return sp::DHookRegister_XMM13;
	else if (!strcmp(str, "xmm14"))
		return sp::DHookRegister_XMM14;
	else if (!strcmp(str, "xmm15"))
		return sp::DHookRegister_XMM15;
	else if (!strcmp(str, "st0"))
		return sp::DHookRegister_ST0;
	return sp::DHookRegister_Default;
}

}