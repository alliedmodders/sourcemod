/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod SDK Tools Extension
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

#include "vcallbuilder.h"
#include "extension.h"

ValveCall::ValveCall()
{
	call = NULL;
	vparams = NULL;
	retinfo = NULL;
	thisinfo = NULL;
	retbuf = NULL;
}

ValveCall::~ValveCall()
{
	while (!stk.empty())
	{
		unsigned char *ptr = stk.front();
		delete [] ptr;
		stk.pop();
	}
	if (call)
	{
		call->Destroy();
	}
	delete [] retbuf;
	delete [] vparams;
}

unsigned char *ValveCall::stk_get()
{
	unsigned char *ptr;
	if (stk.empty())
	{
		ptr = new unsigned char[stackSize];
	} else {
		ptr = stk.front();
		stk.pop();
	}
	return ptr;
}

void ValveCall::stk_put(unsigned char *ptr)
{
	stk.push(ptr);
}

ValveCall *CreateValveCall(void *addr,
						   ValveCallType vcalltype,
						   const ValvePassInfo *retInfo,
						   const ValvePassInfo *params,
						   unsigned int numParams)
{
	if (numParams > 32)
	{
		return NULL;
	}

	ValveCall *vc = new ValveCall;

	vc->type = vcalltype;

	size_t size = 0;
	vc->stackSize = 0;

	/* Get return information - encode only */
	PassInfo retBuf;
	size_t retBufSize = 0;
	bool retbuf_needs_extra;
	if (retInfo)
	{
		if ((size = ValveParamToBinParam(retInfo->vtype, retInfo->type, retInfo->flags, &retBuf, retbuf_needs_extra)) == 0)
		{
			delete vc;
			return NULL;
		}
		retBufSize = retBuf.size;
		if (retbuf_needs_extra)
		{
			retBufSize += size;
		}
	}

	/* Get parameter info */
	PassInfo paramBuf[32];
	size_t sizes[32];
	size_t normSize = 0;
	size_t extraSize = 0;
	for (unsigned int i=0; i<numParams; i++)
	{
		bool needs_extra;
		if ((size = ValveParamToBinParam(params[i].vtype, 
			params[i].type,
			params[i].flags,
			&paramBuf[i],
			needs_extra)) == 0)
		{
			delete vc;
			return NULL;
		}
		if (needs_extra)
		{
			sizes[i] = size;
		} else {
			sizes[i] = 0;
		}
		normSize += paramBuf[i].size;
		extraSize += sizes[i];
	}


	/* Get thisinfo if needed */
	ValvePassInfo thisbuf;
	ValvePassInfo *thisinfo = NULL;
	CallConvention cv = CallConv_Cdecl;
	if (vcalltype != ValveCall_Static)
	{
		thisinfo = &thisbuf;
		thisinfo->type = PassType_Basic;
		switch (vcalltype)
		{
		case ValveCall_Entity:
			thisinfo->vtype = Valve_CBaseEntity;
			thisinfo->flags = PASSFLAG_BYVAL;
			thisinfo->decflags |= VDECODE_FLAG_ALLOWWORLD;
			break;
		case ValveCall_Player:
			thisinfo->vtype = Valve_CBasePlayer;
			thisinfo->flags = PASSFLAG_BYVAL;
			thisinfo->decflags = 0;
			break;
		case ValveCall_GameRules:
			thisinfo->vtype = Valve_POD;
			thisinfo->flags = PASSFLAG_ASPOINTER;
			thisinfo->decflags = 0;
		}
		thisinfo->encflags = 0;
		thisinfo->offset = 0;
		normSize += sizeof(void *);
		cv = CallConv_ThisCall;
	}

	/* Now we can try creating the call */
	if ((vc->call = g_pBinTools->CreateCall(addr,
		cv,
		(retInfo ? &retBuf : NULL),
		paramBuf,
		numParams))
		== NULL)
	{
		if (!vc->call)
		{
			delete vc;
			return NULL;
		}
	}

	/* Allocate extra space for thisptr AND ret buffer, even if we don't use it */
	vc->vparams = new ValvePassInfo[numParams + 2];

	/* We've got the call and everything is encoded.
	 * It's time to save the valve specific information and helper variables.
	 */
	if (retInfo)
	{
		/* Allocate and copy */
		vc->retinfo = &(vc->vparams[numParams]);
		*vc->retinfo = *retInfo;
		vc->retinfo->offset = 0;
		vc->retinfo->obj_offset = retbuf_needs_extra ? sizeof(void *) : 0;
		/* Allocate stack space */
		vc->retbuf = new unsigned char[retBufSize];
	} else {
		vc->retinfo = NULL;
		vc->retbuf = NULL;
	}

	if (thisinfo)
	{
		/* Allocate and copy */
		vc->thisinfo = &(vc->vparams[numParams + 1]);
		*vc->thisinfo = *thisinfo;
		vc->thisinfo->offset = 0;
		vc->thisinfo->obj_offset = 0;
	} else {
		vc->thisinfo = NULL;
	}

	/* Now, save info about each parameter. */
	size_t last_extra_offset = 0;
	for (unsigned int i=0; i<numParams; i++)
	{
		/* Copy */
		vc->vparams[i] = params[i];
		vc->vparams[i].offset = vc->call->GetParamInfo(i)->offset;
		vc->vparams[i].obj_offset = last_extra_offset;
		last_extra_offset += sizes[i];
	}

	vc->stackSize = normSize + extraSize;
	vc->stackEnd = normSize;

	return vc;
}

