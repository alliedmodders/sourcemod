#ifndef _INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_
#define _INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_

#include "sourcemod.h"
#include "PluginSys.h"
#include "sourcemm_api.h"
#include <convar.h>

using namespace SourceMod;

class ConVarAccessor : 
	public IConCommandBaseAccessor,
	public SMGlobalClass
{
public: // IConCommandBaseAccessor
	virtual bool RegisterConCommandBase(ConCommandBase *pCommand);
public: // SMGlobalClass
	virtual void OnSourceModStartup(bool late);
};

#endif //_INCLUDE_SOURCEMOD_SERVERCOMMANDS_H_
