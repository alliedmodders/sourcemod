#ifndef _INCLUDE_SOURCEMOD_GLOBALS_H_
#define _INCLUDE_SOURCEMOD_GLOBALS_H_

/**
 * @file Contains global headers that most files in SourceMod will need.
 */

#include <sp_vm_types.h>
#include <sp_vm_api.h>
#include "sm_platform.h"
#include "interfaces/IShareSys.h"

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
private:
	SMGlobalClass *m_pGlobalClassNext;
	static SMGlobalClass *head;
};

extern ISourcePawnEngine *g_pSourcePawn;
extern IVirtualMachine *g_pVM;
extern IdentityToken_t *g_pCoreIdent;

#include "sm_autonatives.h"

#endif //_INCLUDE_SOURCEMOD_GLOBALS_H_
