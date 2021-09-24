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

#include "extension.h"
#include "CallWrapper.h"
#include "CallMaker.h"

CallWrapper::CallWrapper(const SourceHook::ProtoInfo *protoInfo) : m_FnFlags(0)
{
	m_AddrCodeBase = NULL;
	m_AddrCallee = NULL;

	unsigned int argnum = protoInfo->numOfParams;

	m_Info = *protoInfo;
	m_Info.paramsPassInfo = new SourceHook::PassInfo[argnum + 1];
	memcpy((void *)m_Info.paramsPassInfo, protoInfo->paramsPassInfo, sizeof(SourceHook::PassInfo) * (argnum+1));

	if (argnum)
	{
		m_Params = new PassEncode[argnum];
		for (size_t i=0; i<argnum; i++)
		{
			GetSMPassInfo(&(m_Params[i].info), &(m_Info.paramsPassInfo[i+1]));
		}
	} else {
		m_Params = NULL;
	}

	if (m_Info.retPassInfo.size != 0)
	{
		m_RetParam = new PassInfo;
		GetSMPassInfo(m_RetParam, &(m_Info.retPassInfo));
	} else {
		m_RetParam = NULL;
	}

	/* Calculate virtual stack offsets for each parameter */
	size_t offs = 0;

	if (m_Info.convention == SourceHook::ProtoInfo::CallConv_ThisCall)
	{
		offs += sizeof(void *);
	}
	for (size_t i=0; i<argnum; i++)
	{
		m_Params[i].offset = offs;
		offs += m_Params[i].info.size;
	}
}

CallWrapper::CallWrapper(const SourceHook::ProtoInfo *protoInfo, const PassInfo *retInfo,
                         const PassInfo paramInfo[], unsigned int fnFlags) : CallWrapper(protoInfo)
{
	if (retInfo)
	{
		m_RetParam->fields = retInfo->fields;
		m_RetParam->numFields = retInfo->numFields;
	}
	else
	{
		m_RetParam = nullptr;
	}
	
	unsigned int argnum = protoInfo->numOfParams;
	for (unsigned int i = 0; i < argnum; i++)
	{
		m_Params[i].info.fields = paramInfo[i].fields;
		m_Params[i].info.numFields = paramInfo[i].numFields;
	}
	
	m_FnFlags = fnFlags;
}

CallWrapper::~CallWrapper()
{
	delete [] m_Params;
	delete m_RetParam;
	delete [] m_Info.paramsPassInfo;
}

void CallWrapper::Destroy()
{
	if (m_AddrCodeBase != NULL)
	{
		g_SPEngine->FreePageMemory(m_AddrCodeBase);
		m_AddrCodeBase = NULL;
	}

	delete this;
}

CallConvention CallWrapper::GetCallConvention()
{
	/* Need to convert to a SourceMod convention for bcompat */
	return GetSMCallConvention((SourceHook::ProtoInfo::CallConvention)m_Info.convention);
}

const PassEncode *CallWrapper::GetParamInfo(unsigned int num)
{
	if (num + 1 > GetParamCount())
	{
		return NULL;
	}

	return &m_Params[num];
}

const PassInfo *CallWrapper::GetReturnInfo()
{
	return m_RetParam;
}

unsigned int CallWrapper::GetParamCount()
{
	return m_Info.numOfParams;
}

void CallWrapper::Execute(void *vParamStack, void *retBuffer)
{
	typedef void (*CALL_EXECUTE)(void *, void *);
	CALL_EXECUTE fn = (CALL_EXECUTE)GetCodeBaseAddr();
	fn(vParamStack, retBuffer);
}

void CallWrapper::SetMemFuncInfo(const SourceHook::MemFuncInfo *funcInfo)
{
	m_FuncInfo = *funcInfo;
}

SourceHook::MemFuncInfo *CallWrapper::GetMemFuncInfo()
{
	return &m_FuncInfo;
}

void CallWrapper::SetCalleeAddr(void *addr)
{
	m_AddrCallee = addr;
}

void CallWrapper::SetCodeBaseAddr(void *addr)
{
	m_AddrCodeBase = addr;
}

void *CallWrapper::GetCalleeAddr()
{
	return m_AddrCallee;
}

void *CallWrapper::GetCodeBaseAddr()
{
	return m_AddrCodeBase;
}

const SourceHook::PassInfo *CallWrapper::GetSHReturnInfo()
{
	return &(m_Info.retPassInfo);
}

SourceHook::ProtoInfo::CallConvention CallWrapper::GetSHCallConvention()
{
	return (SourceHook::ProtoInfo::CallConvention)m_Info.convention;
}

const SourceHook::PassInfo * CallWrapper::GetSHParamInfo(unsigned int num)
{
	if (num + 1 > GetParamCount())
	{
		return NULL;
	}

	return &(m_Info.paramsPassInfo[num+1]);
}

unsigned int CallWrapper::GetParamOffset(unsigned int num)
{
	assert(num < GetParamCount() && num >= 0);

	return m_Params[num].offset;
}

unsigned int CallWrapper::GetFunctionFlags()
{
	return m_FnFlags;
}
