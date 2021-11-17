/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Hooks Extension
 * Copyright (C) 2012-2021 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_VFUNC_CALL_H_
#define _INCLUDE_VFUNC_CALL_H_

#include "vhook.h"
#include "extension.h"
#include "util.h"

#define PARAMINFO_SWITCH(passType) \
		paramInfo[i].flags = dg->params.at(i).flags; \
		paramInfo[i].size = dg->params.at(i).size; \
		paramInfo[i].type = passType;

#define VSTK_PARAM_SWITCH(paramType) \
		if(paramStruct->isChanged[i]) \
		{ \
			*(paramType *)vptr = *(paramType *)newAddr; \
		} \
		else \
		{ \
			*(paramType *)vptr = *(paramType *)orgAddr; \
		} \
		if(i + 1 != dg->params.size()) \
		{ \
			vptr += dg->params.at(i).size; \
		} \
		break;

#define VSTK_PARAM_SWITCH_OBJECT() \
		memcpy(vptr, objAddr, dg->params.at(i).size); \
		if(i + 1 != dg->params.size()) \
		{ \
			vptr += dg->params.at(i).size; \
		} \
		break;

template <class T>
T CallVFunction(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(T);
		if( dg->returnType != ReturnType_Vector)
		{
			returnInfo.type = PassType_Basic;
		}
		else
		{
			returnInfo.type = PassType_Object;
		}
	}

	ICallWrapper *pCall;

	size_t size = GetParamsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());

		for(int i = 0; i < (int)dg->params.size(); i++)
		{
			size_t offset = GetParamOffset(paramStruct, i);

			void *orgAddr = (void **)((intptr_t)paramStruct->orgParams + offset);
			void *newAddr = (void **)((intptr_t)paramStruct->newParams + offset);

			switch(dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH(float);
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(string_t);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(SDKVector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				case HookParamType_Object:
				{
					void *objAddr = GetObjectAddr(HookParamType_Object, paramStruct->dg->params.at(i).flags, paramStruct->orgParams, offset);
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH_OBJECT();
				}
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	T ret = 0;

	if(dg->returnType == ReturnType_Void)
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, NULL, paramInfo, dg->params.size());
		pCall->Execute(vstk, NULL);
	}
	else
	{
		pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
		pCall->Execute(vstk, &ret);
	}

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
template <>
SDKVector CallVFunction<SDKVector>(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(SDKVector);
		returnInfo.type = PassType_Object;
	}

	ICallWrapper *pCall;

	size_t size = GetParamsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());
		for(int i = 0; i < (int)dg->params.size(); i++)
		{
			size_t offset = GetParamOffset(paramStruct, i);

			void *orgAddr = *(void **)((intptr_t)paramStruct->orgParams + offset);
			void *newAddr = *(void **)((intptr_t)paramStruct->newParams + offset);

			switch (dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH(float);
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(string_t);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(SDKVector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				case HookParamType_Object:
				{
					void *objAddr = GetObjectAddr(HookParamType_Object, paramStruct->dg->params.at(i).flags, paramStruct->orgParams, offset);
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH_OBJECT();
				}
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	SDKVector ret;

	pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
	pCall->Execute(vstk, &ret);

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
#ifndef WIN32
template <>
string_t CallVFunction<string_t>(DHooksCallback *dg, HookParamsStruct *paramStruct, void *iface)
{
	SourceMod::PassInfo *paramInfo = NULL;
	SourceMod::PassInfo returnInfo;

	if(dg->returnType != ReturnType_Void)
	{
		returnInfo.flags = dg->returnFlag;
		returnInfo.size = sizeof(string_t);
		returnInfo.type = PassType_Object;
	}

	ICallWrapper *pCall;

	size_t size = GetParamsSize(dg);

	unsigned char *vstk = (unsigned char *)malloc(sizeof(void *) + size);
	unsigned char *vptr = vstk;

	*(void **)vptr = iface;

	if(paramStruct)
	{
		vptr += sizeof(void *);
		paramInfo = (SourceMod::PassInfo *)malloc(sizeof(SourceMod::PassInfo) * dg->params.size());
		for(int i = 0; i < dg->params.size(); i++)
		{
			size_t offset = GetParamOffset(paramStruct, i);

			void *orgAddr = *(void **)((intptr_t)paramStruct->orgParams + offset);
			void *newAddr = *(void **)((intptr_t)paramStruct->newParams + offset);

			switch (dg->params.at(i).type)
			{
				case HookParamType_Int:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(int);
				case HookParamType_Bool:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(cell_t);
				case HookParamType_Float:
					PARAMINFO_SWITCH(PassType_Float);
					VSTK_PARAM_SWITCH(float);
				case HookParamType_String:
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH(string_t);
				case HookParamType_StringPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(string_t *);
				case HookParamType_CharPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(char *);
				case HookParamType_VectorPtr:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(SDKVector *);
				case HookParamType_CBaseEntity:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(CBaseEntity *);
				case HookParamType_Edict:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(edict_t *);
				case HookParamType_Object:
				{
					void *objAddr = GetObjectAddr(HookParamType_Object, paramStruct->dg->params.at(i).flags, paramStruct->orgParams, offset);
					PARAMINFO_SWITCH(PassType_Object);
					VSTK_PARAM_SWITCH_OBJECT();
				}
				default:
					PARAMINFO_SWITCH(PassType_Basic);
					VSTK_PARAM_SWITCH(void *);
			}
		}
	}

	string_t ret;

	pCall = g_pBinTools->CreateVCall(dg->offset, 0, 0, &returnInfo, paramInfo, dg->params.size());
	pCall->Execute(vstk, &ret);

	pCall->Destroy();
	free(vstk);

	if(paramInfo != NULL)
	{
		free(paramInfo);
	}

	return ret;
}
#endif
#endif
