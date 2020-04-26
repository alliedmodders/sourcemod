/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
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

#include "extension.h"
#include <worldsize.h>

class sm_trace_t : public trace_t
{
public:
	cell_t GetEntRef() const { return m_EntRef; }
	void UpdateEntRef()
	{
		if (m_pEnt)
		{
			m_EntRef = gamehelpers->EntityToReference(m_pEnt);
		}
		else
		{
			m_EntRef = INVALID_EHANDLE_INDEX;
		}
	}
private:
	cell_t m_EntRef = INVALID_EHANDLE_INDEX;
};

class CSMTraceFilter : public CTraceFilter
{
public:
	bool ShouldHitEntity(IHandleEntity *pEntity, int contentsMask)
	{
		cell_t res = 1;
		m_pFunc->PushCell(gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(pEntity)));
		m_pFunc->PushCell(contentsMask);
		m_pFunc->PushCell(m_Data);
		m_pFunc->Execute(&res);

		return (res) ? true : false;
	}
	void SetFunctionPtr(IPluginFunction *pFunc, cell_t data)
	{
		m_pFunc = pFunc;
		m_Data = data;
	}
private:
	IPluginFunction *m_pFunc;
	cell_t m_Data;
};

class CSMTraceEnumerator : public IPartitionEnumerator
{
public:
	IterationRetval_t EnumElement(IHandleEntity *pEntity) override
	{
		cell_t res = 1;
		m_pFunc->PushCell(gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity*>(pEntity)));
		m_pFunc->PushCell(m_Data);
		m_pFunc->Execute(&res);

		return (res) ? ITERATION_CONTINUE : ITERATION_STOP;
	}
	void SetFunctionPtr(IPluginFunction *pFunc, cell_t data)
	{
		m_pFunc = pFunc;
		m_Data = data;
	}
private:
	IPluginFunction *m_pFunc;
	cell_t m_Data;
};

/* Used for the global trace version */
Ray_t g_Ray;
sm_trace_t g_Trace;
Vector g_StartVec;
Vector g_EndVec;
Vector g_HullMins;
Vector g_HullMaxs;
QAngle g_DirAngles;
CTraceFilterHitAll g_HitAllFilter;
CSMTraceFilter g_SMTraceFilter;
CSMTraceEnumerator g_SMTraceEnumerator;

enum
{
	RayType_EndPoint,
	RayType_Infinite
};

// For backwards compatibility, old EnumerateEntities functions accepted bool instead of flags
static int TranslatePartitionFlags(int input)
{
	switch (input)
	{
	case 0:
		return PARTITION_ENGINE_SOLID_EDICTS;
	case 1:
		return PARTITION_ENGINE_TRIGGER_EDICTS;
	default:
		return input >> 1;
	}
}

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
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TRTraceHull(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr, *mins, *maxs;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	g_HullMins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	g_HullMaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	g_Ray.Init(g_StartVec, g_EndVec, g_HullMins, g_HullMaxs);
	enginetrace->TraceRay(g_Ray, params[5], &g_HitAllFilter, &g_Trace);
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TREnumerateEntities(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[5]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}

	cell_t data = 0;
	if (params[0] >= 6)
	{
		data = params[6];
	}

	g_SMTraceEnumerator.SetFunctionPtr(pFunc, data);

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

	int mask = TranslatePartitionFlags(params[3]);
	partition->EnumerateElementsAlongRay(mask, g_Ray, false, &g_SMTraceEnumerator);

	return 1;
}

static cell_t smn_TREnumerateEntitiesHull(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[6]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[6]);
	}

	cell_t data = 0;
	if (params[0] >= 7)
	{
		data = params[7];
	}

	g_SMTraceEnumerator.SetFunctionPtr(pFunc, data);

	cell_t *startaddr, *endaddr, *mins, *maxs;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	g_HullMins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	g_HullMaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	g_Ray.Init(g_StartVec, g_EndVec, g_HullMins, g_HullMaxs);

	int mask = TranslatePartitionFlags(params[5]);
	partition->EnumerateElementsAlongRay(mask, g_Ray, false, &g_SMTraceEnumerator);

	return 1;
}

