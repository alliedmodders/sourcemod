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

#include <signatures.h>

SignatureGameConfig *g_pSignatures;

enum ParseState
{
	PState_None,
	PState_Root,
	PState_Function,
	PState_Arguments,
	PState_Argument
};

ParseState g_ParseState;
unsigned int g_IgnoreLevel;
// The parent section type of a platform specific "windows" or "linux" section.
ParseState g_PlatformOnlyState;

SignatureWrapper *g_CurrentSignature;
std::string g_CurrentFunctionName;
ArgumentInfo g_CurrentArgumentInfo;

SignatureWrapper *SignatureGameConfig::GetFunctionSignature(const char *function)
{
	auto sig = signatures_.find(function);
	if (!sig.found())
		return nullptr;

	return sig->value;
}

/**
 * Game config "Functions" section parsing.
 */
SMCResult SignatureGameConfig::ReadSMC_NewSection(const SMCStates *states, const char *name)
{
	// We're ignoring the parent section. Ignore all child sections as well.
	if (g_IgnoreLevel > 0)
	{
		g_IgnoreLevel++;
		return SMCResult_Continue;
	}

	// Handle platform specific sections first.
#if defined WIN32
	if (!strcmp(name, "windows"))
#elif defined _LINUX
	if (!strcmp(name, "linux"))
#elif defined _OSX
	if (!strcmp(name, "mac"))
#endif
	{
		// We're already in a section for a different OS that we're ignoring. Can't have a section for our OS in here.
		if (g_IgnoreLevel > 0)
		{
			smutils->LogError(myself, "Unreachable platform specific section in \"%s\" Function: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SMCResult_HaltFail;
		}

		// We don't support nested (useless) sections of the same OS like "windows" { "windows" { "foo" "bar" } }
		if (g_PlatformOnlyState != PState_None)
		{
			smutils->LogError(myself, "Duplicate platform specific section for \"%s\". Already parsing only for that OS: line: %i col: %i", name, states->line, states->col);
			return SMCResult_HaltFail;
		}

		// This is a specific block for us.
		g_PlatformOnlyState = g_ParseState;
		return SMCResult_Continue;
	}
#if defined WIN32
	else if (!strcmp(name, "linux") || !strcmp(name, "mac"))
#elif defined _LINUX
	else if (!strcmp(name, "windows") || !strcmp(name, "mac"))
#elif defined _OSX
	else if (!strcmp(name, "windows") || !strcmp(name, "linux"))
#endif
	{
		if (g_PlatformOnlyState != PState_None)
		{
			smutils->LogError(myself, "Unreachable platform specific section in \"%s\" Function: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SMCResult_HaltFail;
		}

		// A specific block for a different platform.
		g_IgnoreLevel++;
		return SMCResult_Continue;
	}

	switch (g_ParseState)
	{
	case PState_Root:
	{
		auto sig = signatures_.find(name);
		if (sig.found())
			g_CurrentSignature = sig->value;
		else
			g_CurrentSignature = new SignatureWrapper();
		g_CurrentFunctionName = name;
		g_ParseState = PState_Function;
		break;
	}

	case PState_Function:
	{
		if (!strcmp(name, "arguments"))
		{
			g_ParseState = PState_Arguments;
		}
		else
		{
			smutils->LogError(myself, "Unknown subsection \"%s\" (expected \"arguments\"): line: %i col: %i", name, states->line, states->col);
			return SMCResult_HaltFail;
		}
		break;
	}
	case PState_Arguments:
	{
		g_ParseState = PState_Argument;
		g_CurrentArgumentInfo.name = name;

		// Reset the parameter info.
		ParamInfo info;
		memset(&info, 0, sizeof(info));
		info.flags = PASSFLAG_BYVAL;
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
	default:
		smutils->LogError(myself, "Unknown subsection \"%s\": line: %i col: %i", name, states->line, states->col);
		return SMCResult_HaltFail;
	}

	return SMCResult_Continue;
}

SMCResult SignatureGameConfig::ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
{
	// We don't care for anything in this section or subsections.
	if (g_IgnoreLevel > 0)
		return SMCResult_Continue;

	switch (g_ParseState)
	{
	case PState_Function:

		if (!strcmp(key, "signature"))
		{
			if (g_CurrentSignature->address.length() > 0 || g_CurrentSignature->offset.length() > 0)
			{
				smutils->LogError(myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
				return SMCResult_HaltFail;
			}
			g_CurrentSignature->signature = value;
		}
		else if (!strcmp(key, "address"))
		{
			if (g_CurrentSignature->signature.length() > 0 || g_CurrentSignature->offset.length() > 0)
			{
				smutils->LogError(myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
				return SMCResult_HaltFail;
			}
			g_CurrentSignature->address = value;
		}
		else if (!strcmp(key, "offset"))
		{
			if (g_CurrentSignature->address.length() > 0 || g_CurrentSignature->signature.length() > 0)
			{
				smutils->LogError(myself, "Cannot have \"signature\", \"address\" or \"offset\" keys at the same time in one function: line: %i col: %i", states->line, states->col);
				return SMCResult_HaltFail;
			}
			g_CurrentSignature->offset = value;
		}
		else if (!strcmp(key, "callconv"))
		{
			CallingConvention callConv;

			if (!strcmp(value, "cdecl"))
				callConv = CallConv_CDECL;
			else if (!strcmp(value, "thiscall"))
				callConv = CallConv_THISCALL;
			else if (!strcmp(value, "stdcall"))
				callConv = CallConv_STDCALL;
			else if (!strcmp(value, "fastcall"))
				callConv = CallConv_FASTCALL;
			else
			{
				smutils->LogError(myself, "Invalid calling convention \"%s\": line: %i col: %i", value, states->line, states->col);
				return SMCResult_HaltFail;
			}

			g_CurrentSignature->callConv = callConv;
		}
		else if (!strcmp(key, "hooktype"))
		{
			HookType hookType;

			if (!strcmp(value, "entity"))
				hookType = HookType_Entity;
			else if (!strcmp(value, "gamerules"))
				hookType = HookType_GameRules;
			else if (!strcmp(value, "raw"))
				hookType = HookType_Raw;
			else
			{
				smutils->LogError(myself, "Invalid hook type \"%s\": line: %i col: %i", value, states->line, states->col);
				return SMCResult_HaltFail;
			}

			g_CurrentSignature->hookType = hookType;
		}
		else if (!strcmp(key, "return"))
		{
			g_CurrentSignature->retType = GetReturnTypeFromString(value);

			if (g_CurrentSignature->retType == ReturnType_Unknown)
			{
				smutils->LogError(myself, "Invalid return type \"%s\": line: %i col: %i", value, states->line, states->col);
				return SMCResult_HaltFail;
			}
		}
		else if (!strcmp(key, "this"))
		{
			if (!strcmp(value, "ignore"))
				g_CurrentSignature->thisType = ThisPointer_Ignore;
			else if (!strcmp(value, "entity"))
				g_CurrentSignature->thisType = ThisPointer_CBaseEntity;
			else if (!strcmp(value, "address"))
				g_CurrentSignature->thisType = ThisPointer_Address;
			else
			{
				smutils->LogError(myself, "Invalid this type \"%s\": line: %i col: %i", value, states->line, states->col);
				return SMCResult_HaltFail;
			}
		}
		else
		{
			smutils->LogError(myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
			return SMCResult_HaltFail;
		}
		break;

	case PState_Argument:

		if (!strcmp(key, "type"))
		{
			g_CurrentArgumentInfo.info.type = GetHookParamTypeFromString(value);
			if (g_CurrentArgumentInfo.info.type == HookParamType_Unknown)
			{
				smutils->LogError(myself, "Invalid argument type \"%s\" for argument \"%s\": line: %i col: %i", value, g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
				return SMCResult_HaltFail;
			}
		}
		else if (!strcmp(key, "size"))
		{
			g_CurrentArgumentInfo.info.size = static_cast<int>(strtol(value, NULL, 0));

			if (g_CurrentArgumentInfo.info.size < 1)
			{
				smutils->LogError(myself, "Invalid argument size \"%s\" for argument \"%s\": line: %i col: %i", value, g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
				return SMCResult_HaltFail;
			}
		}
		else if (!strcmp(key, "flags"))
		{
			size_t flags = 0;
			if (strstr(value, "byval"))
				flags |= PASSFLAG_BYVAL;
			if (strstr(value, "byref"))
				flags |= PASSFLAG_BYREF;
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

			g_CurrentArgumentInfo.info.flags = flags;
		}
		else if (!strcmp(key, "register"))
		{
			g_CurrentArgumentInfo.info.custom_register = GetCustomRegisterFromString(value);

			if (g_CurrentArgumentInfo.info.custom_register == Register_t::None)
			{
				smutils->LogError(myself, "Invalid register \"%s\": line: %i col: %i", value, states->line, states->col);
				return SMCResult_HaltFail;
			}
		}
		else
		{
			smutils->LogError(myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
			return SMCResult_HaltFail;
		}
		break;

	default:
		smutils->LogError(myself, "Unknown key in Functions section \"%s\": line: %i col: %i", key, states->line, states->col);
		return SMCResult_HaltFail;
	}
	return SMCResult_Continue;
}

SMCResult SignatureGameConfig::ReadSMC_LeavingSection(const SMCStates *states)
{
	// We were ignoring this section.
	if (g_IgnoreLevel > 0)
	{
		g_IgnoreLevel--;
		return SMCResult_Continue;
	}

	// We were in a section only for our OS.
	if (g_PlatformOnlyState == g_ParseState)
	{
		g_PlatformOnlyState = PState_None;
		return SMCResult_Continue;
	}

	switch (g_ParseState)
	{
	case PState_Function:
		g_ParseState = PState_Root;

		if (!g_CurrentSignature->address.length() && !g_CurrentSignature->signature.length() && !g_CurrentSignature->offset.length())
		{
			smutils->LogError(myself, "Function \"%s\" doesn't have a \"signature\", \"offset\" nor \"address\" set: line: %i col: %i", g_CurrentFunctionName.c_str(), states->line, states->col);
			return SMCResult_HaltFail;
		}

		if (!g_CurrentSignature->offset.length())
		{
			// DynamicDetours doesn't expose the passflags concept like SourceHook.
			// See if we're trying to set some invalid flags on detour arguments.
			for (auto &arg : g_CurrentSignature->args)
			{
				if ((arg.info.flags & ~PASSFLAG_BYVAL) > 0)
				{
					smutils->LogError(myself, "Function \"%s\" uses unsupported pass flags in argument \"%s\". Flags are only supported for virtual hooks: line: %i col: %i", g_CurrentFunctionName.c_str(), arg.name.c_str(), states->line, states->col);
					return SMCResult_HaltFail;
				}
			}
		}

		// Save this function signature in our cache.
		signatures_.insert(g_CurrentFunctionName.c_str(), g_CurrentSignature);
		g_CurrentFunctionName = "";
		g_CurrentSignature = nullptr;
		break;
	case PState_Arguments:
		g_ParseState = PState_Function;
		break;
	case PState_Argument:
		g_ParseState = PState_Arguments;

		if (g_CurrentArgumentInfo.info.type == HookParamType_Unknown)
		{
			smutils->LogError(myself, "Missing argument type for argument \"%s\": line: %i col: %i", g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
			return SMCResult_HaltFail;
		}

		// The size wasn't set in the config. See if that's fine and we can guess it from the type.
		if (!g_CurrentArgumentInfo.info.size)
		{
			if (g_CurrentArgumentInfo.info.type == HookParamType_Object)
			{
				smutils->LogError(myself, "Object param \"%s\" being set with no size: line: %i col: %i", g_CurrentArgumentInfo.name.c_str(), states->line, states->col);
				return SMCResult_HaltFail;
			}
			else
			{
				g_CurrentArgumentInfo.info.size = GetParamTypeSize(g_CurrentArgumentInfo.info.type);
			}
		}

		if (g_CurrentArgumentInfo.info.pass_type == SourceHook::PassInfo::PassType::PassType_Unknown)
			g_CurrentArgumentInfo.info.pass_type = GetParamTypePassType(g_CurrentArgumentInfo.info.type);

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

	return SMCResult_Continue;
}

void SignatureGameConfig::ReadSMC_ParseStart()
{
	g_ParseState = PState_Root;
	g_IgnoreLevel = 0;
	g_PlatformOnlyState = PState_None;
	g_CurrentSignature = nullptr;
	g_CurrentFunctionName = "";
	g_CurrentArgumentInfo.name = "";
}

ReturnType SignatureGameConfig::GetReturnTypeFromString(const char *str)
{
	if (!strcmp(str, "void"))
		return ReturnType_Void;
	else if (!strcmp(str, "int"))
		return ReturnType_Int;
	else if (!strcmp(str, "bool"))
		return ReturnType_Bool;
	else if (!strcmp(str, "float"))
		return ReturnType_Float;
	else if (!strcmp(str, "string"))
		return ReturnType_String;
	else if (!strcmp(str, "stringptr"))
		return ReturnType_StringPtr;
	else if (!strcmp(str, "charptr"))
		return ReturnType_CharPtr;
	else if (!strcmp(str, "vector"))
		return ReturnType_Vector;
	else if (!strcmp(str, "vectorptr"))
		return ReturnType_VectorPtr;
	else if (!strcmp(str, "cbaseentity"))
		return ReturnType_CBaseEntity;
	else if (!strcmp(str, "edict"))
		return ReturnType_Edict;

	return ReturnType_Unknown;
}

HookParamType SignatureGameConfig::GetHookParamTypeFromString(const char *str)
{
	if (!strcmp(str, "int"))
		return HookParamType_Int;
	else if (!strcmp(str, "bool"))
		return HookParamType_Bool;
	else if (!strcmp(str, "float"))
		return HookParamType_Float;
	else if (!strcmp(str, "string"))
		return HookParamType_String;
	else if (!strcmp(str, "stringptr"))
		return HookParamType_StringPtr;
	else if (!strcmp(str, "charptr"))
		return HookParamType_CharPtr;
	else if (!strcmp(str, "vectorptr"))
		return HookParamType_VectorPtr;
	else if (!strcmp(str, "cbaseentity"))
		return HookParamType_CBaseEntity;
	else if (!strcmp(str, "objectptr"))
		return HookParamType_ObjectPtr;
	else if (!strcmp(str, "edict"))
		return HookParamType_Edict;
	else if (!strcmp(str, "object"))
		return HookParamType_Object;

	return HookParamType_Unknown;
}

Register_t SignatureGameConfig::GetCustomRegisterFromString(const char *str)
{
	if (!strcmp(str, "al"))
		return AL;
	else if (!strcmp(str, "cl"))
		return CL;
	else if (!strcmp(str, "dl"))
		return DL;
	else if (!strcmp(str, "bl"))
		return BL;
	else if (!strcmp(str, "ah"))
		return AH;
	else if (!strcmp(str, "ch"))
		return CH;
	else if (!strcmp(str, "dh"))
		return DH;
	else if (!strcmp(str, "bh"))
		return BH;

	else if (!strcmp(str, "ax"))
		return AX;
	else if (!strcmp(str, "cx"))
		return CX;
	else if (!strcmp(str, "dx"))
		return DX;
	else if (!strcmp(str, "bx"))
		return BX;
	else if (!strcmp(str, "sp"))
		return SP;
	else if (!strcmp(str, "bp"))
		return BP;
	else if (!strcmp(str, "si"))
		return SI;
	else if (!strcmp(str, "di"))
		return DI;

	else if (!strcmp(str, "eax"))
		return EAX;
	else if (!strcmp(str, "ecx"))
		return ECX;
	else if (!strcmp(str, "edx"))
		return EDX;
	else if (!strcmp(str, "ebx"))
		return EBX;
	else if (!strcmp(str, "esp"))
		return ESP;
	else if (!strcmp(str, "ebp"))
		return EBP;
	else if (!strcmp(str, "esi"))
		return ESI;
	else if (!strcmp(str, "edi"))
		return EDI;

	else if (!strcmp(str, "mm0"))
		return MM0;
	else if (!strcmp(str, "mm1"))
		return MM1;
	else if (!strcmp(str, "mm2"))
		return MM2;
	else if (!strcmp(str, "mm3"))
		return MM3;
	else if (!strcmp(str, "mm4"))
		return MM4;
	else if (!strcmp(str, "mm5"))
		return MM5;
	else if (!strcmp(str, "mm6"))
		return MM6;
	else if (!strcmp(str, "mm7"))
		return MM7;

	else if (!strcmp(str, "xmm0"))
		return XMM0;
	else if (!strcmp(str, "xmm1"))
		return XMM1;
	else if (!strcmp(str, "xmm2"))
		return XMM2;
	else if (!strcmp(str, "xmm3"))
		return XMM3;
	else if (!strcmp(str, "xmm4"))
		return XMM4;
	else if (!strcmp(str, "xmm5"))
		return XMM5;
	else if (!strcmp(str, "xmm6"))
		return XMM6;
	else if (!strcmp(str, "xmm7"))
		return XMM7;

	else if (!strcmp(str, "cs"))
		return CS;
	else if (!strcmp(str, "ss"))
		return SS;
	else if (!strcmp(str, "ds"))
		return DS;
	else if (!strcmp(str, "es"))
		return ES;
	else if (!strcmp(str, "fs"))
		return FS;
	else if (!strcmp(str, "gs"))
		return GS;

	else if (!strcmp(str, "st0"))
		return ST0;

	return Register_t::None;
}