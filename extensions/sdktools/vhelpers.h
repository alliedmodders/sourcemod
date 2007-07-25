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

#ifndef _INCLUDE_SDKTOOLS_VHELPERS_H_
#define _INCLUDE_SDKTOOLS_VHELPERS_H_

#include <sh_list.h>
#include <eiface.h>
#include <IBinTools.h>

using namespace SourceMod;

struct CallHelper
{
	CallHelper() : call(NULL), supported(false), setup(false)
	{
	}
	void Shutdown()
	{
		if (call)
		{
			call->Destroy();
			call = NULL;
			supported = false;
		}
	}
	ICallWrapper *call;
	bool supported;
	bool setup;
};

void Teleport(CBaseEntity *pEntity, Vector *origin, QAngle *ang, Vector *velocity);
bool IsTeleportSupported();

void GetVelocity(CBaseEntity *pEntity, Vector *velocity, AngularImpulse *angvelocity);
bool IsGetVelocitySupported();

void ShutdownHelpers();

#endif //_INCLUDE_SDKTOOLS_VHELPERS_H_
