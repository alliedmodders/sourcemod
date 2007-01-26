/**
 * vim: set ts=4 :
 * ================================================================
 * SourcePawn, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ================================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_
#define _INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_

/**
 * @file sp_typeutil.h
 * @brief Defines type utility functions.
 */

#include "sp_vm_types.h"

/**
 * @brief Reinterpret-casts a float to a cell (requires -fno-strict-aliasing for GCC).
 *
 * @param val		Float value.
 * @return			Cell typed version.
 */
inline cell_t sp_ftoc(float val)
{
	return *(cell_t *)&val;
}

/**
 * @brief Reinterpret-casts a cell to a float (requires -fno-strict-aliasing for GCC).
 *
 * @param val		Cell-packed float value.
 * @return			Float typed version.
 */
inline float sp_ctof(cell_t val)
{
	return *(float *)&val;
}

#endif //_INCLUDE_SOURCEPAWN_VM_TYPEUTIL_H_