static cell_t smn_TREnumerateEntitiesSphere(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[4]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[4]);
	}

	cell_t data = 0;
	if (params[0] >= 5)
	{
		data = params[5];
	}

	g_SMTraceEnumerator.SetFunctionPtr(pFunc, data);

	cell_t *startaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	float radius = sp_ctof(params[2]);

	int mask = TranslatePartitionFlags(params[3]);
	partition->EnumerateElementsInSphere(mask, g_StartVec, radius, false, &g_SMTraceEnumerator);

	return 1;
}

static cell_t smn_TREnumerateEntitiesBox(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[4]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[4]);
	}

	cell_t data = 0;
	if (params[0] >= 5)
	{
		data = params[5];
	}

	g_SMTraceEnumerator.SetFunctionPtr(pFunc, data);

	cell_t *minsaddr, *maxsaddr;
	pContext->LocalToPhysAddr(params[1], &minsaddr);
	pContext->LocalToPhysAddr(params[2], &maxsaddr);

	g_HullMins.Init(sp_ctof(minsaddr[0]), sp_ctof(minsaddr[1]), sp_ctof(minsaddr[2]));
	g_HullMaxs.Init(sp_ctof(maxsaddr[0]), sp_ctof(maxsaddr[1]), sp_ctof(maxsaddr[2]));

	int mask = TranslatePartitionFlags(params[3]);
	partition->EnumerateElementsInBox(mask, g_HullMins, g_HullMaxs, false, &g_SMTraceEnumerator);

	return 1;
}

static cell_t smn_TREnumerateEntitiesPoint(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc = pContext->GetFunctionById(params[3]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[3]);
	}

	cell_t data = 0;
	if (params[0] >= 4)
	{
		data = params[4];
	}

	g_SMTraceEnumerator.SetFunctionPtr(pFunc, data);

	cell_t *startaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));

	int mask = TranslatePartitionFlags(params[2]);
	partition->EnumerateElementsAtPoint(mask, g_StartVec, false, &g_SMTraceEnumerator);

	return 1;
}

static cell_t smn_TRClipRayToEntity(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	cell_t *endaddr;
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

	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[5]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[5]);
	}

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());
	g_Ray.Init(g_StartVec, g_EndVec);
	enginetrace->ClipRayToEntity(g_Ray, params[3], pEnt, &g_Trace);
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TRClipRayHullToEntity(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr, *mins, *maxs;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[6]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[6]);
	}

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	g_HullMins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	g_HullMaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	g_Ray.Init(g_StartVec, g_EndVec, g_HullMins, g_HullMaxs);
	enginetrace->ClipRayToEntity(g_Ray, params[5], pEnt, &g_Trace);
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TRClipCurrentRayToEntity(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[2]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[2]);
	}

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());
	enginetrace->ClipRayToEntity(g_Ray, params[1], pEnt, &g_Trace);
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TRTraceRayFilter(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr;
	IPluginFunction *pFunc;
	cell_t data;

	pFunc = pContext->GetFunctionById(params[5]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}

	if (params[0] >= 6)
	{
		data = params[6];
	}
	else
	{
		data = 0;
	}

	g_SMTraceFilter.SetFunctionPtr(pFunc, data);
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
	g_Trace.UpdateEntRef();

	return 1;
}

