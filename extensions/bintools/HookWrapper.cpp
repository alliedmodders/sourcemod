/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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
 * Version: $Id: CallMaker.h 1964 2008-03-27 04:54:56Z damagedsoul $
 */

#if defined HOOKING_ENABLED

#include "HookWrapper.h"
#include "jit_compile.h"

/******************************
 * HookWrapper Implementation *
 ******************************/

HookWrapper::HookWrapper(SourceHook::ISourceHook *pSH, 
						 const SourceHook::ProtoInfo *proto, 
						 SourceHook::MemFuncInfo *memfunc, 
						 void *addr) : m_ParamOffs(NULL), m_ParamSize(0), m_RetSize(0)
{
	unsigned int argnum = proto->numOfParams;

	m_pSH = pSH;
	m_ProtoInfo = *proto;
	m_ProtoInfo.paramsPassInfo = new SourceHook::PassInfo[argnum + 1];
	memcpy((void *)m_ProtoInfo.paramsPassInfo, proto->paramsPassInfo, sizeof(SourceHook::PassInfo) * (argnum+1));
	m_MemFuncInfo = *memfunc;
	m_FuncAddr = addr;
	m_HookWrapper = NULL;

	/* If the function has parameters calculate the parameter buffer size and offsets */
	if (argnum > 0)
	{
		const SourceHook::PassInfo *info = m_ProtoInfo.paramsPassInfo;
		m_ParamOffs = new unsigned int[argnum];
		m_ParamOffs[0] = 0;

		for (unsigned int i=1; i<=argnum; i++)
		{
			/* The stack should be at least aligned to a 4 byte boundary 
			* and byref params have the size of pointers 
			*/
			if (((info[i].type == SourceHook::PassInfo::PassType_Basic) &&
				(info[i].flags & SourceHook::PassInfo::PassFlag_ByVal) && 
				(info[i].size < sizeof(void *))) ||
				(info[i].flags & SourceHook::PassInfo::PassFlag_ByRef))
			{
				m_ParamSize += sizeof(void *);
			}
			else
			{
				m_ParamSize += info[i].size;
			}

			//:TODO: fix this!
			if (i < argnum)
			{
				m_ParamOffs[i] = m_ParamSize;
			}
		}
	}

	/* Finally calculate the retval size if any */
	size_t retsize = m_ProtoInfo.retPassInfo.size;
	if (retsize != 0)
	{
		if (m_ProtoInfo.retPassInfo.flags & SourceHook::PassInfo::PassFlag_ByRef)
		{
			m_RetSize = sizeof(void *);
		}
		else
		{
			m_RetSize = (retsize < sizeof(void *)) ? sizeof(void *) : retsize;
		}
	}
}

ISMDelegate *HookWrapper::CreateDelegate(void *data)
{
	SMDelegate *pDeleg = new SMDelegate(data);
	pDeleg->PatchVtable(m_HookWrapper);

	return pDeleg;
}

unsigned int HookWrapper::GetParamCount()
{
	return m_ProtoInfo.numOfParams;
}

unsigned int HookWrapper::GetParamOffset(unsigned int argnum, unsigned int *size)
{
	assert(m_ParamOffs && argnum < (unsigned int)m_ProtoInfo.numOfParams);

	if (size)
	{
		*size = m_ProtoInfo.paramsPassInfo[argnum+1].size;
	}

	return m_ParamOffs[argnum];
}

void HookWrapper::PerformRecall(void *params, void *retval)
{
	/* Notify SourceHook of the upcoming recall */
	m_pSH->DoRecall();

	/* Add thisptr into params buffer */
	unsigned char *newparams = new unsigned char[sizeof(void *) + m_ParamSize];
	*(void **)newparams = m_pSH->GetIfacePtr();
	memcpy(newparams + sizeof(void *), params, m_ParamSize);

	/* Execute the call */
	m_CallWrapper->Execute(newparams, retval);

	m_pSH->SetRes(MRES_SUPERCEDE);

	return;
}

void HookWrapper::Destroy()
{
	if (m_HookWrapper != NULL)
	{
		JIT_FreeHook(m_HookWrapper);
	}

	if (m_CallWrapper != NULL)
	{
		m_CallWrapper->Destroy();
	}

	delete this;
}

unsigned int HookWrapper::GetParamSize()
{
	return m_ParamSize;
}

SourceHook::ProtoInfo * HookWrapper::GetProtoInfo()
{
	return &m_ProtoInfo;
}

unsigned int HookWrapper::GetRetSize()
{
	return m_RetSize;
}

void * HookWrapper::GetHandlerAddr()
{
	return m_FuncAddr;
}

void HookWrapper::SetCallWrapperAddr( ICallWrapper *wrap )
{
	m_CallWrapper = wrap;
}

void HookWrapper::SetHookWrpAddr( void *addr )
{
	m_HookWrapper = addr;
}

HookWrapper::~HookWrapper()
{
	delete [] m_ProtoInfo.paramsPassInfo;
	delete [] m_ParamOffs;
}
/*****************************
 * SMDelegate Implementation *
 *****************************/

SMDelegate::SMDelegate(void *data)
{
	m_Data = data;
}

bool SMDelegate::IsEqual(ISHDelegate *pOtherDeleg)
{
	return false;
}

void SMDelegate::DeleteThis()
{
	/* Remove our allocated vtable */
	delete [] *reinterpret_cast<void ***>(this);

	delete this;
}

void SMDelegate::Call()
{
}

void *SMDelegate::GetUserData()
{
	return m_Data;
}

/** 
 * This duplicates the vtable (so each instance has a unique table) and patches SMDelgate::Call to point to our allocated function 
 */
void SMDelegate::PatchVtable(void *addr)
{
	void **new_vtptr = new void *[4];
	void **cur_vtptr = *reinterpret_cast<void ***>(this);
	memcpy(new_vtptr, cur_vtptr, sizeof(void *)*4);

	*reinterpret_cast<void ***>(this) = new_vtptr;

	void *cur_vfnptr = reinterpret_cast<void *>(new_vtptr + VTABLE_PATCH_OFFS);

	*reinterpret_cast<void **>(cur_vfnptr) = addr;
}

#endif
