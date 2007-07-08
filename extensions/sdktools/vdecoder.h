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

#ifndef _INCLUDE_SOURCEMOD_VDECODER_H_
#define _INCLUDE_SOURCEMOD_VDECODER_H_

#include <sm_platform.h>
#include <sp_vm_api.h>
#include <extensions/IBinTools.h>

using namespace SourceMod;
using namespace SourcePawn;

/**
 * @brief Encapsulates types from the SDK
 */
enum ValveType
{
	Valve_CBaseEntity,		/**< CBaseEntity */
	Valve_CBasePlayer,		/**< CBasePlayer (disallow normal ents) */
	Valve_Vector,			/**< Vector */
	Valve_QAngle,			/**< QAngle */
	Valve_POD,				/**< Plain old data */
	Valve_Float,			/**< Float */
	Valve_Edict,			/**< Edict */
	Valve_String,			/**< String */
	Valve_Bool,				/**< Boolean */
};

enum DataStatus
{
	Data_Fail = 0,
	Data_Okay = 1,
};

#define VDECODE_FLAG_ALLOWNULL		(1<<0)		/**< Allow NULL for pointers */
#define VDECODE_FLAG_ALLOWNOTINGAME	(1<<1)		/**< Allow players not in game */
#define VDECODE_FLAG_ALLOWWORLD		(1<<2)		/**< Allow World entity */
#define VDECODE_FLAG_BYREF			(1<<3)		/**< Floats/ints by reference */

#define VENCODE_FLAG_COPYBACK		(1<<0)		/**< Copy back data */

#define PASSFLAG_ASPOINTER			(1<<30)		/**< Not an actual passflag, used internally */

/**
 * @brief Valve pre-defined calling types
 */
enum ValveCallType
{
	ValveCall_Static,		/**< Static call */
	ValveCall_Entity,		/**< Thiscall (CBaseEntity implicit first parameter) */
	ValveCall_Player,		/**< Thiscall (CBasePlayer implicit first parameter) */
	ValveCall_GameRules,	/**< Thiscall (CGameRules implicit first paramater) */
};

/**
 * @brief Valve parameter info
 */
struct ValvePassInfo
{
	ValveType vtype;		/**< IN: Valve type */
	unsigned int decflags;	/**< IN: VDECODE_FLAG_* */
	unsigned int encflags;	/**< IN: VENCODE_FLAG_* */
	PassType type;			/**< IN: Pass information */
	unsigned int flags;		/**< IN: Pass flags */
	size_t offset;			/**< OUT: stack offset */
	size_t obj_offset;		/**< OUT: object offset at end of the stack */
};

struct ValveCall;

/**
 * @brief Converts a valve parameter to a bintools parameter.
 *
 * @param type			Valve type.
 * @param pass			Either basic or object.
 * @param flags			Either BYVAL or BYREF.
 * @param info			Buffer to store param info in.
 * @return				Number of bytes this will use in the virtual stack,
 *						or 0 if conversion was impossible.
 */
size_t ValveParamToBinParam(ValveType type, 
					  PassType pass,
					  unsigned int flags,
					  PassInfo *info,
					  bool &needs_extra);

/**
 * @brief Decodes data from a plugin to native data.
 *
 * Note: If you're going to return false, make sure to 
 * throw an error.
 *
 * @param pContext		Plugin context.
 * @param param			Parameter value from params array.
 * @param buffer		Buffer space in the virutal stack.
 * @return				True on success, false otherwise.
 */
DataStatus DecodeValveParam(IPluginContext *pContext,
					  cell_t param,
					  const ValveCall *pCall,
					  const ValvePassInfo *vdata,
					  void *buffer);

/**
 * @brief Encodes native data back into a plugin.
 *
 * Note: If you're going to return false, make sure to 
 * throw an error.
 *
 * @param pContext		Plugin context.
 * @param param			Parameter value from params array.
 * @param buffer		Buffer space in the virutal stack.
 * @return				True on success, false otherwise.
 */
DataStatus EncodeValveParam(IPluginContext *pContext,
					  cell_t param,
					  const ValveCall *pCall,
					  const ValvePassInfo *vdata,
					  const void *buffer);

#endif //_INCLUDE_SOURCEMOD_VDECODER_H_