static cell_t smn_TRTraceHullFilter(IPluginContext *pContext, const cell_t *params)
{
	cell_t data;
	IPluginFunction *pFunc;
	cell_t *startaddr, *endaddr, *mins, *maxs;

	pFunc = pContext->GetFunctionById(params[6]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}

	data = params[7];

	g_SMTraceFilter.SetFunctionPtr(pFunc, data);
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	g_StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	g_HullMins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	g_HullMaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	g_EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	g_Ray.Init(g_StartVec, g_EndVec, g_HullMins, g_HullMaxs);
	enginetrace->TraceRay(g_Ray, params[5], &g_SMTraceFilter, &g_Trace);
	g_Trace.UpdateEntRef();

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

	sm_trace_t *tr = new sm_trace_t;
	ray.Init(StartVec, EndVec);
	enginetrace->TraceRay(ray, params[3], &g_HitAllFilter, tr);
	tr->UpdateEntRef();

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRTraceHullEx(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr, *mins, *maxs;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	Ray_t ray;
	Vector StartVec, EndVec, vmins, vmaxs;

	StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	vmins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	vmaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	ray.Init(StartVec, EndVec, vmins, vmaxs);

	sm_trace_t *tr = new sm_trace_t;
	enginetrace->TraceRay(ray, params[5], &g_HitAllFilter, tr);
	tr->UpdateEntRef();

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRClipRayToEntityEx(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	cell_t *endaddr;
	pContext->LocalToPhysAddr(params[2], &endaddr);

	Vector StartVec, EndVec;

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

	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[5]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[5]);
	}

	Ray_t ray;
	sm_trace_t *tr = new sm_trace_t;

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());
	ray.Init(StartVec, EndVec);
	enginetrace->ClipRayToEntity(ray, params[3], pEnt, tr);
	tr->UpdateEntRef();

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRClipRayHullToEntityEx(IPluginContext *pContext, const cell_t *params)
{
	cell_t *startaddr, *endaddr, *mins, *maxs;
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[6]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[6]);
	}

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());

	Ray_t ray;
	Vector StartVec, EndVec, vmins, vmaxs;

	StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	vmins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	vmaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	ray.Init(StartVec, EndVec, vmins, vmaxs);

	sm_trace_t *tr = new sm_trace_t;
	enginetrace->ClipRayToEntity(ray, params[5], pEnt, tr);
	tr->UpdateEntRef();

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRClipCurrentRayToEntityEx(IPluginContext *pContext, const cell_t *params)
{
	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[2]));
	if (!pEdict || pEdict->IsFree())
	{
		return pContext->ThrowNativeError("Entity %d is invalid", params[2]);
	}

	sm_trace_t *tr = new sm_trace_t;

	IHandleEntity *pEnt = reinterpret_cast<IHandleEntity*>(pEdict->GetUnknown()->GetBaseEntity());
	enginetrace->ClipRayToEntity(g_Ray, params[1], pEnt, tr);
	tr->UpdateEntRef();

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
	cell_t data;

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

	if (params[0] >= 6)
	{
		data = params[6];
	}
	else
	{
		data = 0;
	}

	smfilter.SetFunctionPtr(pFunc, data);
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

	sm_trace_t *tr = new sm_trace_t;
	ray.Init(StartVec, EndVec);
	enginetrace->TraceRay(ray, params[3], &smfilter, tr);
	tr->UpdateEntRef();

	HandleError herr;
	Handle_t hndl;
	if (!(hndl=handlesys->CreateHandle(g_TraceHandle, tr, pContext->GetIdentity(), myself->GetIdentity(), &herr)))
	{
		delete tr;
		return pContext->ThrowNativeError("Unable to create a new trace handle (error %d)", herr);
	}

	return hndl;
}

