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
