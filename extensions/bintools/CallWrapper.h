/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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
	void Destroy();
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