static cell_t smn_TRTraceHullFilterEx(IPluginContext *pContext, const cell_t *params)
{
	IPluginFunction *pFunc;
	cell_t *startaddr, *endaddr, *mins, *maxs;
	cell_t data;

	pFunc = pContext->GetFunctionById(params[6]);
	if (!pFunc)
	{
		return pContext->ThrowNativeError("Invalid function id (%X)", params[5]);
	}
	pContext->LocalToPhysAddr(params[1], &startaddr);
	pContext->LocalToPhysAddr(params[2], &endaddr);
	pContext->LocalToPhysAddr(params[3], &mins);
	pContext->LocalToPhysAddr(params[4], &maxs);

	Vector StartVec, EndVec, vmins, vmaxs;
	CSMTraceFilter smfilter;
	Ray_t ray;

	data = params[7];

	smfilter.SetFunctionPtr(pFunc, data);
	StartVec.Init(sp_ctof(startaddr[0]), sp_ctof(startaddr[1]), sp_ctof(startaddr[2]));
	vmins.Init(sp_ctof(mins[0]), sp_ctof(mins[1]), sp_ctof(mins[2]));
	vmaxs.Init(sp_ctof(maxs[0]), sp_ctof(maxs[1]), sp_ctof(maxs[2]));
	EndVec.Init(sp_ctof(endaddr[0]), sp_ctof(endaddr[1]), sp_ctof(endaddr[2]));

	ray.Init(StartVec, EndVec, vmins, vmaxs);

	sm_trace_t *tr = new sm_trace_t;
	enginetrace->TraceRay(ray, params[5], &smfilter, tr);
	tr->UpdateEntRef();

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
	sm_trace_t *tr;
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

static cell_t smn_TRGetFractionLeftSolid(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return sp_ftoc(tr->fractionleftsolid);
}

static cell_t smn_TRGetPlaneNormal(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	Vector *normal = &tr->plane.normal;

	cell_t *r;
	pContext->LocalToPhysAddr(params[2], &r);
	r[0] = sp_ftoc(normal->x);
	r[1] = sp_ftoc(normal->y);
	r[2] = sp_ftoc(normal->z);

	return 1;
}

static cell_t smn_TRGetStartPosition(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	addr[0] = sp_ftoc(tr->startpos.x);
	addr[1] = sp_ftoc(tr->startpos.y);
	addr[2] = sp_ftoc(tr->startpos.z);

	return 1;
}

static cell_t smn_TRGetEndPosition(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
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

static cell_t smn_TRGetDisplacementFlags(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->dispFlags;
}

static cell_t smn_TRGetSurfaceName(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	pContext->StringToLocal(params[2], params[3], tr->surface.name);

	return 1;
}

static cell_t smn_TRGetSurfaceProps(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->surface.surfaceProps;
}

static cell_t smn_TRGetSurfaceFlags(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->surface.flags;
}

static cell_t smn_TRGetPhysicsBone(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->physicsbone;
}

static cell_t smn_TRAllSolid(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->allsolid ? 1 : 0;
}

static cell_t smn_TRStartSolid(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->startsolid ? 1 : 0;
}

static cell_t smn_TRDidHit(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
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
	sm_trace_t *tr;
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

static cell_t smn_TRGetHitBoxIndex(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		tr = &g_Trace;
	} else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None) {
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return tr->hitbox;
}

static cell_t smn_TRGetEntityIndex(IPluginContext *pContext, const cell_t *params)
{
	sm_trace_t *tr;
	HandleError err;
	HandleSecurity sec(pContext->GetIdentity(), myself->GetIdentity());

	if (params[1] == BAD_HANDLE)
	{
		return gamehelpers->ReferenceToBCompatRef(g_Trace.GetEntRef());
	}
	else if ((err = handlesys->ReadHandle(params[1], g_TraceHandle, &sec, (void **)&tr)) != HandleError_None)
	{
		return pContext->ThrowNativeError("Invalid Handle %x (error %d)", params[1], err);
	}

	return gamehelpers->ReferenceToBCompatRef(tr->GetEntRef());
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
#if SOURCE_ENGINE >= SE_LEFT4DEAD || SOURCE_ENGINE == SE_BMS
		mask = enginetrace->GetPointContents(pos, MASK_ALL, &hentity);
#else
		mask = enginetrace->GetPointContents(pos, &hentity);
#endif
		*ent = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity *>(hentity));
	}

	return mask;
}

