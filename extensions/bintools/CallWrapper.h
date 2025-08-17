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

#ifndef _INCLUDE_SOURCEMOD_CALLWRAPPER_H_
#define _INCLUDE_SOURCEMOD_CALLWRAPPER_H_

#include <IBinTools.h>

using namespace SourceMod;

enum FuncAddrMethod
{
	FuncAddr_Direct,
	FuncAddr_VTable
};

struct ProtoInfo
{
	int numOfParams;			//!< number of parameters
	PassInfo retPassInfo;		//!< PassInfo for the return value. size=0 -> no retval
	const PassInfo *paramsPassInfo;	//!< PassInfos for the parameters

	// paramsPassInfo[0] is basically a dummy parameter.
	// However, paramsPassInfo[0].size stores the version of the ProtoInfo structure.

	// Extra info:
	int convention;
};

class CallWrapper : public ICallWrapper
{
public:
	CallWrapper(const ProtoInfo *protoInfo);
	CallWrapper(const ProtoInfo *protoInfo, const PassInfo *retInfo,
	            const PassInfo paramInfo[], unsigned int fnFlags);
	~CallWrapper();
public: //ICallWrapper
	CallConvention GetCallConvention();
	const PassEncode *GetParamInfo(unsigned int num);
	const PassInfo *GetReturnInfo();
	unsigned int GetParamCount();
	void Execute(void *vParamStack, void *retBuffer);
	void Destroy();
	unsigned int GetParamOffset(unsigned int num);
	unsigned int GetFunctionFlags();
	void SetVtableIndex(unsigned int);
	unsigned int GetVtableIndex();
public:
	void SetCalleeAddr(void *addr);
	void SetCodeBaseAddr(void *addr);
	void *GetCalleeAddr();
	void *GetCodeBaseAddr();
private:
	PassEncode *m_Params;
	ProtoInfo m_Info;
	PassInfo *m_RetParam;
	void *m_AddrCallee;
	void *m_AddrCodeBase;
	unsigned int m_FnFlags;
};

#endif //_INCLUDE_SOURCEMOD_CALLWRAPPER_H_
