/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#include <stdio.h>
#include "CallMaker.h"
#include "jit_compile.h"
#include "extension.h"

/* SourceMod <==> SourceHook type conversion functions */

SourceHook::ProtoInfo::CallConvention GetSHCallConvention(SourceMod::CallConvention cv)
{
	if (cv < SourceMod::CallConv_ThisCall || cv > SourceMod::CallConv_Cdecl)
	{
		return SourceHook::ProtoInfo::CallConv_Unknown;
	}

	return (SourceHook::ProtoInfo::CallConvention)(cv + 1);
}

SourceMod::CallConvention GetSMCallConvention(SourceHook::ProtoInfo::CallConvention cv)
{
	assert(cv >= SourceHook::ProtoInfo::CallConv_ThisCall || cv <= SourceHook::ProtoInfo::CallConv_Cdecl);

	return (SourceMod::CallConvention)(cv - 1);
}

SourceHook::PassInfo::PassType GetSHPassType(SourceMod::PassType type)
{
	if (type < SourceMod::PassType_Basic || type > SourceMod::PassType_Object)
	{
		return SourceHook::PassInfo::PassType_Unknown;
	}

	return (SourceHook::PassInfo::PassType)(type + 1);
}

SourceMod::PassType GetSMPassType(int type)
{
	/* SourceMod doesn't provide an Unknown type so we it must be an error */
	assert(type >= SourceHook::PassInfo::PassType_Basic && type <= SourceHook::PassInfo::PassType_Object);

	return (SourceMod::PassType)(type - 1);
}

void GetSMPassInfo(SourceMod::PassInfo *out, const SourceHook::PassInfo *in)
{
	out->size = in->size;
	out->flags = in->flags;
	out->type = GetSMPassType(in->type);
}

ICallWrapper *CallMaker::CreateCall(void *address,
						 CallConvention cv,
						 const PassInfo *retInfo,
						 const PassInfo paramInfo[],
						 unsigned int numParams,
						 unsigned int fnFlags)
{
	SourceHook::CProtoInfoBuilder protoInfo(GetSHCallConvention(cv));

	for (unsigned int i=0; i<numParams; i++)
	{
		protoInfo.AddParam(paramInfo[i].size, GetSHPassType(paramInfo[i].type), paramInfo[i].flags,
			NULL, NULL, NULL, NULL);
	}

	if (retInfo)
	{
		protoInfo.SetReturnType(retInfo->size, GetSHPassType(retInfo->type), retInfo->flags,
			NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(0, SourceHook::PassInfo::PassType_Unknown, 0,
			NULL, NULL, NULL, NULL);
	}

#if defined PLATFORM_X64
	return g_CallMaker2.CreateCall(address, &(*protoInfo), retInfo, paramInfo, fnFlags);
#else
	return g_CallMaker2.CreateCall(address, &(*protoInfo));
#endif
}

ICallWrapper *CallMaker::CreateVCall(unsigned int vtblIdx, 
									 unsigned int vtblOffs, 
									 unsigned int thisOffs, 
									 const PassInfo *retInfo, 
									 const PassInfo paramInfo[], 
									 unsigned int numParams,
									 unsigned int fnFlags)
{
	SourceHook::MemFuncInfo info;
	info.isVirtual = true;
	info.vtblindex = vtblIdx;
	info.vtbloffs = vtblOffs;
	info.thisptroffs = thisOffs;

	SourceHook::CProtoInfoBuilder protoInfo(SourceHook::ProtoInfo::CallConv_ThisCall);

	for (unsigned int i=0; i<numParams; i++)
	{
		protoInfo.AddParam(paramInfo[i].size, GetSHPassType(paramInfo[i].type), paramInfo[i].flags,
			NULL, NULL, NULL, NULL);
	}

	if (retInfo)
	{
		protoInfo.SetReturnType(retInfo->size, GetSHPassType(retInfo->type), retInfo->flags,
			NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(0, SourceHook::PassInfo::PassType_Unknown, 0,
			NULL, NULL, NULL, NULL);
	}
	
#if defined PLATFORM_X64
	return g_CallMaker2.CreateVirtualCall(&(*protoInfo), &info, retInfo, paramInfo, fnFlags);
#else
	return g_CallMaker2.CreateVirtualCall(&(*protoInfo), &info);
#endif
}

ICallWrapper *CallMaker2::CreateCall(void *address, const SourceHook::ProtoInfo *protoInfo)
{
#ifdef PLATFORM_X86
	CallWrapper *pWrapper = new CallWrapper(protoInfo);
	pWrapper->SetCalleeAddr(address);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_Direct);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateVirtualCall(const SourceHook::ProtoInfo *protoInfo, 
											const SourceHook::MemFuncInfo *info)
{
#ifdef PLATFORM_X86
	CallWrapper *pWrapper = new CallWrapper(protoInfo);
	pWrapper->SetMemFuncInfo(info);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_VTable);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateCall(void *address, const SourceHook::ProtoInfo *protoInfo,
                                     const PassInfo *retInfo, const PassInfo paramInfo[],
                                     unsigned int fnFlags)
{
#ifdef PLATFORM_X64
	CallWrapper *pWrapper = new CallWrapper(protoInfo, retInfo, paramInfo, fnFlags);
	pWrapper->SetCalleeAddr(address);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_Direct);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateVirtualCall(const SourceHook::ProtoInfo *protoInfo, 
											const SourceHook::MemFuncInfo *info,
											const PassInfo *retInfo,
                                            const PassInfo paramInfo[],
                                            unsigned int fnFlags)
{
#ifdef PLATFORM_X64
	CallWrapper *pWrapper = new CallWrapper(protoInfo, retInfo, paramInfo, fnFlags);
	pWrapper->SetMemFuncInfo(info);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_VTable);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

#if 0
IHookWrapper *CallMaker2::CreateVirtualHook(SourceHook::ISourceHook *pSH, 
											const SourceHook::ProtoInfo *protoInfo,
											const SourceHook::MemFuncInfo *info, 
											VIRTUAL_HOOK_PROTO f)
{
	HookWrapper *pWrapper = new HookWrapper(pSH, protoInfo, const_cast<SourceHook::MemFuncInfo *>(info), (void *)f);

	void *addr = JIT_HookCompile(pWrapper);
	pWrapper->SetHookWrpAddr(addr);

	ICallWrapper *callWrap = CreateVirtualCall(protoInfo, info);
	pWrapper->SetCallWrapperAddr(callWrap);
	
	return pWrapper;
}
#endif

