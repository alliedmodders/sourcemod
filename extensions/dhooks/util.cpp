/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod DHooks Extension
 * Copyright (C) 2019 AlliedModders LLC.  All rights reserved.
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

#include "util.h"

void * GetObjectAddr(HookParamType type, unsigned int flags, void **params, size_t offset)
{
#ifdef  WIN32
	if (type == HookParamType_Object)
		return (void *)((intptr_t)params + offset);
#elif POSIX
	if (type == HookParamType_Object && !(flags & PASSFLAG_ODTOR)) //Objects are passed by rrefrence if they contain destructors.
		return (void *)((intptr_t)params + offset);
#endif
	return *(void **)((intptr_t)params + offset);

}

size_t GetParamOffset(HookParamsStruct *paramStruct, unsigned int index)
{
	size_t offset = 0;
	for (unsigned int i = 0; i < index; i++)
	{
#ifndef WIN32
		if (paramStruct->dg->params.at(i).type == HookParamType_Object && (paramStruct->dg->params.at(i).flags & PASSFLAG_ODTOR)) //Passed by refrence
		{
			offset += sizeof(void *);
			continue;
		}
#endif
		offset += paramStruct->dg->params.at(i).size;
	}
	return offset;
}

size_t GetParamTypeSize(HookParamType type)
{
	return sizeof(void *);
}

size_t GetParamsSize(DHooksCallback *dg)//Get the full size, this is for creating the STACK.
{
	size_t res = 0;

	for (int i = dg->params.size() - 1; i >= 0; i--)
	{
		res += dg->params.at(i).size;
	}

	return res;
}
