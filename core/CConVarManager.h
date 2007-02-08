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

#ifndef _INCLUDE_SOURCEMOD_CCONVARMANAGER_H_
#define _INCLUDE_SOURCEMOD_CCONVARMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "HandleSys.h"
#include "sm_trie.h"
#include <IRootConsoleMenu.h>

class CConVarManager :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IRootConsoleCommand
{
public:
	CConVarManager();
	~CConVarManager();
public: // SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: // IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *command, unsigned int argcount);
public:
	inline HandleType_t GetHandleType()
	{
		return m_ConVarType;
	}
public:
	Handle_t CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal, const char *helpText,
	                      int flags, bool hasMin, float min, bool hasMax, float max);
	Handle_t FindConVar(const char* name);
private:
	HandleType_t m_ConVarType;
	Trie *m_CvarCache;
};

extern CConVarManager g_ConVarManager;

#endif // _INCLUDE_SOURCEMOD_CCONVARMANAGER_H_