ValveCall *CreateValveVCall(unsigned int vtableIdx,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams)
{
	if (numParams > 32)
	{
		return NULL;
	}

	ValveCall *vc = new ValveCall;

	vc->type = vcalltype;

	size_t size = 0;
	vc->stackSize = 0;

	/* Get return information - encode only */
	PassInfo retBuf;
	size_t retBufSize = 0;
	bool retbuf_needs_extra;
	if (retInfo)
	{
		if ((size = ValveParamToBinParam(retInfo->vtype, retInfo->type, retInfo->flags, &retBuf, retbuf_needs_extra)) == 0)
		{
			delete vc;
			return NULL;
		}
		retBufSize = retBuf.size;
		if (retbuf_needs_extra)
		{
			retBufSize += size;
		}
	}

	/* Get parameter info */
	PassInfo paramBuf[32];
	size_t sizes[32];
	size_t normSize = 0;
	size_t extraSize = 0;
	for (unsigned int i=0; i<numParams; i++)
	{
		bool needs_extra;
		if ((size = ValveParamToBinParam(params[i].vtype, 
										params[i].type,
										params[i].flags,
										&paramBuf[i],
										needs_extra)) == 0)
		{
			delete vc;
			return NULL;
		}
		if (needs_extra)
		{
			sizes[i] = size;
		} else {
			sizes[i] = 0;
		}
		normSize += paramBuf[i].size;
		extraSize += sizes[i];
	}

	/* Now we can try creating the call */
	if ((vc->call = g_pBinTools->CreateVCall(vtableIdx, 
		0, 
		0, 
		(retInfo ? &retBuf : NULL),
		paramBuf,
		numParams))
		== NULL)
	{
		if (!vc->call)
		{
			delete vc;
			return NULL;
		}
	}

	/* Allocate extra space for thisptr AND ret buffer, even if we don't use it */
	vc->vparams = new ValvePassInfo[numParams + 2];

	/* We've got the call and everything is encoded.
	 * It's time to save the valve specific information and helper variables.
	 */
	if (retInfo)
	{
		/* Allocate and copy */
		vc->retinfo = &(vc->vparams[numParams]);
		*vc->retinfo = *retInfo;
		vc->retinfo->offset = 0;
		vc->retinfo->obj_offset = retbuf_needs_extra ? sizeof(void *) : 0;
		/* Allocate stack space */
		vc->retbuf = new unsigned char[retBufSize];
	} else {
		vc->retinfo = NULL;
		vc->retbuf = NULL;
	}

	/* Save the this info for the dynamic decoder */
	vc->thisinfo = &(vc->vparams[numParams + 1]);
	vc->thisinfo->type = PassType_Basic;
	switch (vcalltype)
	{
	case ValveCall_Entity:
		vc->thisinfo->vtype = Valve_CBaseEntity;
		vc->thisinfo->flags = PASSFLAG_BYVAL;
		vc->thisinfo->decflags = VDECODE_FLAG_ALLOWWORLD;
		break;
	case ValveCall_Player:
		vc->thisinfo->vtype = Valve_CBasePlayer;
		vc->thisinfo->flags = PASSFLAG_BYVAL;
		vc->thisinfo->decflags = 0;
		break;
	case ValveCall_GameRules:
		vc->thisinfo->vtype = Valve_POD;
		vc->thisinfo->flags = PASSFLAG_ASPOINTER;
		vc->thisinfo->decflags = 0;
		break;
	}
	vc->thisinfo->encflags = 0;
	vc->thisinfo->offset = 0;
	vc->thisinfo->obj_offset = 0;
	normSize += sizeof(void *);

	/* Now, save info about each parameter. */
	size_t last_extra_offset = 0;
	for (unsigned int i=0; i<numParams; i++)
	{
		/* Copy */
		vc->vparams[i] = params[i];
		vc->vparams[i].offset = vc->call->GetParamInfo(i)->offset;
		vc->vparams[i].obj_offset = last_extra_offset;
		last_extra_offset += sizes[i];
	}

	vc->stackSize = normSize + extraSize;
	vc->stackEnd = normSize;

	return vc;
}
