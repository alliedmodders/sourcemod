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

#include "extension.h"
#include "CallWrapper.h"

CallWrapper::CallWrapper(CallConvention cv, const PassInfo *paramInfo, const PassInfo *retInfo, unsigned int numParams)
{
	m_Cv = cv;

	if (numParams)
	{
		m_Params = new PassEncode[numParams];
		for (size_t i=0; i<numParams; i++)
		{
			m_Params[i].info = paramInfo[i];
		}
	}

	if (retInfo)
	{
		m_RetParam = new PassInfo;
		*m_RetParam = *retInfo;
	} else {
		m_RetParam = NULL;
	}

	m_NumParams = numParams;

	/* Calculate virtual stack offsets for each parameter */
	size_t offs = 0;

	if (cv == CallConv_ThisCall)
	{
		offs += sizeof(void *);
	}
	for (size_t i=0; i<numParams; i++)
	{
		m_Params[i].offset = offs;
		offs += m_Params[i].info.size;
	}
}

CallWrapper::~CallWrapper()
{
	delete [] m_Params;
	delete m_RetParam;
	g_SPEngine->ExecFree(m_Addrs[ADDR_CODEBASE]);
}

CallConvention CallWrapper::GetCallConvention()
{
	return m_Cv;
}

const PassEncode *CallWrapper::GetParamInfo(unsigned int num)
{
	return (num+1 > m_NumParams) ? NULL : &m_Params[num];
}

const PassInfo *CallWrapper::GetReturnInfo()
{
	return m_RetParam;
}

unsigned int CallWrapper::GetParamCount()
{
	return m_NumParams;
}

void CallWrapper::Execute(void *vParamStack, void *retBuffer)
{
	typedef void (*CALL_EXECUTE)(void *, void *);
	CALL_EXECUTE fn = (CALL_EXECUTE)m_Addrs[ADDR_CODEBASE];
	fn(vParamStack, retBuffer);
}
