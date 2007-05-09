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
