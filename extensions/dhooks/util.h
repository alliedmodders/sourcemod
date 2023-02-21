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

#ifndef _INCLUDE_UTIL_FUNCTIONS_H_
#define _INCLUDE_UTIL_FUNCTIONS_H_

#include "vhook.h"
#include "convention.h"

enum PluginRegister
{
	// Don't change the register and use the default for the calling convention.
	DHookRegister_Default,

	// 8-bit general purpose registers
	DHookRegister_AL,
	DHookRegister_CL,
	DHookRegister_DL,
	DHookRegister_BL,
	DHookRegister_AH,
	DHookRegister_CH,
	DHookRegister_DH,
	DHookRegister_BH,

	// 32-bit general purpose registers
	DHookRegister_EAX,
	DHookRegister_ECX,
	DHookRegister_EDX,
	DHookRegister_EBX,
	DHookRegister_ESP,
	DHookRegister_EBP,
	DHookRegister_ESI,
	DHookRegister_EDI,

	// 128-bit XMM registers
	DHookRegister_XMM0,
	DHookRegister_XMM1,
	DHookRegister_XMM2,
	DHookRegister_XMM3,
	DHookRegister_XMM4,
	DHookRegister_XMM5,
	DHookRegister_XMM6,
	DHookRegister_XMM7,

	// 80-bit FPU registers
	DHookRegister_ST0
};

size_t GetParamOffset(HookParamsStruct *params, unsigned int index);
void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset);
size_t GetParamTypeSize(HookParamType type);
size_t GetParamsSize(DHooksCallback *dg);

DataType_t DynamicHooks_ConvertParamTypeFrom(HookParamType type);
DataType_t DynamicHooks_ConvertReturnTypeFrom(ReturnType type);
Register_t DynamicHooks_ConvertRegisterFrom(PluginRegister reg);
#endif
