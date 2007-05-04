/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
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