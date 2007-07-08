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

#include "extension.h"
#include <worldsize.h>

class CSMTraceFilter : public CTraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity *pEntity, int contentsMask)
	{
		cell_t res = 1;
		edict_t *pEdict = gameents->BaseEntityToEdict(reinterpret_cast<CBaseEntity *>(pEntity));
		m_pFunc->PushCell(engine->IndexOfEdict(pEdict));
		m_pFunc->PushCell(contentsMask);
		m_pFunc->Execute(&res);

		return (res) ? true : false;
	}
	void SetFunctionPtr(IPluginFunction *pFunc)
	{
		m_pFunc = pFunc;
	}
private:
	IPluginFunction *m_pFunc;
};

/* Used for the global trace version */
Ray_t g_Ray;
trace_t g_Trace;
Vector g_StartVec;
Vector g_EndVec;
QAngle g_DirAngles;
CTraceFilterHitAll g_HitAllFilter;
CSMTraceFilter g_SMTraceFilter;

enum
{
	RayType_EndPoint,
	RayType_Infinite
};

static cell_t smn_TRTraceRay(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	switch (params[4])
	{
	case RayType_EndPoint:
		{
			g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			break;
		}
	case RayType_Infinite:
		{
			g_DirAngles.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			AngleVectors(g_DirAngles, &g_EndVec);

			/* Make it unitary and get the ending point */
			g_EndVec.NormalizeInPlace();
			g_EndVec = g_StartVec + g_EndVec * MAX_TRACE_LENGTH;
			break;
		}
	}

	g_Ray.Init(g_StartVec, g_EndVec);
	enginetrace->TraceRay(g_Ray, params[3], &g_HitAllFilter, &g_Trace);

	return 1;
}

static cell_t smn_TRTraceRayFilter(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr;
	IPluginFunction *pFunc;

	pFunc = pContext->GetFunctionById(params[5]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}
	g_SMTraceFilter.SetFunctionPtr(pFunc);
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	switch (params[4])
	{
	case RayType_EndPoint:
		{
			g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			break;
		}
	case RayType_Infinite:
		{
			g_DirAngles.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			AngleVectors(g_DirAngles, &g_EndVec);

			/* Make it unitary and get the ending point */
			g_EndVec.NormalizeInPlace();
			g_EndVec = g_StartVec + g_EndVec * MAX_TRACE_LENGTH;
			break;
		}
	}

	g_Ray.Init(g_StartVec, g_EndVec);
	enginetrace->TraceRay(g_Ray, params[3], &g_SMTraceFilter, &g_Trace);

	return 1;
}

static cell_t smn_TRTraceRayEx(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);

	Vector StartVec, EndVec;
	Ray_t ray;

	StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	switch (params[4])
	{
	case RayType_EndPoint:
		{
			EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			break;
		}
	case RayType_Infinite:
		{
			QAngle DirAngles;
			DirAngles.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			AngleVectors(DirAngles, &EndVec);

			/* Make it unitary and get the ending point */
			EndVec.NormalizeInPlace();
			EndVec = StartVec + EndVec * MAX_TRACE_LENGTH;
			break;
		}
	}

	trace_t *tr = new trace_t;
	ray.Init(StartVec, EndVec);
	enginetrace->TraceRay(ray, params[3], &g_HitAllFilter, tr);

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRTraceRayFilterEx(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc;
	cell_t *startaddr, *endaddr;

	pFunc = pContext->GetFunctionById(params[5]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);

	Vector StartVec, EndVec;
	CSMTraceFilter smfilter;
	Ray_t ray;

	smfilter.SetFunctionPtr(pFunc);
	StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	switch (params[4])
	{
	case RayType_EndPoint:
		{
			EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			break;
		}
	case RayType_Infinite:
		{
			QAngle DirAngles;
			DirAngles.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));
			AngleVectors(DirAngles, &EndVec);

			/* Make it unitary and get the ending point */
			EndVec.NormalizeInPlace();
			EndVec = StartVec + EndVec * MAX_TRACE_LENGTH;
			break;
		}
	}

	trace_t *tr = new trace_t;
	ray.Init(StartVec, EndVec);
	enginetrace->TraceRay(ray, params[3], &smfilter, tr);

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRGetFraction(IPluginContext *pContext, const cell_t *params)
{
	trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return sp_ftoc(tr->fraction);
}

static cell_t smn_TRGetEndPosition(IPluginContext *pContext, const cell_t *params)
{
	trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[2] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[2], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[2], err);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[1], &addr);

	addr[0] = sp_ftoc(tr->endpos.x);
	addr[1] = sp_ftoc(tr->endpos.y);
	addr[2] = sp_ftoc(tr->endpos.z);

	return 1;
}

static cell_t smn_TRDidHit(IPluginContext *pContext, const cell_t *params)
{
	trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->DidHit() ? 1 : 0;
}

static cell_t smn_TRGetHitGroup(IPluginContext *pContext, const cell_t *params)
{
	trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->hitgroup;
}

static cell_t smn_TRGetEntityIndex(IPluginContext *pContext, const cell_t *params)
{
	trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	edict_t *pEdict = gameents->BaseEntityToEdict(tr->m_pEnt);
	return engine->IndexOfEdict(pEdict);
}

static cell_t smn_TRGetPointContents(IPluginContext *pContext, const cell_t *params)
{
	cell_t *vec, *ent;
	IHandleEntity *hentity;
	Vector pos;
	int mask;

	pContext->LocalToPhysAddr(params[1], &vec);
	pContext->LocalToPhysAddr(params[2], &ent);

	pos.x = sp_ctof(vec[0]);
	pos.y = sp_ctof(vec[1]);
	pos.z = sp_ctof(vec[2]);

	if (*ent == -1)
	{
		mask = enginetrace->GetPointContents(pos);
	} else {
		mask = enginetrace->GetPointContents(pos, &hentity);
		edict_t *pEdict = gameents->BaseEntityToEdict(reinterpret_cast<CBaseEntity *>(hentity));
		*ent = engine->IndexOfEdict(pEdict);
	}

	return mask;
}

static cell_t smn_TRGetPointContentsEnt(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = engine->PEntityOfEntIndex(params[1]);
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[1]);
	}

	cell_t *addr;
	Vector pos;

	pContext->LocalToPhysAddr(params[2], &addr);

	pos.x = sp_ctof(addr[0]);
	pos.y = sp_ctof(addr[1]);
	pos.z = sp_ctof(addr[2]);

	return enginetrace->GetPointContents_Collideable(pEdict->GetCollideable(), pos);
}

sp_nativeinfo_t g_TRNatives[] = 
{
	{"TR_TraceRay",				smn_TRTraceRay},
	{"TR_TraceRayEx",			smn_TRTraceRayEx},
	{"TR_GetFraction",			smn_TRGetFraction},
	{"TR_GetEndPosition",		smn_TRGetEndPosition},
	{"TR_GetEntityIndex",		smn_TRGetEntityIndex},
	{"TR_DidHit",				smn_TRDidHit},
	{"TR_GetHitGroup",			smn_TRGetHitGroup},
	{"TR_GetPointContents",		smn_TRGetPointContents},
	{"TR_GetPointContentsEnt",	smn_TRGetPointContentsEnt},
	{"TR_TraceRayFilter",		smn_TRTraceRayFilter},
	{"TR_TraceRayFilterEx",		smn_TRTraceRayFilterEx},
	{NULL,						NULL}
};
