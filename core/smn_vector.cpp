/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#include "sm_globals.h"
#include <vector.h>
#include <mathlib.h>

#define SET_VECTOR(addr, vec) \
	addr[0] = sp_ftoc(vec.x); \
	addr[1] = sp_ftoc(vec.y); \
	addr[2] = sp_ftoc(vec.z); 

static cell_t GetVectorLength(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;

	pContext->LocalToPhysAddr(params[1], &addr);

	Vector source(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
	
	if (!params[2])
	{
		return sp_ftoc(source.Length());
	} else {
		return sp_ftoc(source.LengthSqr());
	}
}

static cell_t GetVectorDistance(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr1, *addr2;

	pContext->LocalToPhysAddr(params[1], &addr1);
	pContext->LocalToPhysAddr(params[2], &addr2);

	Vector source(sp_ctof(addr1[0]), sp_ctof(addr1[1]), sp_ctof(addr1[2]));
	Vector dest(sp_ctof(addr2[0]), sp_ctof(addr2[1]), sp_ctof(addr2[2]));

	if (!params[3])
	{
		return sp_ftoc(source.DistTo(dest));
	} else {
		return sp_ftoc(source.DistToSqr(dest));
	}
}

static cell_t GetVectorDotProduct(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr1, *addr2;

	pContext->LocalToPhysAddr(params[1], &addr1);
	pContext->LocalToPhysAddr(params[2], &addr2);

	Vector source(sp_ctof(addr1[0]), sp_ctof(addr1[1]), sp_ctof(addr1[2]));
	Vector dest(sp_ctof(addr2[0]), sp_ctof(addr2[1]), sp_ctof(addr2[2]));

	return sp_ftoc(source.Dot(dest));
}

static cell_t GetVectorCrossProduct(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr1, *addr2, *set;

	pContext->LocalToPhysAddr(params[1], &addr1);
	pContext->LocalToPhysAddr(params[2], &addr2);
	pContext->LocalToPhysAddr(params[3], &set);

	Vector vec1(sp_ctof(addr1[0]), sp_ctof(addr1[1]), sp_ctof(addr1[2]));
	Vector vec2(sp_ctof(addr2[0]), sp_ctof(addr2[1]), sp_ctof(addr2[2]));

	Vector vec3 = vec1.Cross(vec2);

	SET_VECTOR(set, vec3);

	return 1;
}

static cell_t GetAngleVectors(IPluginContext *pContext, const cell_t *params)
{
	cell_t *ang_addr;

	pContext->LocalToPhysAddr(params[1], &ang_addr);

	QAngle angle(sp_ctof(ang_addr[0]), sp_ctof(ang_addr[1]), sp_ctof(ang_addr[2]));
	Vector fwd, right, up;

	AngleVectors(angle, &fwd, &right, &up);

	cell_t *addr_fwd, *addr_right, *addr_up;
	pContext->LocalToPhysAddr(params[2], &addr_fwd);
	pContext->LocalToPhysAddr(params[3], &addr_right);
	pContext->LocalToPhysAddr(params[4], &addr_up);

	SET_VECTOR(addr_fwd, fwd);
	SET_VECTOR(addr_right, right);
	SET_VECTOR(addr_up, up);

	return 1;
}

static cell_t GetVectorAngles(IPluginContext *pContext, const cell_t *params)
{
	cell_t *vec_addr;

	pContext->LocalToPhysAddr(params[1], &vec_addr);

	Vector vec(sp_ctof(vec_addr[0]), sp_ctof(vec_addr[1]), sp_ctof(vec_addr[2]));
	QAngle angle;

	VectorAngles(vec, angle);

	cell_t *ang_addr;
	pContext->LocalToPhysAddr(params[2], &ang_addr);

	SET_VECTOR(ang_addr, angle);

	return 1;
}

static cell_t GetVectorVectors(IPluginContext *pContext, const cell_t *params)
{
	cell_t *vec_addr;

	pContext->LocalToPhysAddr(params[1], &vec_addr);

	Vector vec(sp_ctof(vec_addr[0]), sp_ctof(vec_addr[1]), sp_ctof(vec_addr[2]));
	Vector right, up;

	VectorVectors(vec, right, up);

	cell_t *addr_right, *addr_up;
	pContext->LocalToPhysAddr(params[2], &addr_right);
	pContext->LocalToPhysAddr(params[3], &addr_up);

	SET_VECTOR(addr_right, right);
	SET_VECTOR(addr_up, up);

	return 1;
}

static cell_t NormalizeVector(IPluginContext *pContext, const cell_t *params)
{
	cell_t *addr;

	pContext->LocalToPhysAddr(params[1], &addr);

	Vector source(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));
	float length = VectorNormalize(source);

	pContext->LocalToPhysAddr(params[2], &addr);
	SET_VECTOR(addr, source);

	return sp_ftoc(length);
}

REGISTER_NATIVES(vectorNatives)
{
	{"GetAngleVectors",			GetAngleVectors},
	{"GetVectorAngles",			GetVectorAngles},
	{"GetVectorCrossProduct",	GetVectorCrossProduct},
	{"GetVectorDistance",		GetVectorDistance},
	{"GetVectorDotProduct",		GetVectorDotProduct},
	{"GetVectorLength",			GetVectorLength},
	{"GetVectorVectors",		GetVectorVectors},
	{"NormalizeVector",			NormalizeVector},
	{NULL,					NULL}
};
