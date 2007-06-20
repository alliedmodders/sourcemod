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

	size_t size = 0;
	vc->stackSize = 0;

	/* Get return information - encode only*/
	PassInfo retBuf;
	size_t retBufSize = 0;
	if (retInfo)
	{
		if ((size = ValveParamToBinParam(retInfo->vtype, retInfo->type, retInfo->flags, &retBuf)) == 0)
		{
			delete vc;
			return NULL;
		}
		retBufSize = size;
	}

	/* Get thisinfo if needed */
	ValvePassInfo thisbuf;
	ValvePassInfo *thisinfo = NULL;
	CallConvention cv = CallConv_Cdecl;
	if (vcalltype != ValveCall_Static)
	{
		thisinfo = &thisbuf;
		thisinfo->type = PassType_Basic;
		if (vcalltype == ValveCall_Entity)
		{
			thisinfo->vtype = Valve_CBaseEntity;
			thisinfo->decflags |= VDECODE_FLAG_ALLOWWORLD;
		} else if (vcalltype == ValveCall_Player) {
			thisinfo->vtype = Valve_CBasePlayer;
			thisinfo->decflags = 0;
		}
		thisinfo->encflags = 0;
		thisinfo->flags = PASSFLAG_BYVAL;
		thisinfo->offset = 0;
		vc->stackSize += sizeof(void *);
		cv = CallConv_ThisCall;
	}

	/* Get parameter info */
	PassInfo paramBuf[32];
	for (unsigned int i=0; i<numParams; i++)
	{
		if ((size = ValveParamToBinParam(params[i].vtype, params[i].type, params[i].flags, &paramBuf[i])) == 0)
		{
			delete vc;
			return NULL;
		}
		vc->stackSize += size;
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
	} else {
		vc->thisinfo = NULL;
	}

	/* Now, save info about each parameter. */
	for (unsigned int i=0; i<numParams; i++)
	{
		/* Copy */
		vc->vparams[i] = params[i];
		vc->vparams[i].offset = vc->call->GetParamInfo(i)->offset;
	}

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

	size_t size = 0;
	vc->stackSize = 0;

	/* Get return information - encode only*/
	PassInfo retBuf;
	size_t retBufSize = 0;
	if (retInfo)
	{
		if ((size = ValveParamToBinParam(retInfo->vtype, retInfo->type, retInfo->flags, &retBuf)) == 0)
		{
			delete vc;
			return NULL;
		}
		retBufSize = size;
	}

	/* Get parameter info */
	PassInfo paramBuf[32];
	for (unsigned int i=0; i<numParams; i++)
	{
		if ((size = ValveParamToBinParam(params[i].vtype, params[i].type, params[i].flags, &paramBuf[i])) == 0)
		{
			delete vc;
			return NULL;
		}
		vc->stackSize += size;
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
		/* Allocate stack space */
		vc->retbuf = new unsigned char[retBufSize];
	} else {
		vc->retinfo = NULL;
		vc->retbuf = NULL;
	}

	/* Save the this info for the dynamic decoder */
	vc->thisinfo = &(vc->vparams[numParams + 1]);
	vc->thisinfo->type = PassType_Basic;
	if (vcalltype == ValveCall_Entity)
	{
		vc->thisinfo->vtype = Valve_CBaseEntity;
		vc->thisinfo->decflags = VDECODE_FLAG_ALLOWWORLD;
	} else if (vcalltype == ValveCall_Player) {
		vc->thisinfo->vtype = Valve_CBasePlayer;
		vc->thisinfo->decflags = 0;
	}
	vc->thisinfo->encflags = 0;
	vc->thisinfo->flags = PASSFLAG_BYVAL;
	vc->thisinfo->offset = 0;
	vc->stackSize += sizeof(void *);

	/* Now, save info about each parameter. */
	for (unsigned int i=0; i<numParams; i++)
	{
		/* Copy */
		vc->vparams[i] = params[i];
		vc->vparams[i].offset = vc->call->GetParamInfo(i)->offset;
	}

	return vc;
}
