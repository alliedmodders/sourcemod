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

#include "sm_autonatives.h"
#include "PluginSys.h"

void CoreNativesToAdd::OnSourceModAllInitialized()
{
	g_PluginSys.RegisterNativesFromCore(m_NativeList);
}
