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

#include "ThreadSupport.h"
#include "sm_globals.h"
#include "ShareSys.h"

#if defined PLATFORM_POSIX
#include "thread/PosixThreads.h"
#elif defined PLATFORM_WINDOWS
#include "thread/WinThreads.h"
#endif

MainThreader g_MainThreader;
IThreader *g_pThreader = &g_MainThreader;

class RegThreadStuff : public SMGlobalClass
{
public:
	void OnSourceModAllInitialized()
	{
		g_ShareSys.AddInterface(NULL, g_pThreader);
	}
} s_RegThreadStuff;
