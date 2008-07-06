/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_VALVE_CALLER_H_
#define _INCLUDE_SOURCEMOD_VALVE_CALLER_H_

#include <sh_stack.h>
#include <extensions/IBinTools.h>
#include "vdecoder.h"

using namespace SourceMod;

/**
 * @brief Info necessary to call a Valve function
 */
struct ValveCall
{
	ICallWrapper *call;							/**< From IBinTools */
	ValveCallType type;							/**< Call type */
	ValvePassInfo *vparams;						/**< Valve parameter info */
	ValvePassInfo *retinfo;						/**< Return buffer info */
	ValvePassInfo *thisinfo;					/**< Thiscall info */
	size_t stackSize;							/**< Stack size */
	size_t stackEnd;							/**< End of the bintools stack */
	unsigned char *retbuf;						/**< Return buffer */
	SourceHook::CStack<unsigned char *> stk;	/**< Parameter stack */

	unsigned char *stk_get();
	void stk_put(unsigned char *ptr);
	ValveCall();
	~ValveCall();
};

ValveCall *CreateValveVCall(unsigned int vtableIdx,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams);

ValveCall *CreateValveCall(void *addr,
							ValveCallType vcalltype,
							const ValvePassInfo *retInfo,
							const ValvePassInfo *params,
							unsigned int numParams);

#endif //_INCLUDE_SOURCEMOD_VALVE_CALLER_H_
