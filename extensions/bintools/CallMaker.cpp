/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC. All rights reserved.
 * ===============================================================
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#include "CallMaker.h"
#include "CallWrapper.h"
#include "jit_call.h"

ICallWrapper *CallMaker::CreateCall(void *address,
						 CallConvention cv,
						 const PassInfo *retInfo,
						 const PassInfo paramInfo[],
						 unsigned int numParams)
{
	CallWrapper *pWrapper = new CallWrapper(cv, paramInfo, retInfo, numParams);
	pWrapper->m_Addrs[ADDR_CALLEE] = address;

	JIT_Compile(pWrapper, FuncAddr_Direct);

	return pWrapper;
}

ICallWrapper *CallMaker::CreateVCall(unsigned int vtblIdx, 
									 unsigned int vtblOffs, 
									 unsigned int thisOffs, 
									 const PassInfo *retInfo, 
									 const PassInfo paramInfo[], 
									 unsigned int numParams)
{
	CallWrapper *pWrapper = new CallWrapper(CallConv_ThisCall, paramInfo, retInfo, numParams);
	pWrapper->m_VtInfo.thisOffs = thisOffs;
	pWrapper->m_VtInfo.vtblIdx = vtblIdx;
	pWrapper->m_VtInfo.vtblOffs = vtblOffs;

	JIT_Compile(pWrapper, FuncAddr_VTable);

	return pWrapper;
}
