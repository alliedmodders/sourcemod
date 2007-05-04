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

#ifndef _INCLUDE_SOURCEMOD_CCALLWRAPPER_H_
#define _INCLUDE_SOURCEMOD_CCALLWRAPPER_H_

#include <IBinTools.h>

using namespace SourceMod;

#define ADDR_CALLEE			0
#define ADDR_CODEBASE		1

struct VtableInfo 
{
	unsigned int vtblIdx;
	unsigned int vtblOffs;
	unsigned int thisOffs;
};

class CallWrapper : public ICallWrapper
{
public:
	CallWrapper(CallConvention cv, const PassInfo *paramInfo, const PassInfo *retInfo, unsigned int numParams);
	~CallWrapper();
public: //ICallWrapper
	CallConvention GetCallConvention();
	const PassEncode *GetParamInfo(unsigned int num);
	const PassInfo *GetReturnInfo();
	unsigned int GetParamCount();
	void Execute(void *vParamStack, void *retBuffer);
public:
	inline void deleteThis() { delete this; }
	void *m_Addrs[4];
	VtableInfo m_VtInfo;
private:
	CallConvention m_Cv;
	PassEncode *m_Params;
	PassInfo *m_RetParam;
	unsigned int m_NumParams;
};

#endif //_INCLUDE_SOURCEMOD_CCALLWRAPPER_H_
