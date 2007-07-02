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

#ifndef _INCLUDE_SOURCEMOD_VALVE_CALLER_H_
#define _INCLUDE_SOURCEMOD_VALVE_CALLER_H_

#include <sh_stack.h>
#include <extensions/IBinTools.h>
#include "vdecoder.h"

using namespace SourceMod;
using namespace SourceHook;

/**
 * @brief Info necessary to call a Valve function
 */
struct ValveCall
{
	ICallWrapper *call;			/**< From IBinTools */
	ValvePassInfo *vparams;		/**< Valve parameter info */
	ValvePassInfo *retinfo;		/**< Return buffer info */
	ValvePassInfo *thisinfo;	/**< Thiscall info */
	size_t stackSize;			/**< Stack size */
	size_t stackEnd;			/**< End of the bintools stack */
	unsigned char *retbuf;		/**< Return buffer */
	CStack<unsigned char *> stk; /**< Parameter stack */

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
