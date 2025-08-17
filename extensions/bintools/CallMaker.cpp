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
#include <vector>
#include "CallMaker.h"
#include "jit_compile.h"
#include "extension.h"

class CProtoInfoBuilder
{
	ProtoInfo m_PI;
	std::vector<PassInfo> m_Params;
public:
	CProtoInfoBuilder(int cc)
	{
		memset(reinterpret_cast<void*>(&m_PI), 0, sizeof(ProtoInfo));
		m_PI.convention = cc;

		// dummy 0 params
		PassInfo dummy;
		memset(reinterpret_cast<void*>(&dummy), 0, sizeof(PassInfo));

		dummy.size = 1;		// Version1

		m_Params.push_back(dummy);
	}

	void SetReturnType(size_t size, PassType type, int flags,
		void *pNormalCtor, void *pCopyCtor, void *pDtor, void *pAssignOperator)
	{
		if (pNormalCtor)
			flags |= PASSFLAG_OCTOR;

		//if (pCopyCtor)
		//	flags |= PassInfo::PassFlag_CCtor;

		if (pDtor)
			flags |= PASSFLAG_ODTOR;

		if (pAssignOperator)
			flags |= PASSFLAG_OASSIGNOP;

		m_PI.retPassInfo.size = size;
		m_PI.retPassInfo.type = type;
		m_PI.retPassInfo.flags = flags;
	}

	void AddParam(size_t size, PassType type, int flags,
		void *pNormalCtor, void *pCopyCtor, void *pDtor, void *pAssignOperator)
	{
		PassInfo pi;

		if (pNormalCtor)
			flags |= PASSFLAG_OCTOR;

		//if (pCopyCtor)
		//	flags |= PassInfo::PassFlag_CCtor;

		if (pDtor)
			flags |= PASSFLAG_ODTOR;

		if (pAssignOperator)
			flags |= PASSFLAG_OASSIGNOP;

		
		pi.size = size;
		pi.type = type;
		pi.flags = flags;

		m_Params.push_back(pi);
		++m_PI.numOfParams;
	}

	operator ProtoInfo*()
	{
		m_PI.paramsPassInfo = &(m_Params[0]);
		return &m_PI;
	}
};

ICallWrapper *CallMaker::CreateCall(void *address,
						 CallConvention cv,
						 const PassInfo *retInfo,
						 const PassInfo paramInfo[],
						 unsigned int numParams,
						 unsigned int fnFlags)
{
	CProtoInfoBuilder protoInfo(cv);

	for (unsigned int i=0; i<numParams; i++)
	{
		protoInfo.AddParam(paramInfo[i].size, paramInfo[i].type, paramInfo[i].flags,
			NULL, NULL, NULL, NULL);
	}

	if (retInfo)
	{
		protoInfo.SetReturnType(retInfo->size, retInfo->type, retInfo->flags,
			NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(0, PassType_Basic, 0,
			NULL, NULL, NULL, NULL);
	}

#if defined KE_ARCH_X64
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

	CProtoInfoBuilder protoInfo(CallConv_ThisCall);

	for (unsigned int i=0; i<numParams; i++)
	{
		protoInfo.AddParam(paramInfo[i].size, paramInfo[i].type, paramInfo[i].flags,
			NULL, NULL, NULL, NULL);
	}

	if (retInfo)
	{
		protoInfo.SetReturnType(retInfo->size, retInfo->type, retInfo->flags,
			NULL, NULL, NULL, NULL);
	}
	else
	{
		protoInfo.SetReturnType(0, PassType::PassType_Basic, 0,
			NULL, NULL, NULL, NULL);
	}
	
#if defined KE_ARCH_X64
	return g_CallMaker2.CreateVirtualCall(&(*protoInfo), vtblIdx, retInfo, paramInfo, fnFlags);
#else
	return g_CallMaker2.CreateVirtualCall(&(*protoInfo), vtblIdx);
#endif
}

ICallWrapper *CallMaker2::CreateCall(void *address, const ProtoInfo *protoInfo)
{
#ifdef KE_ARCH_X86
	CallWrapper *pWrapper = new CallWrapper(protoInfo);
	pWrapper->SetCalleeAddr(address);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_Direct);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateVirtualCall(const ProtoInfo* protoInfo, uint32_t vtable_index)
{
#ifdef KE_ARCH_X86
	CallWrapper *pWrapper = new CallWrapper(protoInfo);
	pWrapper->SetVtableIndex(vtable_index);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_VTable);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateCall(void *address, const ProtoInfo *protoInfo,
                                     const PassInfo *retInfo, const PassInfo paramInfo[],
                                     unsigned int fnFlags)
{
#ifdef KE_ARCH_X64
	CallWrapper *pWrapper = new CallWrapper(protoInfo, retInfo, paramInfo, fnFlags);
	pWrapper->SetCalleeAddr(address);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_Direct);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}

ICallWrapper *CallMaker2::CreateVirtualCall(const ProtoInfo *protoInfo,
											uint32_t vtable_index,
											const PassInfo *retInfo,
                                            const PassInfo paramInfo[],
                                            unsigned int fnFlags)
{
#ifdef KE_ARCH_X64
	CallWrapper *pWrapper = new CallWrapper(protoInfo, retInfo, paramInfo, fnFlags);
	pWrapper->SetVtableIndex(vtable_index);

	void *addr = JIT_CallCompile(pWrapper, FuncAddr_VTable);
	pWrapper->SetCodeBaseAddr(addr);

	return pWrapper;
#else
	return nullptr;
#endif
}