static cell_t smn_TRGetPointContentsEnt(IPluginContext *pContext, const cell_t *params)
{
	/* TODO: See if we can get the collidable with a prop and remove the reliance on edicts */
	edict_t *pEdict = PEntityOfEntIndex(gamehelpers->ReferenceToIndex(params[1]));
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

static cell_t smn_TRPointOutsideWorld(IPluginContext *pContext, const cell_t *params)
{
	cell_t *vec;
	Vector pos;

	pContext->LocalToPhysAddr(params[1], &vec);

	pos.x = sp_ctof(vec[0]);
	pos.y = sp_ctof(vec[1]);
	pos.z = sp_ctof(vec[2]);

	return enginetrace->PointOutsideWorld(pos);
}

sp_nativeinfo_t g_TRNatives[] =
{
	{"TR_TraceRay",					smn_TRTraceRay},
	{"TR_TraceHull",				smn_TRTraceHull},
	{"TR_EnumerateEntities",        smn_TREnumerateEntities},
	{"TR_EnumerateEntitiesHull",    smn_TREnumerateEntitiesHull},
	{"TR_EnumerateEntitiesSphere",  smn_TREnumerateEntitiesSphere},
	{"TR_EnumerateEntitiesBox",     smn_TREnumerateEntitiesBox},
	{"TR_EnumerateEntitiesPoint",   smn_TREnumerateEntitiesPoint},
	{"TR_TraceRayEx",				smn_TRTraceRayEx},
	{"TR_TraceHullEx",				smn_TRTraceHullEx},
	{"TR_GetFraction",				smn_TRGetFraction},
	{"TR_GetFractionLeftSolid",		smn_TRGetFractionLeftSolid},
	{"TR_GetStartPosition",			smn_TRGetStartPosition},
	{"TR_GetEndPosition",			smn_TRGetEndPosition},
	{"TR_GetEntityIndex",			smn_TRGetEntityIndex},
	{"TR_GetDisplacementFlags",		smn_TRGetDisplacementFlags },
	{"TR_GetSurfaceName",			smn_TRGetSurfaceName},
	{"TR_GetSurfaceProps",			smn_TRGetSurfaceProps},
	{"TR_GetSurfaceFlags",			smn_TRGetSurfaceFlags},
	{"TR_GetPhysicsBone",			smn_TRGetPhysicsBone},
	{"TR_AllSolid",					smn_TRAllSolid},
	{"TR_StartSolid",				smn_TRStartSolid},
	{"TR_DidHit",					smn_TRDidHit},
	{"TR_GetHitGroup",				smn_TRGetHitGroup},
	{"TR_GetHitBoxIndex",			smn_TRGetHitBoxIndex},
	{"TR_ClipRayToEntity",			smn_TRClipRayToEntity},
	{"TR_ClipRayToEntityEx",		smn_TRClipRayToEntityEx},
	{"TR_ClipRayHullToEntity",		smn_TRClipRayHullToEntity},
	{"TR_ClipRayHullToEntityEx",	smn_TRClipRayHullToEntityEx},
	{"TR_ClipCurrentRayToEntity",	smn_TRClipCurrentRayToEntity},
	{"TR_ClipCurrentRayToEntityEx",	smn_TRClipCurrentRayToEntityEx},
	{"TR_GetPointContents",			smn_TRGetPointContents},
	{"TR_GetPointContentsEnt",		smn_TRGetPointContentsEnt},
	{"TR_TraceRayFilter",			smn_TRTraceRayFilter},
	{"TR_TraceRayFilterEx",			smn_TRTraceRayFilterEx},
	{"TR_TraceHullFilter",			smn_TRTraceHullFilter},
	{"TR_TraceHullFilterEx",		smn_TRTraceHullFilterEx},
	{"TR_GetPlaneNormal",			smn_TRGetPlaneNormal},
	{"TR_PointOutsideWorld",		smn_TRPointOutsideWorld},
	{NULL,							NULL}
};
