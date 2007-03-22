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

#ifndef _INCLUDE_SOURCEMOD_GLOBALS_H_
#define _INCLUDE_SOURCEMOD_GLOBALS_H_

/**
 * @file Contains global headers that most files in SourceMod will need.
 */

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "sm_platform.h"
#include <IShareSys.h>

using namespace SourcePawn;
using namespace SourceMod;

/** 
 * @brief Any class deriving from this will be automatically initiated/shutdown by SourceMod
 */
class SMGlobalClass
{
	friend class SourceModBase;
public:
	SMGlobalClass();
public:
	/**
	 * @brief Called when SourceMod is initially loading
	 */
	virtual void OnSourceModStartup(bool late)
	{
	}

	/**
	 * @brief Called after all global classes have initialized
	 */
	virtual void OnSourceModAllInitialized()
	{
	}

	/**
	 * @brief Called when SourceMod is shutting down
	 */
	virtual void OnSourceModShutdown()
	{
	}

	/**
	* @brief Called after SourceMod is completely shut down
	*/
	virtual void OnSourceModAllShutdown()
	{
	}
private:
	SMGlobalClass *m_pGlobalClassNext;
	static SMGlobalClass *head;
};

extern ISourcePawnEngine *g_pSourcePawn;
extern IVirtualMachine *g_pVM;
extern IdentityToken_t *g_pCoreIdent;

#include "sm_autonatives.h"

#endif //_INCLUDE_SOURCEMOD_GLOBALS_H_
