/**
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

#ifndef _INCLUDE_SOURCEMOD_CDBGREPORTER_H_
#define _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

#include "sp_vm_api.h"
#include "sm_globals.h"

class DebugReport : 
	public SMGlobalClass, 
	public IDebugListener
{
public: // SMGlobalClass
	void OnSourceModAllInitialized();
public: // IDebugListener
	void OnContextExecuteError(IPluginContext *ctx, IContextTrace *error);
private:
	int _GetPluginIndex(IPluginContext *ctx);
};

extern DebugReport g_DbgReporter;

#endif // _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

