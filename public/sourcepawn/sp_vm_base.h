/**
 * vim: set ts=4 :
 * ================================================================
 * SourcePawn
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License, 
 * version 3.0, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to 
 * link the code of this program (as well as its derivative works) to 
 * "Half-Life 2," the "Source Engine," the "SourcePawn JIT," and any 
 * Game MODs that run on software by the Valve Corporation.  You must 
 * obey the GNU General Public License in all respects for all other 
 * code used. Additionally, AlliedModders LLC grants this exception 
 * to all derivative works. AlliedModders LLC defines further 
 * exceptions, found in LICENSE.txt (as of this writing, version 
 * JULY-31-2007), or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_VM_BASE_H_
#define _INCLUDE_SOURCEPAWN_VM_BASE_H_

/**
 * @file sp_vm_base.h
 * @brief Contains JIT export/linkage macros.
 */

#include <sp_vm_api.h>

/* :TODO: rename this to sp_vm_linkage.h */

#if defined WIN32
#define EXPORT_LINK		extern "C" __declspec(dllexport)
#elif defined __GNUC__
#define EXPORT_LINK		extern "C" __attribute__((visibility("default")))
#endif

/** No longer used */
typedef SourcePawn::IVirtualMachine *(*SP_GETVM_FUNC)(SourcePawn::ISourcePawnEngine *);

#endif //_INCLUDE_SOURCEPAWN_VM_BASE_H_

