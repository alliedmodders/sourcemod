/**
 * ================================================================
 * SourcePawn (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ================================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEPAWN_JIT_X86_DLL_H_
#define _INCLUDE_SOURCEPAWN_JIT_X86_DLL_H_

#include <sp_vm_base.h>

#if defined WIN32
#define EXPORTFUNC	extern "C" __declspec(dllexport)
#elif defined __GNUC__
#if __GNUC__ >= 3
#define EXPORTFUNC extern "C" __attribute__((visibility("default")))
#else
#define EXPORTFUNC extern "C"
#endif //__GNUC__ >= 3
#endif //defined __GNUC__

#endif //_INCLUDE_SOURCEPAWN_JIT_X86_DLL_H_
