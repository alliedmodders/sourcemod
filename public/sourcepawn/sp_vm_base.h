/**
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

